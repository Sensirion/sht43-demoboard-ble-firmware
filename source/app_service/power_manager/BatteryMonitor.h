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

/// @file BatteryMonitor.h
#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include "utility/StaticCodeAnalysisHelper.h"
#include "utility/scheduler/Message.h"
#include "utility/scheduler/MessageListener.h"

/// specifies the power states of the application
typedef enum {
  BATTERY_MONITOR_APP_STATE_UNDEFINED,  ///< no vbat measurement done yet
  BATTERY_MONITOR_APP_STATE_NO_RESTRICTION,
  BATTERY_MONITOR_APP_STATE_REDUCED_OPERATION,
  BATTERY_MONITOR_APP_STATE_CRITICAL_BATTERY_LEVEL,
} BatteryMonitor_AppState_t;

/// Defines the message that is sent by the battery monitor when
/// a state change happens.
typedef struct tBatteryMonitor_Message {
  MessageBroker_MsgHead_t head;             ///< message head
  BatteryMonitor_AppState_t currentState;   ///< application state that is
                                            ///< becoming active
  BatteryMonitor_AppState_t previousState;  ///< previous application state
  uint8_t remainingCapacity;                ///< remaining capacity; this value
                                            ///< in only valid when message id
                                            ///< is NEW_CAPACITY_VALUE
} BatteryMonitor_Message_t;

ASSERT_SIZE_TYPE1_LESS_THAN_TYPE2(Message_Message_t, uint64_t);

/// specifies the message id's of this category
typedef enum {
  BATTERY_MONITOR_MESSAGE_ID_STATE_CHANGE = 1,
  BATTERY_MONITOR_MESSAGE_ID_CAPACITY_CHANGE = 2
} BatteryMonitor_MessageId_t;

/// Create a new instance of the battery monitor. The Battery monitor is
/// itself a message listener.
/// @return the instantiated BatteryMonitor instance;
MessageListener_Listener_t* BatteryMonitor_Instance();

/// Get the battery voltage synchronously.
/// @note: This function will not trigger a new measurement but just return a
/// previously read value.
/// @return the last measured value of the battery voltage in millivolt.
uint32_t BatteryMonitor_GetBatteryVoltage();

#endif  // BATTERY_MONITOR_H
