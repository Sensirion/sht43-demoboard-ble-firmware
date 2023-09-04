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

#include "utility/scheduler/MessageListener.h"

#include <stdbool.h>
#include <stdint.h>

/// Ids of the defined info items that can be stored
typedef enum {
  ITEM_DEF_SYSTEM_CONFIG = 0,
  ITEM_DEF_MEASUREMENT_SAMPLE
} ItemStore_ItemDef_t;

/// Structure definition of item 'Configuration'
///
/// This structure is not yet finalized!
typedef struct _tItemStore_SystemConfig {
  char deviceName[32];       ///< name of the device that may be set via BLE
  uint32_t loggingInterval;  ///< logging interval in ms. smallest value 5s.
  bool debug;                ///< enable uart trace
  uint8_t reserve[24];       ///< overall size may be 64 bytes so that we
                             ///< can define further values in the future.
} ItemStore_SystemConfig_t;

/// Structure definition of item 'Measurement'
/// As the flash is written in double words, we put two samples in one item.
typedef struct _tItemStore_MeasurementSample {
  struct {
    uint16_t humidityTicks;     ///< raw measurement value of humidity
    uint16_t temperatureTicks;  ///< raw measurement value of temperature
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
  void* enumeratorDetails;  ///< Internal implementation details of the
                            ///< enumerator
} ItemStore_Enumerator_t;

/// Get the message listener of the item store
/// @return Message listener that handles massages for the item stores
MessageListener_Listener_t* ItemStore_ListenerInstance();

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

/// Initialize an object to enumerate all items that are stored
/// in the specified item store.
/// As long as an enumerator is active, no new items can be added.
/// @param item Id of the item store to enumerate
/// @param enumerator Pointer to enumerator object that shall be initialized
void ItemStore_BeginEnumerate(ItemStore_ItemDef_t item,
                              ItemStore_Enumerator_t* enumerator);

/// Close an initialized enumerator
///
/// After closing the enumerator, new items may be added to the item store
/// again.
/// @param enumerator Pointer to enumerator object that shall be initialized
void ItemStore_EndEnumerate(ItemStore_Enumerator_t* enumerator);

/// Get the item that was added last to the item store
/// @param item Id of the item store
/// @param data Pointer to a data structure where the data will be copied
/// @return true if the operation succeeded; false otherwise.
bool ItemStore_GetNewestItem(ItemStore_ItemDef_t item,
                             ItemStore_ItemStruct_t* data);

/// Get the number of items that are stored in the specified item store
/// @param enumerator Pointer to an initialized enumerator;
/// @return number of available items in the item store
uint16_t ItemStore_Count(ItemStore_Enumerator_t* enumerator);

/// Access the next item in the item store
/// @param enumerator
/// @param data Pointer to a data structure that can hold the next element
///             retrieved by the enumerator
/// @return true if the operation succeeds; false otherwise
bool ItemStore_GetNext(ItemStore_Enumerator_t* enumerator,
                       ItemStore_ItemStruct_t* data);
#endif  // ITEM_STORE_H
