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

/// @file MeasurementItemController.c
#include "MeasurementItemController.h"

#include "ItemStore.h"
#include "app_service/networking/ble/BleGatt.h"
#include "app_service/networking/ble/BleInterface.h"
#include "app_service/sensor/Sht4x.h"
#include "utility/AppDefines.h"
#include "utility/ErrorHandler.h"
#include "utility/scheduler/MessageId.h"

/// Structure used while serving a data readout request.
typedef struct _tSampleRequestData {
  /// metadata to be sent to ble context
  BleTypes_SamplesMetaData_t metadata;

  /// Start index of the enumerator
  uint16_t enumeratorStartIndex;

  /// Number requested samples
  uint16_t requestedNrOfSamples;

  /// A buffer to hold one page of samples;
  /// Since we cannot add more samples while enumerating, we read page by page
  /// and release the enumerator after each read.
  ItemStore_MeasurementSample_t samplesCache[510];

  /// Data structure that is returned to the ble context
  BleGatt_RequestResponseData_t responseData;
} SampleRequestData_t;

/// Defines the structure of the measurement item controller.
typedef struct _tMeasurementItemController {
  /// Message listener of controller
  MessageListener_Listener_t listener;
  /// Logging interval
  uint32_t loggingIntervalS;
  /// Count-down of logging interval
  int32_t remainingTimeS;
  /// Moving average of humidity;
  /// humidityAverage[t+1] =
  ///       humidityAverage[t]*coefficient[0] + humidity[t]*coefficient[1]
  float humidityAverage;
  /// Moving average of temperature; same as for humidity
  float temperatureAverage;
  /// Averaging coefficients
  /// coefficient[0] + coefficient[1] = 1.0
  float coefficient[2];
  /// flag to indicate if items can be added to the item store
  bool isAddItemPossible;
  /// Count number of pending erases. When we set the logging interval when
  /// a page wrap happens, we might end up in a situation where we have two
  /// erase done messages!
  int8_t nrOfPendingErase;
  /// flag to indicate that a new sample is ready to be inserted into the item store
  bool isSampleReady;
  /// current sample index
  uint8_t currentSampleIndex;
  /// Flag to indicate that the logging interval needs to be saved in the
  /// system settings
  bool isLoggingIntervalChanged;
  /// Samples that will be inserted into the item store as soon as possible
  ItemStore_MeasurementSample_t samples;

} MeasurementItemController_t;

/// State of the controller where inserting new items is possible.
/// @param message The message to be dispatched
/// @return true if the message was handled; false otherwise
static bool ItemStoreIdleState(Message_Message_t* message);

/// Update the moving average for humidity and temperature
/// @param message message to be processed
static void UpdateMovingAverage(Sht4x_SensorMessage_t* message);

/// Update the sample value if a full logging interval has elapsed
/// @param msg The message with the amount of elapsed seconds
/// @param canAddItem flag to tell if item can be saved to item store or not
static void EvalTimeEvent(Message_Message_t* msg, bool canAddItem);

/// Handle the request from a BLE gatt service.
/// The function evaluates the request and sends back the requested information
/// to the BLE context if needed.
/// @param msg Message that contains the information about the request
/// @return true if the message was handled; false otherwise.
static bool HandleBleServiceRequest(Message_Message_t* msg);

/// Add ready samples to the item store
/// @param canAddItem Flag to indicate if item store is ready to save items.
static void SaveReadySamples(bool canAddItem);

/// Enumerator callback to count the number of available samples.
/// @param enumeratorReady Flag that indicates if enumerator is ready to use
static void CountSamples(bool enumeratorReady);

/// Evaluate the number of samples and initialize the sample request structure;
/// @param enumeratorReady
static void BeginReadSamples(bool enumeratorReady);

/// Read a bunch of data into the sample cache and notify the data to the
/// BLE context;
/// @param enumeratorReady
static void ReadMoreSamples(bool enumeratorReady);

/// Structure to hold the state of the sample request
static SampleRequestData_t _sampleRequest;

/// Enumerator to be used to service various requests
static ItemStore_Enumerator_t _sampleEnumerator;

/// Definition of Measurement item controller
MeasurementItemController_t _measurementItemController = {
    .loggingIntervalS = 60,
    .remainingTimeS = 60,
    .currentSampleIndex = 0,
    .isSampleReady = false,
    .isAddItemPossible = true,
    .coefficient = {5.0f / 6.0f, 1.0f / 6.0f},
    .listener.currentMessageHandlerCb = ItemStoreIdleState,
    .listener.receiveMask = MESSAGE_BROKER_CATEGORY_ITEM_STORE |
                            MESSAGE_BROKER_CATEGORY_SENSOR_VALUE |
                            MESSAGE_BROKER_CATEGORY_TIME_INFORMATION |
                            MESSAGE_BROKER_CATEGORY_BLE_SERVICE_REQUEST};

MessageListener_Listener_t* MeasurementItemController_Instance() {
  return &_measurementItemController.listener;
}

static bool ItemStoreIdleState(Message_Message_t* msg) {
  // receive a new measurement value
  if (msg->header.category == MESSAGE_BROKER_CATEGORY_SENSOR_VALUE &&
      msg->header.id == SHT4X_MESSAGE_ID_SENSOR_DATA &&
      msg->header.parameter1 > SHT4X_COMMAND_READ_SERIAL_NUMBER) {
    UpdateMovingAverage((Sht4x_SensorMessage_t*)msg);
    return true;
  }
  if ((msg->header.category == MESSAGE_BROKER_CATEGORY_TIME_INFORMATION) &&
      (msg->header.id == MESSAGE_ID_TIME_INFO_TIME_ELAPSED)) {
    EvalTimeEvent(msg, _measurementItemController.isAddItemPossible);
    return true;
  }
  if (msg->header.category == MESSAGE_BROKER_CATEGORY_ITEM_STORE) {
    if (msg->header.id == ITEM_STORE_MESSAGE_ERASE) {
      _measurementItemController.nrOfPendingErase++;
      _measurementItemController.isAddItemPossible = false;
      return true;
    }
    if (msg->header.id == ITEM_STORE_MESSAGE_ERASE_DONE) {
      _measurementItemController.nrOfPendingErase--;
      // that should actually never happen
      if (_measurementItemController.nrOfPendingErase < 0) {
        _measurementItemController.nrOfPendingErase = 0;
      }
      _measurementItemController.isAddItemPossible =
          (_measurementItemController.nrOfPendingErase == 0);
      SaveReadySamples(true);
      return true;
    }
  }
  if (msg->header.category == MESSAGE_BROKER_CATEGORY_BLE_SERVICE_REQUEST) {
    return HandleBleServiceRequest(msg);
  }
  return false;
}

static void UpdateMovingAverage(Sht4x_SensorMessage_t* msg) {
  _measurementItemController.humidityAverage =
      _measurementItemController.coefficient[0] *
          _measurementItemController.humidityAverage +
      _measurementItemController.coefficient[1] *
          msg->data.measurement.humidityTicks;
  _measurementItemController.temperatureAverage =
      _measurementItemController.coefficient[0] *
          _measurementItemController.temperatureAverage +
      _measurementItemController.coefficient[1] *
          msg->data.measurement.temperatureTicks;
}

static void EvalTimeEvent(Message_Message_t* msg, bool canAddItem) {
  _measurementItemController.remainingTimeS -= msg->header.parameter1;
  if (_measurementItemController.remainingTimeS <= 0) {
    _measurementItemController.remainingTimeS =
        _measurementItemController.loggingIntervalS;
    _measurementItemController.samples
        .sample[_measurementItemController.currentSampleIndex]
        .temperatureTicks =
        (uint16_t)(_measurementItemController.temperatureAverage + 0.5f);
    _measurementItemController.samples
        .sample[_measurementItemController.currentSampleIndex]
        .humidityTicks =
        (uint16_t)(_measurementItemController.humidityAverage + 0.5f);
    _measurementItemController.currentSampleIndex =
        (_measurementItemController.currentSampleIndex + 1) % 2;

    _measurementItemController.isSampleReady =
        // don't clear if already set (should never happen)
        (_measurementItemController.isSampleReady ||
         (_measurementItemController.currentSampleIndex == 0));
  }
  SaveReadySamples(canAddItem);
}

static void SaveReadySamples(bool canAddItem) {
  if (_measurementItemController.isSampleReady && canAddItem) {
    ItemStore_AddItem(
        ITEM_DEF_MEASUREMENT_SAMPLE,
        (ItemStore_ItemStruct_t*)&_measurementItemController.samples);
    _measurementItemController.isSampleReady = false;
  }
}

static bool HandleBleServiceRequest(Message_Message_t* message) {
  if (message->header.id == SERVICE_REQUEST_MESSAGE_ID_GET_LOGGING_INTERVAL) {
    BleInterface_Message_t msg = {
        .head.category = MESSAGE_BROKER_CATEGORY_BLE_EVENT,
        .head.id = BLE_INTERFACE_MSG_ID_SVC_REQ_RESPONSE,
        .head.parameter1 = SERVICE_REQUEST_MESSAGE_ID_GET_LOGGING_INTERVAL,
        .parameter.responseData =
            _measurementItemController.loggingIntervalS * 1000};
    BleInterface_PublishBleMessage((Message_Message_t*)&msg);
    return true;
  }
  if (message->header.id == SERVICE_REQUEST_MESSAGE_ID_SET_LOGGING_INTERVAL) {
    // we allow logging intervals only to be a multiple of 10s
    uint32_t newInterval = message->parameter2 / 10000 * 10;
    if (newInterval < 10) {
      newInterval = 10;
    }
    if (newInterval != _measurementItemController.loggingIntervalS) {
      _measurementItemController.loggingIntervalS = newInterval;
      // accumulate at most over one hour
      float divider = MIN(newInterval, 3600) / 5.0f;
      _measurementItemController.coefficient[1] = 1.0f / divider;
      _measurementItemController.coefficient[0] =
          1.0f - _measurementItemController.coefficient[1];
      ItemStore_DeleteAllItems(ITEM_DEF_MEASUREMENT_SAMPLE);
    }
    return true;
  }
  if (message->header.id == SERVICE_REQUEST_MESSAGE_ID_GET_AVAILABLE_SAMPLES) {
    _sampleEnumerator.startIndex = 0;
    ItemStore_BeginEnumerate(ITEM_DEF_MEASUREMENT_SAMPLE, &_sampleEnumerator,
                             CountSamples);
    return true;
  }

  if (message->header.id == SERVICE_REQUEST_MESSAGE_ID_SET_REQUESTED_SAMPLES) {
    _sampleRequest.requestedNrOfSamples = message->parameter2;
    ItemStore_BeginEnumerate(ITEM_DEF_MEASUREMENT_SAMPLE, &_sampleEnumerator,
                             BeginReadSamples);
    return true;
  }

  if (message->header.id == SERVICE_REQUEST_MESSAGE_ID_GET_NEXT_SAMPLES) {
    // we may use the enumerator for other purposes; hence the start index
    // is always reinitialized.
    _sampleEnumerator.startIndex = _sampleRequest.enumeratorStartIndex;
    ItemStore_BeginEnumerate(ITEM_DEF_MEASUREMENT_SAMPLE, &_sampleEnumerator,
                             ReadMoreSamples);
    return true;
  }

  return false;
}

static void CountSamples(bool enumeratorReady) {
  BleInterface_Message_t msg = {
      .head.category = MESSAGE_BROKER_CATEGORY_BLE_EVENT,
      .head.id = BLE_INTERFACE_MSG_ID_SVC_REQ_RESPONSE,
      .head.parameter1 = SERVICE_REQUEST_MESSAGE_ID_GET_AVAILABLE_SAMPLES,
      .parameter.responseData = 0};

  if (!enumeratorReady) {
    BleInterface_PublishBleMessage((Message_Message_t*)&msg);
    return;
  }
  msg.parameter.responseData = ItemStore_Count(&_sampleEnumerator) * 2;
  BleInterface_PublishBleMessage((Message_Message_t*)&msg);
  ItemStore_EndEnumerate(&_sampleEnumerator);
}

static void BeginReadSamples(bool enumeratorReady) {
  BleInterface_Message_t msg = {
      .head.category = MESSAGE_BROKER_CATEGORY_BLE_EVENT,
      .head.id = BLE_INTERFACE_MSG_ID_SVC_REQ_RESPONSE,
      .head.parameter1 = SERVICE_REQUEST_MESSAGE_ID_SET_REQUESTED_SAMPLES,
      .parameter.responsePtr = 0};

  if (!enumeratorReady) {
    ErrorHandler_RecoverableError(ERROR_CODE_ITEM_STORE);
    return;
  }
  uint16_t availableSamples = ItemStore_Count(&_sampleEnumerator) * 2;
  ItemStore_EndEnumerate(&_sampleEnumerator);
  _sampleRequest.metadata.numberOfSamples =
      MIN(availableSamples, _sampleRequest.requestedNrOfSamples);
  msg.parameter.responsePtr = &_sampleRequest.metadata;
  _sampleRequest.metadata.loggingIntervalMs =
      _measurementItemController.loggingIntervalS * 1000;

  // compute where we need to start reading from.
  _sampleRequest.enumeratorStartIndex = 0;
  if (availableSamples > _sampleRequest.requestedNrOfSamples) {
    _sampleRequest.enumeratorStartIndex =
        availableSamples - _sampleRequest.requestedNrOfSamples;
  }
  // compute the age of the last sample in flash
  _sampleRequest.metadata.ageOfLatestSample =
      ((_measurementItemController.loggingIntervalS -
        _measurementItemController.remainingTimeS) +
       _measurementItemController.currentSampleIndex *
           _measurementItemController.loggingIntervalS) *
      1000;
  // now that all information is assembled, send it back to the ble context
  BleInterface_PublishBleMessage((Message_Message_t*)&msg);
}

static void ReadMoreSamples(bool enumeratorReady) {
  BleInterface_Message_t msg = {
      .head.category = MESSAGE_BROKER_CATEGORY_BLE_EVENT,
      .head.id = BLE_INTERFACE_MSG_ID_SVC_REQ_RESPONSE,
      .head.parameter1 = SERVICE_REQUEST_MESSAGE_ID_GET_NEXT_SAMPLES,
      .parameter.responsePtr = 0};

  if (!enumeratorReady) {
    ErrorHandler_RecoverableError(ERROR_CODE_ITEM_STORE);
    return;
  }
  uint16_t index = 0;
  while (_sampleEnumerator.hasMoreItems &&
         index < COUNT_OF(_sampleRequest.samplesCache) &&
         index < (_sampleRequest.metadata.numberOfSamples / 2)) {
    ItemStore_GetNext(
        &_sampleEnumerator,
        (ItemStore_ItemStruct_t*)&_sampleRequest.samplesCache[index++]);
  }
  ItemStore_EndEnumerate(&_sampleEnumerator);
  _sampleRequest.enumeratorStartIndex += index;
  _sampleRequest.responseData.dataLength =
      index * sizeof(ItemStore_MeasurementSample_t);
  _sampleRequest.responseData.data = (uint8_t*)_sampleRequest.samplesCache;
  msg.parameter.responsePtr = &_sampleRequest.responseData;
  BleInterface_PublishBleMessage((Message_Message_t*)&msg);
}
