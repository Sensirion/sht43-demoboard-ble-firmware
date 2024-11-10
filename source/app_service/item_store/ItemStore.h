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

/// @file ItemStore.h
#ifndef ITEM_STORE_H
#define ITEM_STORE_H

#include "utility/StaticCodeAnalysisHelper.h"
#include "utility/scheduler/MessageListener.h"

#include <stdbool.h>
#include <stdint.h>

/// Buffer size for the alternative device name
#define DEVICE_NAME_BUFFER_LENGTH 32

/// Maximal length of the alternative device name. Due to the 0 termination
/// of c-strings this is the buffer size -1.
#define DEVICE_NAME_MAX_LEN (DEVICE_NAME_BUFFER_LENGTH - 1)

/// Callback to notify the state of the enumerator as a response of a call
/// to the function `ItemStore_BeginEnumerate`
/// When the iterator is ready, it may be used to synchronously iterate through
/// the elements in the item store.
typedef void (*ItemStore_EnumeratorStatusCb_t)(bool ready);

/// Ids of the defined info items that can be stored
typedef enum {
  ITEM_DEF_SYSTEM_CONFIG = 0,
  ITEM_DEF_MEASUREMENT_SAMPLE
} ItemStore_ItemDef_t;

/// Defines the messages that are used by the item store.
/// Since other controllers  need to know when it is possible to add items
/// this enum is public.
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
  /// Begin to enumerate the items within an item store.
  ITEM_STORE_MESSAGE_BEGIN_ENUMERATE,
  /// Signal the end of the enumerate operation.
  /// Clients of the item store need to know when they can add items again.
  ITEM_STORE_MESSAGE_END_ENUMERATE
} ItemStore_MessageId_t;

/// Structure definition of item 'Configuration'
///
/// This structure is not yet finalized!
typedef struct _tItemStore_SystemConfig {
  /// Actual version of the system configuration; the version number is meant
  /// to guarantee backwards compatibility.
  uint8_t version;
  /// Enable/disable uart trace
  bool isLogEnabled;
  /// Enable/disable advertise
  bool isAdvertiseDataEnabled;
  uint8_t paddingByte;  ///< pad the remaining byte to be 4 byte aligned
  /// Name of the device that may be set via BLE
  char deviceName[DEVICE_NAME_BUFFER_LENGTH];
  uint32_t loggingInterval;  ///< logging interval in ms. smallest value 5s.
  uint8_t reserve2[84];      ///< Overall size is 128 bytes so that we
                             ///< can define further values in the future.
  uint32_t crc;              ///< Crc to check data integrity
} ItemStore_SystemConfig_t;

/// Structure definition of item 'Measurement'
/// As the flash is written in double words, we put two samples in one item.
typedef struct _tItemStore_MeasurementSample {
  struct {
    uint16_t temperatureTicks;  ///< raw measurement value of temperature
    uint16_t humidityTicks;     ///< raw measurement value of humidity
  } sample[2];                  ///< samples contained in this item
} ItemStore_MeasurementSample_t;

/// Summarize all possible item structures.
typedef union {
  ItemStore_SystemConfig_t configuration;     ///< SystemConfig item
  ItemStore_MeasurementSample_t measurement;  ///< MeasurementSample item
} ItemStore_ItemStruct_t;

/// Define an enumerator to enumerate all items of  an item store
typedef struct _tItemStore_Enumerator {
  bool hasMoreItems;        ///< Flag indicating if more items are available
  int32_t startIndex;       ///< Points to the start position where the
                            ///< enumeration will start.
                            ///< A negative index will denote a start index
                            ///< relative to the end of the collection.
  void* enumeratorDetails;  ///< Internal implementation details of the
                            ///< enumerator
} ItemStore_Enumerator_t;

/// Get the message listener of the item store
/// @return Message listener that handles massages for the item stores
MessageListener_Listener_t* ItemStore_ListenerInstance();

/// Parameter of an erase message
typedef struct {
  ItemStore_ItemDef_t itemStore;  ///< Item store on which the erase takes place
  bool reinit;                    ///< Flag if initialization is required
                                  ///< after erase.
  uint8_t pageNumber;             ///< Number of page where the erase starts
  uint8_t nrOfPages;              ///< Number of pages to erase
} ItemStore_EraseParameters_t;

ASSERT_SIZE_TYPE1_LESS_THAN_TYPE2(ItemStore_EraseParameters_t, uint32_t);

/// Initialize the item store upon reset.
///
/// The ItemStore_listenerInstance() has to be registered prior to calling ItemStore_Init()
/// since initialisation of the item store may involve an erase operation.
void ItemStore_Init();

/// Add a new item to the specified item store
/// @param item Id of the item store
/// @param data The data to be added to the item store
void ItemStore_AddItem(ItemStore_ItemDef_t item,
                       const ItemStore_ItemStruct_t* data);

/// Delete all items in the specified item store
/// All pages that belong to this item store will be erased
/// @param item Id of the item store
void ItemStore_DeleteAllItems(ItemStore_ItemDef_t item);

/// Check if there is any item in the item store.
///
/// Since we only check the first page of the item store this operation is
/// synchronous.
///
/// @param item Id of the item store to be checked.
/// @return true if there is no valid data in the item store
bool ItemStore_IsEmpty(ItemStore_ItemDef_t item);

/// Initialize an object to enumerate all items that are stored
/// in the specified item store. This operation is called asynchronously in
/// order to not interfere with pending erase operations.
/// The client is notified about the state of the enumerator with the
/// callback  `onEnumeratorReadyCb`.
/// As long as an enumerator is active, no new items can be added.
/// @param item Id of the item store to enumerate
/// @param enumerator Pointer to enumerator object that shall be initialized
/// @param onDoneCb Callback that signals that the completion of the operation.
void ItemStore_BeginEnumerate(ItemStore_ItemDef_t item,
                              ItemStore_Enumerator_t* enumerator,
                              ItemStore_EnumeratorStatusCb_t onDoneCb);

/// Close an initialized enumerator
///
/// After each successful call to `ItemStore_BeginEnumerate()` this function
/// has to be called to exit the enumerating state. Only after closing the
/// an open enumerator, new items may be added to the item store.
/// This operation is executed synchronously
/// @param enumerator Pointer to enumerator object that shall be initialized
/// @param item the item store that was enumerated
void ItemStore_EndEnumerate(ItemStore_Enumerator_t* enumerator,
                            ItemStore_ItemDef_t item);

/// Get the number of items that are stored in the specified item store
/// @param enumerator Pointer to an initialized enumerator;
/// @return Number of available items in the
///         item store if the enumerator is valid; -1 otherwise
int32_t ItemStore_Count(ItemStore_Enumerator_t* enumerator);

/// Access the next item in the item store.
/// This operation is executed synchronously.
/// @param enumerator
/// @param data Pointer to a data structure that can hold the next element
///             retrieved by the enumerator
/// @return true if the operation succeeds; false otherwise
bool ItemStore_GetNext(ItemStore_Enumerator_t* enumerator,
                       ItemStore_ItemStruct_t* data);
#endif  // ITEM_STORE_H
