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
#include "app_service/sensor/Sht4x.h"
#include "utility/scheduler/MessageId.h"

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
  /// flag to indicate that a new sample is ready to be inserted into the item store
  bool isSampleReady;
  /// current sample index
  uint8_t currentSampleIndex;
  /// Samples that will be inserted into the item store as soon as possible
  ItemStore_MeasurementSample_t samples;
} MeasurementItemController_t;

/// State of the controller where inserting new items is possible.
/// @param message The message to be dispatched
/// @return true if the message was handled; false otherwise
static bool ItemStoreIdleState(Message_Message_t* message);

/// Update the moving average for humidity and temperature
/// @param msg message to be processed
static void UpdateMovingAverage(Sht4x_SensorMessage_t* msg);

/// Update the sample value if a full logging interval has elapsed
/// @param msg The message with the amount of elapsed seconds
/// @param canAddItem flag to tell if item can be saved to item store or not
static void EvalTimeEvent(Message_Message_t* msg, bool canAddItem);

/// Add ready samples to the item store
/// @param canAddItem Flag to indicate if item store is ready to save items.
static void SaveReadySamples(bool canAddItem);

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
                            MESSAGE_BROKER_CATEGORY_TIME_INFORMATION};

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
      _measurementItemController.isAddItemPossible = false;
      return true;
    }
    if (msg->header.id == ITEM_STORE_MESSAGE_ERASE_DONE) {
      _measurementItemController.isAddItemPossible = true;
      SaveReadySamples(true);
      return true;
    }
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
