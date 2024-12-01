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

/// @file MessageId.h
///
/// Defines the message ids for each category that is not clearly associated
/// to a specific module.
/// The message id's of the following categories are defined in another header:
/// - Category SensorValue: Sht4x.h
/// - Category ButtonEvent: Button.h
/// - Category Test: SysTest.h
///
/// No messages defined for the category SystemStateChange so far.

#ifndef MESSAGE_ID_H
#define MESSAGE_ID_H

/// Defines the message id of the category TimeInformation
typedef enum { MESSAGE_ID_TIME_INFO_TIME_ELAPSED = 1 } MessageId_TimeInfo_t;

/// Defines the message id for system state change category
typedef enum {
  MESSAGE_ID_STATE_CHANGE_ERROR = 1,
  MESSAGE_ID_PERIPHERALS_INITIALIZED = 2,
  MESSAGE_ID_READOUT_INTERVAL_CHANGE = 3,
  MESSAGE_ID_BLE_SUBSYSTEM_READY = 4,
  MESSAGE_ID_DEVICE_SETTINGS_READ = 5,
  MESSAGE_ID_DEVICE_SETTINGS_CHANGED = 6,
  MESSAGE_ID_BLE_SUBSYSTEM_OFF = 7,
  MESSAGE_ID_BLE_SUBSYSTEM_ON = 8,
} MessageId_StateChange_t;

#endif  // MESSAGE_ID_H
