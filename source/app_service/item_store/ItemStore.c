////////////////////////////////////////////////////////////////////////////////
//  S E N S I R I O N   AG,  Laubisruetistr. 50, CH-8712 Staefa, Switzerland
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023, Sensirion AG
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////

/// @file ItemStore.c
#include "ItemStore.h"

#include "hal/Flash.h"
#include "stm32wbxx_hal.h"
#include "stm32wbxx_hal_flash.h"
#include "utility/ErrorHandler.h"
#include "utility/scheduler/Message.h"
#include "utility/scheduler/MessageListener.h"

#include <string.h>
/// First page of the system settings item
#define SYSTEM_CONFIG_FIRST_PAGE 65

/// Last page of the system settings item
#define SYSTEM_CONFIG_LAST_PAGE SYSTEM_CONFIG_FIRST_PAGE + 1

/// First page of the measurement item
#define MEASUREMENT_VALUES_FIRST_PAGE SYSTEM_CONFIG_LAST_PAGE + 1

/// Last page of the measurement item
#define MEASUREMENT_VALUES_LAST_PAGE MEASUREMENT_VALUES_FIRST_PAGE + 16

/// Compute the flash address from a page number
#define PAGE_ADDR(x) ((x)*FLASH_PAGE_SIZE) + FLASH_BASE

/// Get the number of element within an array
#define COUNT_OF(arr) sizeof((arr)) / sizeof((arr)[0])

/// Upper bound for pageBlock indices
#define MAX_BLOCK_INDEX 32

/// A tag to identify the header of a page;
#define PAGE_MAGIC 0xA53CC35A

/// Compute the following page number in the specified item_store.
#define NEXT_PAGE_NR(item_store, page_nr)                             \
  ((((page_nr) + 1) > (item_store->lastPage)) ? item_store->firstPage \
                                              : ((page_nr) + 1))

/// Check the consistency of the page begin tag
#define BEGIN_TAG_IS_CONSISTENT(store, header, actual_page) \
  ((header.beginTag.magic == PAGE_MAGIC) &&                 \
   (header.beginTag.pageId == actual_page) &&               \
   (header.beginTag.itemSize == store->itemSize))

/// Check the consistency of the page end tag
#define COMPLETE_TAG_IS_CONSISTENT(store, header, actual_page)     \
  ((header.completeTag.magic == PAGE_MAGIC) &&                     \
   (header.completeTag.nrOfItems ==                                \
    ((FLASH_PAGE_SIZE - sizeof(header)) / ((store)->itemSize))) && \
   (header.completeTag.nextPage == NEXT_PAGE_NR(store, actual_page)))

/// Marker that page is in use
typedef struct {
  uint32_t magic;   ///< tag to flag a page that contains valid data
  uint8_t pageId;   ///< id of the page; simplifies the address computation
  uint8_t blockId;  ///< id of the block; required to find the most recent page
  ItemStore_ItemDef_t itemId;  ///< id of the item that is written in that page
  uint8_t itemSize;            ///< size of an item on this page
} PageBeginTag_t;

/// Marker that page is complete; no further items fit
typedef struct {
  uint32_t magic;      ///< tag to flag a page that is filled with valid data
  uint16_t nrOfItems;  ///< number of items stored in this page
  uint8_t nextPage;    ///< next page that will be used
  uint8_t nextPageId;  ///< next page id that will be used
} PageCompleteTag_t;

/// Describe the data that are used to
/// handle the page buffers for a specific item;
typedef struct {
  uint8_t firstPage;  ///< Number of the first page of this item store.
  uint8_t lastPage;   ///< Number of the last page of this item store.
  uint8_t itemSize;   ///< Size of the items in this info store.
  /// Number of pages this item store may use.
  /// For items that do not need history and very frequent update this value is 2
  uint8_t nrOfPages;
  /// Number of completely filled pages; used for efficient counting of items
  uint8_t nrOfFullPages;
  /// Tag of the page that is processed while initializing this item store.
  uint16_t currentPageNrOfItems;
  /// Tag of page that is currently processed during recovery of item store.
  PageBeginTag_t currentPageInfo;
  /// Tag of page where the next write will happen
  PageBeginTag_t nextWritePageInfo;
  /// Tag of the oldest page
  PageBeginTag_t oldestPageInfo;
  /// The state of the item store; the item store may be in the state
  /// enumerating or idle
  MessageListener_HandleReceivedMessageCb_t currentState;
} ItemStoreInfo_t;

/// Describes the header of a page.
/// This information is used to reconstruct the
/// order of items after reset of the device.
typedef struct {
  PageBeginTag_t beginTag;        ///< Marks the begin of the page
  PageCompleteTag_t completeTag;  ///< Marks the completeness of the page
} PageHeader_t;

/// Parameter of the AddItem message
typedef struct {
  ItemStore_ItemStruct_t* data;  ///< Content to be written to the item store
} AddItemParameters_t;

/// Parameter of the ErasePage message
typedef struct {
  uint8_t pageNumber;  ///< Number of page where the erase starts
  uint8_t nrOfPages;   ///< Number of pages to erase
} EraseParameters_t;

/// Messages that can be sent
typedef enum {
  /// Add a new item to the item store.
  /// Adding new items is not possible while enumerating or erasing.
  ITEM_STORE_MESSAGE_ADD_ITEM = 0,
  /// Erase a page if no space is left.
  /// It is not possible to start an erase while another erase is ongoing or
  /// while enumerating an item store.
  ITEM_STORE_MESSAGE_ERASE,
  /// Notify that the erase is done.
  ITEM_STORE_MESSAGE_ERASE_DONE,
  /// Enumerate the items within an item store.
  ITEM_STORE_MESSAGE_ENUMERATE
} ItemStoreMessageId_t;

/// Defines an ItemStore message;
typedef struct {
  MessageBroker_MsgHead_t header;  ///< Message header
  union {
    const ItemStore_ItemStruct_t* addParameter;  ///< Parameter for add item
    EraseParameters_t eraseParameter;            ///< Parameter for erase page
  } data;                                        ///< Message data
} ItemStoreMessage_t;

/// Idle state of the ItemStore Listener
/// @param message The message to be dispatched
/// @return true if the message was handled; false otherwise
static bool ListenerIdleState(Message_Message_t* message);

/// State of the ItemStore Listener while waiting on erase complete.
/// @param message The message to be dispatched
/// @return true if the message was handled; false otherwise
static bool ListenerErasingState(Message_Message_t* message);

/// State of the item Store
/// @param message The message to be dispatched
/// @return true if the message was handled; false otherwise
static bool IdleState(Message_Message_t* message);

/// Synchronously write an item to the flash
/// @param item Selects the item store to write the data
/// @param data The data that is written
/// @return true if the operation succeeds; false otherwise
static bool AddItem(ItemStore_ItemDef_t item,
                    const ItemStore_ItemStruct_t* data);

/// Initialize the data structure of a single item store.
/// This operation reconstructs the item store metadata from the contents of the
/// flash.
/// @param itemStoreInfo Pointer to item store info
/// @param id The id of the item store.
static void InitItemStore(ItemStoreInfo_t* itemStoreInfo,
                          ItemStore_ItemDef_t id);

/// Count the number of entries on the current page
/// This assumes that the currentPageInfo field of the itemStoreInfo is
/// properly set.
/// @param itemStoreInfo Pointer to an item store instance.
/// @return The number of items on the page.
static uint32_t CountItemsOnCurrentPage(ItemStoreInfo_t* itemStoreInfo);

/// Check if there is valid data in the buffer;
/// The flash erase state is all 1.
/// @param buffer A buffer containing previously read data
/// @param nrOfBytes number of bytes to be checked
/// @return true if all bits are 1; false otherwise
static bool HasNoData(const uint8_t* buffer, uint8_t nrOfBytes);

/// Write a single data item to an item store.
/// The write is done synchronously
/// @param itemStoreInfo Pointer to an item store instance.
/// @param data data to be written
/// @return true if the operation succeeds; false otherwise
static bool WriteItem(ItemStoreInfo_t* itemStoreInfo,
                      const ItemStore_ItemStruct_t* data);

/// Write the complete page tag if a page is complete.
///
/// It is checked if more items can be written to this page. If this is not
/// the case, the PageCompleteTag is written and the nextWritePage is advanced.
/// In case the nextWritePage is not free a page erase is triggered.
/// @param itemStoreInfo  Pointer to an item store instance.
/// @return true if the operation succeeds; false otherwise.
static bool ClosePageIfFull(ItemStoreInfo_t* itemStoreInfo);

/// Update oldest and newest page based on the block id read from flash.
/// @param itemStoreInfo The item store that is initialized
/// @param initialize Flag to indicate that oldest and newst page have to be
///                   initialized
static void UpdateNewestOldestPage(ItemStoreInfo_t* itemStoreInfo,
                                   bool initialize);

/// Adjust the next page to write items
///
/// In case all pages are complete, the next page to receive new items
/// needs to be adjusted. This advances the nextWritePage and triggers
/// an erase page if the nextWritePage is not empty.
/// @param itemStoreInfo Pointer to the ItemStore
/// @param completeTag CompletePage marker that contains information
///                    about next write page.
/// @return true if the operation succeeds; false otherwise.
static bool AdjustNextWritePage(ItemStoreInfo_t* itemStoreInfo,
                                PageCompleteTag_t* completeTag);

/// Callback that indicates the completion of a page erase
/// @param pageId The page id of the erased page.
static void FlashEraseDoneCb(uint32_t pageId);

/// list metadata of item stores
ItemStoreInfo_t _itemStore[] = {
    [ITEM_DEF_SYSTEM_CONFIG] = {.firstPage = SYSTEM_CONFIG_FIRST_PAGE,
                                .lastPage = SYSTEM_CONFIG_LAST_PAGE,
                                .nrOfPages = 2,
                                .itemSize = sizeof(ItemStore_SystemConfig_t),
                                .currentState = IdleState},
    [ITEM_DEF_MEASUREMENT_SAMPLE] = {.firstPage = MEASUREMENT_VALUES_FIRST_PAGE,
                                     .lastPage = MEASUREMENT_VALUES_LAST_PAGE,
                                     .nrOfPages = 17,
                                     .itemSize =
                                         sizeof(ItemStore_MeasurementSample_t),
                                     .currentState = IdleState},
};

/// Message listeners to receive application message for the item store
MessageListener_Listener_t _messageListener = {
    .currentMessageHandlerCb = ListenerIdleState,
    .receiveMask = MESSAGE_BROKER_CATEGORY_ITEM_STORE};

/// Reminder of an erase operation that cannot be handled at the time
/// it was requested.
EraseParameters_t _eraseReminder;

/// Get the ItemStore message listener
/// @return Pointer to the ItemStore_Listener
MessageListener_Listener_t* ItemStore_ListenerInstance() {
  return &_messageListener;
}

void ItemStore_Init() {
  for (uint8_t i = 0; i < COUNT_OF(_itemStore); i++) {
    InitItemStore(&_itemStore[i], (ItemStore_ItemDef_t)i);
  }
}

// Add item must run asynchronously since it is only allowed to
// add items, while no flash erase is ongoing!
void ItemStore_AddItem(ItemStore_ItemDef_t item,
                       const ItemStore_ItemStruct_t* data) {
  ItemStoreMessage_t msg = {
      .header.category = MESSAGE_BROKER_CATEGORY_ITEM_STORE,
      .header.id = ITEM_STORE_MESSAGE_ADD_ITEM,
      .header.parameter1 = item,
      .data.addParameter = data};
  Message_PublishAppMessage((Message_Message_t*)&msg);
}

// Implement add item. Writing to flash is synchronous. In case a page gets full
// an erase may be required to make another page free for next insert.
// This is then an asynchronous operation.
static bool AddItem(ItemStore_ItemDef_t item,
                    const ItemStore_ItemStruct_t* data) {
  ItemStoreInfo_t* itemStoreInfo = &_itemStore[item];
  PageHeader_t header;
  uint32_t pageAddress = PAGE_ADDR(itemStoreInfo->nextWritePageInfo.pageId);
  if (!Flash_Read(pageAddress, (uint8_t*)&header, sizeof(header))) {
    return false;
  }
  // first write to this page
  if (HasNoData((uint8_t*)&header, sizeof(header))) {
    // write the page header start tag
    if (!Flash_Write(pageAddress, (uint8_t*)&itemStoreInfo->nextWritePageInfo,
                     sizeof(PageBeginTag_t))) {
      return false;
    }
    itemStoreInfo->currentPageNrOfItems = 0;
    return WriteItem(itemStoreInfo, data);
  }
  // the page is already in use
  if (HasNoData((uint8_t*)&header.completeTag, sizeof(PageCompleteTag_t))) {
    if (!WriteItem(itemStoreInfo, data)) {
      return false;
    }
    // Check if page is full.
    return ClosePageIfFull(itemStoreInfo);
  }
  return false;
}

static void InitItemStore(ItemStoreInfo_t* itemStoreInfo,
                          ItemStore_ItemDef_t id) {
  itemStoreInfo->currentPageInfo.magic = PAGE_MAGIC;
  itemStoreInfo->currentPageInfo.pageId = itemStoreInfo->firstPage;
  itemStoreInfo->currentPageInfo.blockId = 0;
  itemStoreInfo->currentPageInfo.itemId = id;
  itemStoreInfo->currentPageInfo.itemSize = itemStoreInfo->itemSize;
  // in case we have no valid data we still need a valid initialization
  itemStoreInfo->nextWritePageInfo = itemStoreInfo->currentPageInfo;
  itemStoreInfo->oldestPageInfo = itemStoreInfo->currentPageInfo;
  PageHeader_t pageHeader = {0};
  for (uint8_t i = 0; i < itemStoreInfo->nrOfPages; i++) {
    uint8_t actualPageId = i + itemStoreInfo->firstPage;
    Flash_Read(PAGE_ADDR(actualPageId), (uint8_t*)&pageHeader,
               sizeof(pageHeader));
    if (HasNoData((uint8_t*)&pageHeader,
                  sizeof(pageHeader))) {  // no valid data
      continue;
    }
    if (!BEGIN_TAG_IS_CONSISTENT(itemStoreInfo, pageHeader, actualPageId)) {
      ErrorHandler_RecoverableErrorExtended(ERROR_CODE_ITEM_STORE,
                                            actualPageId);
    }

    itemStoreInfo->currentPageInfo = pageHeader.beginTag;
    UpdateNewestOldestPage(itemStoreInfo, i == 0);

    if (!HasNoData(
            (uint8_t*)&pageHeader.completeTag,  // page is marked complete
            sizeof pageHeader.completeTag)) {
      if (!COMPLETE_TAG_IS_CONSISTENT(itemStoreInfo, pageHeader,
                                      actualPageId)) {
        ErrorHandler_RecoverableErrorExtended(ERROR_CODE_ITEM_STORE,
                                              actualPageId);
      }
      itemStoreInfo->nrOfFullPages++;
    } else {  // there is remaining space
      itemStoreInfo->currentPageNrOfItems =
          CountItemsOnCurrentPage(itemStoreInfo);
      // Add the close tag if it was not written properly
      ClosePageIfFull(itemStoreInfo);
    }
  }
  // If the page to write the data is full we have to move it to the next
  // free page and eventually erase the oldest page
  Flash_Read(PAGE_ADDR(itemStoreInfo->nextWritePageInfo.pageId),
             (uint8_t*)&pageHeader, sizeof pageHeader);
  if (pageHeader.completeTag.magic == PAGE_MAGIC) {
    AdjustNextWritePage(itemStoreInfo, &pageHeader.completeTag);
  }
}

static bool WriteItem(ItemStoreInfo_t* itemStoreInfo,
                      const ItemStore_ItemStruct_t* data) {
  // never write into a page that does not belong to the item store!
  ASSERT(
      (itemStoreInfo->firstPage <= itemStoreInfo->nextWritePageInfo.pageId) &&
      (itemStoreInfo->lastPage >= itemStoreInfo->nextWritePageInfo.pageId));
  uint32_t pageAddress = PAGE_ADDR(itemStoreInfo->nextWritePageInfo.pageId);
  uint32_t writeAddress =
      pageAddress + sizeof(PageHeader_t) +
      itemStoreInfo->currentPageNrOfItems * itemStoreInfo->itemSize;
  if (!Flash_Write(writeAddress, (uint8_t*)data, itemStoreInfo->itemSize)) {
    return false;
  }
  itemStoreInfo->currentPageNrOfItems += 1;
  return true;
}

static bool ClosePageIfFull(ItemStoreInfo_t* itemStoreInfo) {
  uint8_t actualPage = itemStoreInfo->nextWritePageInfo.pageId;
  uint32_t pageAddress = PAGE_ADDR(actualPage);

  uint32_t writeAddress =
      pageAddress + sizeof(PageHeader_t) +
      itemStoreInfo->currentPageNrOfItems * itemStoreInfo->itemSize;
  // no new item fits in this page
  if (writeAddress + itemStoreInfo->itemSize > pageAddress + FLASH_PAGE_SIZE) {
    uint8_t nextPage = NEXT_PAGE_NR(itemStoreInfo, actualPage);
    // Prepare close tag and write to flash
    PageCompleteTag_t completeTag = {
        .magic = PAGE_MAGIC,
        .nextPage = nextPage,
        .nextPageId =
            (itemStoreInfo->nextWritePageInfo.blockId + 1) % MAX_BLOCK_INDEX,
        .nrOfItems = itemStoreInfo->currentPageNrOfItems};

    if (!Flash_Write(pageAddress + sizeof(PageBeginTag_t),
                     (uint8_t*)&completeTag, sizeof(PageCompleteTag_t))) {
      return false;
    }
    itemStoreInfo->nrOfFullPages++;

    // adjust currentPageInfo to point to the next page
    itemStoreInfo->nextWritePageInfo.blockId = completeTag.nextPageId;
    itemStoreInfo->nextWritePageInfo.pageId = nextPage;

    // check if next page to write is correct and empty
    return AdjustNextWritePage(itemStoreInfo, &completeTag);
  }
  return true;
}

static void UpdateNewestOldestPage(ItemStoreInfo_t* itemStoreInfo,
                                   bool initialize) {
  // initialize the newest and oldest with a value that exists on the flash
  if (initialize) {
    itemStoreInfo->oldestPageInfo = itemStoreInfo->currentPageInfo;
    itemStoreInfo->nextWritePageInfo = itemStoreInfo->currentPageInfo;
    return;
  }
  uint8_t nrOfPages = itemStoreInfo->nrOfPages;
  uint8_t currentBlockId = itemStoreInfo->currentPageInfo.blockId;
  uint8_t newestBlockId = itemStoreInfo->nextWritePageInfo.blockId;
  uint8_t oldestBlockId = itemStoreInfo->oldestPageInfo.blockId;
  if (((currentBlockId > newestBlockId) &&  // no wrap around
       ((currentBlockId - newestBlockId) < nrOfPages)) ||
      ((currentBlockId < newestBlockId) &&  // there was a wrap around
       ((newestBlockId - currentBlockId) > nrOfPages))) {
    itemStoreInfo->nextWritePageInfo = itemStoreInfo->currentPageInfo;
  } else if (((currentBlockId < oldestBlockId) &&
              ((oldestBlockId - currentBlockId) < nrOfPages)) ||
             ((currentBlockId > oldestBlockId) &&
              (currentBlockId - oldestBlockId > nrOfPages))) {
    itemStoreInfo->oldestPageInfo = itemStoreInfo->currentPageInfo;
  }
}

static uint32_t CountItemsOnCurrentPage(ItemStoreInfo_t* itemStoreInfo) {
  uint32_t pageStart = PAGE_ADDR(itemStoreInfo->currentPageInfo.pageId);
  uint32_t address = pageStart + sizeof(PageHeader_t);
  uint16_t nrOfItems = 0;
  uint8_t readBuffer[sizeof(ItemStore_ItemStruct_t)];
  while (address < (pageStart + FLASH_PAGE_SIZE)) {
    Flash_Read(address, readBuffer, itemStoreInfo->itemSize);
    if (HasNoData(readBuffer, itemStoreInfo->itemSize)) {
      return nrOfItems;
    }
    nrOfItems += 1;
    address += itemStoreInfo->itemSize;
  }
  return nrOfItems;
}

static bool HasNoData(const uint8_t* buffer, uint8_t nrOfBytes) {
  for (uint8_t i = 0; i < nrOfBytes; i++) {
    if (buffer[i] != 0xFF) {
      return false;
    }
  }
  return true;
}

static bool AdjustNextWritePage(ItemStoreInfo_t* itemStoreInfo,
                                PageCompleteTag_t* completeTag) {
  PageHeader_t header;
  // check if next page is empty
  if (!Flash_Read(PAGE_ADDR(completeTag->nextPage), (uint8_t*)&header,
                  sizeof header)) {
    return false;
  }
  // initialize the current item header such that it is ready to be written
  // to the new flash page
  itemStoreInfo->nextWritePageInfo.pageId = completeTag->nextPage;
  itemStoreInfo->nextWritePageInfo.blockId = completeTag->nextPageId;

  // if the page is empty nothing needs to be done
  if (HasNoData((uint8_t*)&header, sizeof(header))) {
    return true;
  }

  // get the new oldest page information
  if (!Flash_Read(PAGE_ADDR(header.completeTag.nextPage), (uint8_t*)&header,
                  sizeof header)) {
    return false;
  }
  // the oldest page is removed
  ASSERT(itemStoreInfo->oldestPageInfo.pageId == completeTag->nextPage);
  // the oldest entry is removed -> read next oldest page
  itemStoreInfo->oldestPageInfo = header.beginTag;

  // else erase the next page; during this time, no other request
  // will be handled
  itemStoreInfo->nrOfFullPages--;
  ItemStoreMessage_t msg = {
      .header.category = MESSAGE_BROKER_CATEGORY_ITEM_STORE,
      .header.id = ITEM_STORE_MESSAGE_ERASE,
      .data.eraseParameter = {.pageNumber = completeTag->nextPage,
                              .nrOfPages = 1}};
  Message_PublishAppMessage((Message_Message_t*)&msg);
  return true;
}

static bool ListenerIdleState(Message_Message_t* message) {
  ASSERT(message->header.parameter1 < 2);
  ItemStoreMessage_t* msg = (ItemStoreMessage_t*)message;
  if (message->header.id == ITEM_STORE_MESSAGE_ERASE) {
    Flash_Erase(msg->data.eraseParameter.pageNumber,
                msg->data.eraseParameter.nrOfPages, FlashEraseDoneCb);
    _messageListener.currentMessageHandlerCb = ListenerErasingState;
    return true;
  }
  return _itemStore[message->header.parameter1].currentState(message);
}

static bool IdleState(Message_Message_t* message) {
  if (message->header.id == ITEM_STORE_MESSAGE_ADD_ITEM) {
    ItemStoreMessage_t* msg = (ItemStoreMessage_t*)message;
    if (!AddItem(msg->header.parameter1, msg->data.addParameter)) {
      ErrorHandler_RecoverableError(ERROR_CODE_ITEM_STORE);
    }
    return true;
  }
  if (message->header.id == ITEM_STORE_MESSAGE_ENUMERATE) {
    return true;
  }
  return false;
}

static bool ListenerErasingState(Message_Message_t* message) {
  // only one erase can run at a time!
  // all other requests are ignored while being in this state!
  if (message->header.id == ITEM_STORE_MESSAGE_ERASE) {
    ItemStoreMessage_t* msg = (ItemStoreMessage_t*)message;
    _eraseReminder = msg->data.eraseParameter;
    return true;
  }
  if (message->header.id == ITEM_STORE_MESSAGE_ERASE_DONE) {
    if (_eraseReminder.pageNumber > 0) {
      Flash_Erase(_eraseReminder.pageNumber, _eraseReminder.nrOfPages,
                  FlashEraseDoneCb);
      _eraseReminder.pageNumber = 0;
    } else {
      _messageListener.currentMessageHandlerCb = ListenerIdleState;
    }
    return true;
  }
  return false;
}

static void FlashEraseDoneCb(uint32_t pageId) {
  Message_Message_t message = {
      .header.category = MESSAGE_BROKER_CATEGORY_ITEM_STORE,
      .header.id = ITEM_STORE_MESSAGE_ERASE_DONE};
  Message_PublishAppMessage(&message);
}
