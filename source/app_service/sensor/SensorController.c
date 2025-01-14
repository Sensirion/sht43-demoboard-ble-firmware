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

/// @file
///
/// The sensor controller orchestrates the read and write commands to the SHT
/// sensor.

#include "SensorController.h"

#include "Sht4x.h"
#include "app_conf.h"
#include "app_service/timer_server/TimerServer.h"
#include "hal/Crc.h"
#include "hal/I2c3.h"
#include "math.h"
#include "stm32_lpm.h"
#include "stm32wbxx_hal.h"
#include "utility/ErrorHandler.h"
#include "utility/log/Log.h"
#include "utility/scheduler/MessageId.h"

#include <string.h>

/// Defines the maximum number of consecutive i2c errors before
/// the device enters an error state that requires a reset.
#define MAX_CONSECUTIVE_ERRORS 3

/// we allow only for one active reminder!
Message_Message_t _reminder;

/// Handles messages in idle state
/// @param msg received message
/// @return true if the message was handled, false otherwise
static bool IdleStateCb(Message_Message_t* msg);

/// Handles messages in RequestStarted state
/// @param msg received message
/// @return true if the message was handled, false otherwise
static bool ShtRequestStartedStateCb(Message_Message_t* msg);

/// Handles messages in ShtRequestRestartedCb state
///
/// In contrast to the `RequestStarted` state, the readout will be triggered
/// from the main system event and not from the timer event SENSOR_READY.
/// Therefore, it is also not necessary to spawn a timer when the write
/// operation was acknowledged.
///
/// @param msg received message
/// @return true if the message was handled, false otherwise
static bool ShtRequestRestartedCb(Message_Message_t* msg);

/// Handles messages in RequestReading state
/// @param msg received message
/// @return true if the message was handled, false otherwise
static bool ShtRequestReadingStateCb(Message_Message_t* msg);

/// Try to recover from successive errors
/// @param msg received message
/// @return true if the message was handled, false otherwise
static bool ShtErrorHandlerCb(Message_Message_t* msg);

/// In case we receive a request that can not be handled right now,
/// we may set a reminder to handle it later on.
///
/// @param msg received message
static void SetReminderIfNeeded(Message_Message_t* msg);

/// Set idle state and in process a possible pending request
static void SetIdleState();

/// Notify that a general call reset was sent
static void GeneralCallResetSentCb();

/// Handles an incoming error
/// @param errorCode specifies the error code that might be propagated
static void HandleError(uint32_t errorCode);

/// Reset timer
///
/// After a general call reset was successfully issued this timer is started
/// and the system only resumes when the timer has elapsed
static uint8_t _resetTimer;

/// State machine instance of sensor controller
static SensorController_Controller_t _sht4xController = {
    .consecutiveErrors = 0,
    .listener.currentMessageHandlerCb = IdleStateCb,
    .listener.receiveMask = MESSAGE_BROKER_CATEGORY_SYSTEM_STATE_CHANGE |
                            MESSAGE_BROKER_CATEGORY_SENSOR_VALUE |
                            MESSAGE_BROKER_CATEGORY_TIME_INFORMATION};

SensorController_Controller_t* SensorController_Sht4xControllerInstance() {
  _resetTimer =
      TimerServer_CreateTimer(TIMER_SERVER_MODE_SINGLE_SHOT, SetIdleState);
  return &_sht4xController;
}

static bool IdleStateCb(Message_Message_t* msg) {
  if (msg->header.category == MESSAGE_BROKER_CATEGORY_SYSTEM_STATE_CHANGE &&
      msg->header.id == MESSAGE_ID_BLE_SUBSYSTEM_READY) {
    Sht4x_StartRequest(SHT4X_COMMAND_READ_SERIAL_NUMBER);
    _sht4xController.listener.currentMessageHandlerCb =
        ShtRequestStartedStateCb;
    return true;
  }
  if (msg->header.category == MESSAGE_BROKER_CATEGORY_TIME_INFORMATION &&
      msg->header.id == MESSAGE_ID_TIME_INFO_TIME_ELAPSED) {
    Sht4x_StartRequest(SHT4X_COMMAND_HIGH_REPEATABILITY_MEASUREMENT);
    _sht4xController.listener.currentMessageHandlerCb =
        ShtRequestStartedStateCb;

    return true;
  }
  return false;
}

static bool ShtRequestStartedStateCb(Message_Message_t* msg) {
  if (msg->header.category == MESSAGE_BROKER_CATEGORY_SENSOR_VALUE) {
    if (msg->header.id == SHT4X_MESSAGE_ID_REQUEST_SENT) {
      // A successful i2c write took place. Hence we reset
      // the counter for successive i2c errors
      _sht4xController.consecutiveErrors = 0;
      Sht4x_NotifySensorReady();
      return true;
    }
    if (msg->header.id == SHT4X_MESSAGE_ID_SENSOR_READY) {
      Sht4x_ReadRequestData();
      _sht4xController.listener.currentMessageHandlerCb =
          ShtRequestReadingStateCb;
      return true;
    }
  }
  // if we get this message it means that we missed an error indication.
  if (msg->header.category == MESSAGE_BROKER_CATEGORY_TIME_INFORMATION) {
    HandleError(ERROR_CODE_SENSOR_READOUT);
    return true;
  }
  if (msg->header.category == MESSAGE_BROKER_CATEGORY_RECOVERABLE_ERROR) {
    HandleError(msg->parameter2);
    return true;
  }
  SetReminderIfNeeded(msg);
  return false;
}

static bool ShtRequestRestartedCb(Message_Message_t* msg) {
  if (msg->header.category == MESSAGE_BROKER_CATEGORY_TIME_INFORMATION) {
    if (msg->header.id == MESSAGE_ID_TIME_INFO_TIME_ELAPSED) {
      Sht4x_ReadRequestData();
      _sht4xController.listener.currentMessageHandlerCb =
          ShtRequestReadingStateCb;
      return true;
    }
  }
  if (msg->header.category == MESSAGE_BROKER_CATEGORY_RECOVERABLE_ERROR) {
    HandleError(msg->parameter2);
    return true;
  }
  SetReminderIfNeeded(msg);
  return false;
}

static bool ShtRequestReadingStateCb(Message_Message_t* msg) {
  if (msg->header.category == MESSAGE_BROKER_CATEGORY_SENSOR_VALUE) {
    if (msg->header.id == SHT4X_MESSAGE_ID_SENSOR_DATA) {
      _sht4xController.consecutiveErrors = 0;
      Sht4x_StartRequest(SHT4X_COMMAND_HIGH_REPEATABILITY_MEASUREMENT);
      _sht4xController.listener.currentMessageHandlerCb = ShtRequestRestartedCb;

      return true;
    }
    if (msg->header.id == SHT4X_MESSAGE_ID_ERROR) {
      HandleError(ERROR_CODE_SENSOR_READOUT);
      return true;
    }
  }
  // if we get this message it means that we missed an error indication.
  if (msg->header.category == MESSAGE_BROKER_CATEGORY_TIME_INFORMATION) {
    HandleError(ERROR_CODE_SENSOR_READOUT);
    return true;
  }
  if (msg->header.category == MESSAGE_BROKER_CATEGORY_RECOVERABLE_ERROR) {
    HandleError(msg->parameter2);
    return true;
  }
  SetReminderIfNeeded(msg);
  return false;
}

static bool ShtErrorHandlerCb(Message_Message_t* msg) {
  if (msg->header.category == MESSAGE_BROKER_CATEGORY_SYSTEM_STATE_CHANGE) {
    if (msg->header.id == MESSAGE_ID_GENERAL_CALL_RESET) {
      _sht4xController.consecutiveErrors = 0;
      // this will switch to idle state after 30 ms
      TimerServer_Start(_resetTimer, 30);
      return true;
    }
  }
  // if we get this message it means that we missed an error indication.
  if (msg->header.category == MESSAGE_BROKER_CATEGORY_TIME_INFORMATION) {
    ErrorHandler_UnrecoverableError(ERROR_CODE_SENSOR_READOUT);
    return true;
  }
  if (msg->header.category == MESSAGE_BROKER_CATEGORY_RECOVERABLE_ERROR) {
    ErrorHandler_UnrecoverableError(ERROR_CODE_SENSOR_READOUT);
    return true;
  }
  return false;
}

static void HandleError(uint32_t errorCode) {
  // make sure that no history is pending
  _sht4xController.activeReminder = false;
  // reset the i2c block
  I2c3_Release(true);
  _sht4xController.consecutiveErrors += 1;
  if (_sht4xController.consecutiveErrors >= MAX_CONSECUTIVE_ERRORS) {
    static uint8_t _reset[] = {0x06};
    _sht4xController.listener.currentMessageHandlerCb = ShtErrorHandlerCb;
    I2c3_Write(0x00, _reset, 1, GeneralCallResetSentCb);
    return;
  }
  // just ignore the error and try in the next readout
  SetIdleState();
}

static void SetReminderIfNeeded(Message_Message_t* msg) {
  if (msg->header.category == MESSAGE_BROKER_CATEGORY_SYSTEM_STATE_CHANGE &&
      msg->header.id == MESSAGE_ID_BLE_SUBSYSTEM_READY) {
    _sht4xController.activeReminder = true;
    memcpy(&_reminder, msg, sizeof(_reminder));
  }
}

static void GeneralCallResetSentCb() {
  Message_Message_t _resetMessage = {
      .header.category = MESSAGE_BROKER_CATEGORY_SYSTEM_STATE_CHANGE,
      .header.id = MESSAGE_ID_GENERAL_CALL_RESET,
      .header.parameter1 = 0xFF,  // invalid data
      .parameter2 = 0xFFFFFFFF    // not defined at this point in time
  };
  Message_PublishAppMessage(&_resetMessage);
}

static void SetIdleState() {
  Crc_Disable();
  _sht4xController.listener.currentMessageHandlerCb = IdleStateCb;
  if (_sht4xController.activeReminder) {
    _sht4xController.listener.currentMessageHandlerCb(&_reminder);
    _sht4xController.activeReminder = false;
  }
}
