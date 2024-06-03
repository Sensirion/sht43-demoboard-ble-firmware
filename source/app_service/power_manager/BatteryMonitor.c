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

/// @file BatteryMonitor.c
#include "BatteryMonitor.h"

#include "hal/Adc.h"
#include "utility/AppDefines.h"
#include "utility/scheduler/MessageId.h"

#include <math.h>
#include <stdbool.h>

/// slope of the battery level curve in the range 100% - 25%
#define BATTERY_LEVEL_SLOPE_1 0.3f

/// offset of the battery level curve in the range 100% - 25%
#define BATTERY_LEVEL_OFFSET_1 -800

/// slope of the battery level curve in the range 25% - 5%
#define BATTERY_LEVEL_SLOPE_2 0.08f

/// offset of the battery level curve in the range 25% - 5%
#define BATTERY_LEVEL_OFFSET_2 -190

/// Constants to define the threshold when the
/// application state changes.
/// After comparing with various spec sheets and discharge curves
/// the values below are set and mapped to a remaining capacity
/// The battery service will compute its capacity from this curve.
///
/// There are three different slopes (simplified):
/// * 3V+ - 2.75V: No restriction (100% - 25% of capacity)
/// * 2.75 V - 2.5V: No connection possible anymore and battery low symbol
///   will be displayed (25% - 5% capacity)
/// * 2.5 - 2.2: No data logging and no advertisement, battery low symbol is
///   blinking and draining the battery empty. Below 2.2V the gadget will be
///   switched off. (5% - 0% capacity)
typedef enum {
  BATTERY_THRESHOLD_NO_RESTRICTION = 2750,       ///< ~25% remaining capacity
  BATTERY_THRESHOLD_RESTRICTED_OPERATION = 2500  ///< ~5% remaining
} BatteryThreshold_t;

/// Type definition for the BatteryMonitor
typedef struct tBatteryMonitor {
  MessageListener_Listener_t listener;  ///< listener to receive the time tick
  uint32_t batteryLevelMV;              ///< battery level in millivolt
  uint8_t remainingCapacity;            ///< battery level in percent
  BatteryMonitor_AppState_t actualApplicationState;  ///< actual application
                                                     ///< state
} BatteryMonitor_t;

/// defines the depth of the measurement history
#define HISTORY_DEPTH 4

/// A buffer to keep the last `HISTORY_DEPTH` measurements
uint32_t _batteryVoltageMeasurements[HISTORY_DEPTH] = {0};

/// insert index for batteryLevelMeasurements buffer
uint8_t _bufferIndex = 0;

/// flag to tell if the measurement buffer was filled once
uint8_t _historyComplete = false;

/// A buffer to keep the last `HISTORY_DEPTH` measurements is sorted order
uint32_t _batteryVoltageMeasurementsSorted[HISTORY_DEPTH];

/// Compute the sorted measurement values and store them in
/// the _batteryVoltageMeasurementsSorted.
static void CopyAndSortMeasurements();

/// Get the median of the measured voltages in the measurement history
/// @return the median voltage value
static uint32_t GetMeasuredVoltagesMedianMv();

/// Compute the remaining capacity of the battery
/// @param batteryLevelMv battery level in millivolt
/// @return remaining capacity in percent
static uint8_t ComputeRemainingCapacity(uint32_t batteryLevelMv);

/// The message handler of the BatteryMonitor.
/// The BatteryMonitor only listens to the time tick of the application. It has
/// no further state behavior.
/// @param message the received message
/// @return true if the message was handled; false otherwise
static bool MessageHandlerCb(Message_Message_t* message);

/// Callback to receive vbat measurement value, for the first time
/// this will initialize the batteryLevel and application state of
/// the BatteryMonitor but not trigger an update.
/// @param vbatMv measurement value of vbat in millivolt
static void InitializeVbatCb(uint32_t vbatMv);

/// Callback to receive vbat measurement value.
/// This will update the vbat and trigger an event changed message in case
/// the battery level was changed.
/// @param vbatMv measurement value of vbat in millivolt
static void UpdateVbatCb(uint32_t vbatMv);

/// Compute the application state from a measured battery voltage
/// @param vbatMv measured battery voltage
/// @return the computed application state
static BatteryMonitor_AppState_t VbatToApplicationState(uint32_t vbatMv);

/// The only instance of the battery monitor
BatteryMonitor_t _batteryMonitorInstance = {
    .listener = {.currentMessageHandlerCb = MessageHandlerCb,
                 .receiveMask = MESSAGE_BROKER_CATEGORY_TIME_INFORMATION},
    .remainingCapacity = 0,
    .actualApplicationState = BATTERY_MONITOR_APP_STATE_UNDEFINED};

/// Create a new instance of the battery monitor. The Battery monitor is
/// itself a message listener.
/// @return the instantiated BatteryMonitor instance;
MessageListener_Listener_t* BatteryMonitor_Instance() {
  static bool initialized = false;
  if (initialized) {
    return (MessageListener_Listener_t*)&_batteryMonitorInstance;
  }
  Adc_MeasureVbat(InitializeVbatCb);
  initialized = true;
  return (MessageListener_Listener_t*)&_batteryMonitorInstance;
}

static void InitializeVbatCb(uint32_t vbatMv) {
  // Initializes the filter and insert the first measurement.
  _bufferIndex = 0;
  _historyComplete = false;
  _batteryVoltageMeasurements[_bufferIndex++] = vbatMv;
}

static void UpdateVbatCb(uint32_t vbatMv) {
  _batteryVoltageMeasurements[_bufferIndex++] = vbatMv;
  if (_bufferIndex == HISTORY_DEPTH) {
    _bufferIndex = 0;
    _historyComplete = true;
  }
  // as long as we do not have enough measurements, we do not update
  // the application state
  if (!_historyComplete) {
    return;
  }

  _batteryMonitorInstance.batteryLevelMV = GetMeasuredVoltagesMedianMv();

  BatteryMonitor_AppState_t state =
      _batteryMonitorInstance.actualApplicationState;
  _batteryMonitorInstance.actualApplicationState =
      VbatToApplicationState(_batteryMonitorInstance.batteryLevelMV);

  uint8_t remainingCapacity =
      ComputeRemainingCapacity(_batteryMonitorInstance.batteryLevelMV);

  if (state != _batteryMonitorInstance.actualApplicationState) {
    BatteryMonitor_Message_t msg = {
        .currentState = _batteryMonitorInstance.actualApplicationState,
        .previousState = state,
        .head = {.category = MESSAGE_BROKER_CATEGORY_BATTERY_EVENT,
                 .id = BATTERY_MONITOR_MESSAGE_ID_STATE_CHANGE}};
    Message_PublishAppMessage((Message_Message_t*)&msg);
  }
  if (remainingCapacity != _batteryMonitorInstance.remainingCapacity) {
    _batteryMonitorInstance.remainingCapacity = remainingCapacity;
    BatteryMonitor_Message_t msg = {
        .remainingCapacity = remainingCapacity,
        .head = {.category = MESSAGE_BROKER_CATEGORY_BATTERY_EVENT,
                 .id = BATTERY_MONITOR_MESSAGE_ID_CAPACITY_CHANGE}};
    Message_PublishAppMessage((Message_Message_t*)&msg);
  }
}

static BatteryMonitor_AppState_t VbatToApplicationState(uint32_t vbatMv) {
  if (vbatMv > BATTERY_THRESHOLD_NO_RESTRICTION) {
    return BATTERY_MONITOR_APP_STATE_NO_RESTRICTION;
  }
  if (vbatMv > BATTERY_THRESHOLD_RESTRICTED_OPERATION) {
    return BATTERY_MONITOR_APP_STATE_REDUCED_OPERATION;
  }
  return BATTERY_MONITOR_APP_STATE_CRITICAL_BATTERY_LEVEL;
}

static bool MessageHandlerCb(Message_Message_t* message) {
  if (message->header.category == MESSAGE_BROKER_CATEGORY_TIME_INFORMATION &&
      message->header.id == MESSAGE_ID_TIME_INFO_TIME_ELAPSED) {
    Adc_MeasureVbat(UpdateVbatCb);
    return true;
  }
  return false;
}

static uint8_t ComputeRemainingCapacity(uint32_t batteryLevelMv) {
  if (_batteryMonitorInstance.actualApplicationState ==
      BATTERY_MONITOR_APP_STATE_NO_RESTRICTION) {
    return (uint8_t)fminf(100, roundf(batteryLevelMv * BATTERY_LEVEL_SLOPE_1 +
                                      BATTERY_LEVEL_OFFSET_1));
  }

  return (uint8_t)fmaxf(
      0, fminf(25, roundf(batteryLevelMv * BATTERY_LEVEL_SLOPE_2 +
                          BATTERY_LEVEL_OFFSET_2)));
}

void CopyAndSortMeasurements() {
  for (uint8_t i = 0; i < HISTORY_DEPTH; i++) {
    _batteryVoltageMeasurementsSorted[i] = _batteryVoltageMeasurements[i];
  }
  // as we have a very short array a bubble sort is good enough
  for (uint8_t i = 0; i < HISTORY_DEPTH; i++) {
    for (uint8_t j = 0; j < HISTORY_DEPTH; j++) {
      if (_batteryVoltageMeasurementsSorted[i] <
          _batteryVoltageMeasurementsSorted[j]) {
        // swap entries
        uint32_t tmp = _batteryVoltageMeasurementsSorted[i];
        _batteryVoltageMeasurementsSorted[i] =
            _batteryVoltageMeasurementsSorted[j];
        _batteryVoltageMeasurementsSorted[j] = tmp;
      }
    }
  }
}

uint32_t GetMeasuredVoltagesMedianMv() {
  // computing a median consist of

  // - sorting
  CopyAndSortMeasurements();

  // - determine the element(s) in the middle
  uint8_t index1 = HISTORY_DEPTH / 2;
  uint8_t index2 = (HISTORY_DEPTH % 2) == 0 ? index1 + 1 : index1;

  // - compute the mean of the middle elements
  return (_batteryVoltageMeasurementsSorted[index1] +
          _batteryVoltageMeasurementsSorted[index2]) /
         2;
}
