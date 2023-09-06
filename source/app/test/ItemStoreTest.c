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

/// @file ItemStoreTest.c
#include "ItemStoreTest.h"

#include "app/Presentation.h"
#include "app_service/item_store/ItemStore.h"
#include "app_service/timer_server/TimerServer.h"
#include "utility/log/Trace.h"

/// Parameter of the timerAddItem test
static SysTest_TestMessageParameter_t _timerAddItemParameter;

/// Id of the timer that is used to add items to the item store
static uint8_t _timerAddItemId;

/// parameter for test enumerator function
static uint16_t _numberOfItemsToRead;

/// Called when the add-item timer is elapsed
static void OnTimerElapsed();

/// Default item values for testing
ItemStore_ItemStruct_t _testItemData[] = {
    [0] = {.configuration.debug = true,
           .configuration.deviceName = "test demo board name",
           .configuration.loggingInterval = 5000},
    [1] = {.measurement.sample = {{0xABCD, 0x0123}, {0x4567, 0x89AB}}}};

/// Memory buffer to receive the data from the enumerator
ItemStore_ItemStruct_t _testItemBuffer;

/// Enumerator to read data from the flash
ItemStore_Enumerator_t _enumerator;

/// Callback function used as parameter for ItemStoreTest_BeginEnumerate
/// @param status of the operation BeginEnumerate
static void EnumeratorReadToEnd(bool status);

/// Callback function used as parameter for ItemStoreTest_BeginEnumerate
/// @param status of the operation BeginEnumerate
static void EnumeratorReadCount(bool status);

void ItemStoreTest_AddItem(SysTest_TestMessageParameter_t param) {
  // avoid overflow of message queue
  Presentation_setTimeStep(240);
  for (int i = 0; i < param.byteParameter[1]; i++) {
    ItemStore_AddItem(param.byteParameter[0],
                      &_testItemData[param.byteParameter[0]]);
  }
}

void ItemStoreTest_TimerAddItem(SysTest_TestMessageParameter_t param) {
  _timerAddItemParameter = param;
  _timerAddItemId =
      TimerServer_CreateTimer(TIMER_SERVER_MODE_REPEATED, OnTimerElapsed);
  TimerServer_Start(_timerAddItemId, 200);
}

void ItemStoreTest_EnumerateItems(SysTest_TestMessageParameter_t param) {
  ItemStore_EnumeratorStatusCb_t callback = EnumeratorReadCount;
  _enumerator.startIndex = 0;
  _numberOfItemsToRead = param.shortParameter[1];
  if (param.byteParameter[1] == 1) {
    _enumerator.startIndex = (int16_t)param.shortParameter[1];
    _numberOfItemsToRead = 0;
    callback = EnumeratorReadToEnd;
  }
  ItemStore_BeginEnumerate(param.byteParameter[0], &_enumerator, callback);
}

static void OnTimerElapsed() {
  if (_timerAddItemParameter.shortParameter[1] == 0) {
    TimerServer_Stop(_timerAddItemId);
    TimerServer_DeleteTimer(_timerAddItemId);
    return;
  }
  ItemStore_AddItem(_timerAddItemParameter.byteParameter[0],
                    &_testItemData[_timerAddItemParameter.byteParameter[0]]);
  _timerAddItemParameter.shortParameter[1]--;
}

static void EnumeratorReadToEnd(bool status) {
  if (!status) {
    Trace_Message("Enumerator was not initialized properly!");
    ItemStore_EndEnumerate(&_enumerator);
    return;
  }
  while (_enumerator.hasMoreItems) {
    ItemStore_GetNext(&_enumerator, &_testItemBuffer);
    _numberOfItemsToRead++;
    Trace_Message("read items = %i\n", _numberOfItemsToRead);
  }
  Trace_Message("\n=>read to end done: read items = %i\n",
                _numberOfItemsToRead);
  ItemStore_EndEnumerate(&_enumerator);
}

static void EnumeratorReadCount(bool status) {
  if (!status) {
    Trace_Message("Enumerator was not initialized properly!\n");
    ItemStore_EndEnumerate(&_enumerator);
    return;
  }
  uint16_t readItems = 0;
  while (_enumerator.hasMoreItems && readItems < _numberOfItemsToRead) {
    ItemStore_GetNext(&_enumerator, &_testItemBuffer);
    readItems++;
    Trace_Message("read item; remaining = %i\n",
                  _numberOfItemsToRead - readItems);
  }
  Trace_Message("\n=>read count from 0 done: read items = %i\n",
                _numberOfItemsToRead);
  ItemStore_EndEnumerate(&_enumerator);
}
