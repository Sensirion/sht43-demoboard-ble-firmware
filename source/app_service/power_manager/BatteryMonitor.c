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
#include "utility/scheduler/MessageId.h"

#include <stdbool.h>

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
  BatteryMonitor_AppState_t actualApplicationState;  ///< actual application
                                                     ///< state
} BatteryMonitor_t;

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
    .batteryLevelMV = 0,
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

uint32_t BatteryMonitor_GetBatteryVoltage() {
  return _batteryMonitorInstance.batteryLevelMV;
}

static void InitializeVbatCb(uint32_t vbatMv) {
  // at this point we initialize only vbat. The battery state
  // is updated only on the first timer tick and it will be published to
  // all listeners as it is a state change
  _batteryMonitorInstance.batteryLevelMV = vbatMv;
}

static void UpdateVbatCb(uint32_t vbatMv) {
  _batteryMonitorInstance.batteryLevelMV = vbatMv;
  BatteryMonitor_AppState_t state =
      _batteryMonitorInstance.actualApplicationState;
  _batteryMonitorInstance.actualApplicationState =
      VbatToApplicationState(vbatMv);
  if (state != _batteryMonitorInstance.actualApplicationState) {
    BatteryMonitor_Message_t msg = {
        .currentState = _batteryMonitorInstance.actualApplicationState,
        .previousState = state,
        .head = {.category = MESSAGE_BROKER_CATEGORY_BATTERY_EVENT,
                 .id = BATTERY_MONITOR_MESSAGE_ID_STATE_CHANGE}};
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
