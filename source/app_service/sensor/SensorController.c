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
#include "hal/I2c3.h"
#include "math.h"
#include "stm32_lpm.h"
#include "stm32wbxx_hal.h"
#include "utility/ErrorHandler.h"
#include "utility/log/Trace.h"
#include "utility/scheduler/MessageId.h"

/// Defines the maximum error retries
#define MAX_CONSECUTIVE_ERRORS 3

/// Helper macro for information logging
#define LOG_INFO(...) Trace_Message(__VA_ARGS__)

/// Handles messages in idle state
/// @param msg received message
/// @return true if the message was handled, false otherwise
static bool IdleStateCb(Message_Message_t* msg);

/// Handles messages in RequestStarted state
/// @param msg received message
/// @return true if the message was handled, false otherwise
static bool ShtRequestStartedStateCb(Message_Message_t* msg);

/// Handles messages in RequestReading state
/// @param msg received message
/// @return true if the message was handled, false otherwise
static bool ShtRequestReadingStateCb(Message_Message_t* msg);

/// Handles an incoming error
/// @param errorCode specifies the error code that might be propagated
static void HandleError(uint32_t errorCode);

/// State machine instance of sensor controller
static SensorController_Controller_t _sht4xController = {
    .consecutiveErrors = 0,
    .listener.currentMessageHandlerCb = IdleStateCb,
    .listener.receiveMask = MESSAGE_BROKER_CATEGORY_SENSOR_VALUE |
                            MESSAGE_BROKER_CATEGORY_TIME_INFORMATION};

SensorController_Controller_t* SensorController_Sht4xControllerInstance() {
  return &_sht4xController;
}

static bool IdleStateCb(Message_Message_t* msg) {
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
  if (msg->header.category == MESSAGE_BROKER_CATEGORY_RECOVERABLE_ERROR) {
    HandleError(msg->parameter2);
    return true;
  }
  return false;
}

static bool ShtRequestReadingStateCb(Message_Message_t* msg) {
  if (msg->header.category == MESSAGE_BROKER_CATEGORY_SENSOR_VALUE) {
    if (msg->header.id == SHT4X_MESSAGE_ID_SENSOR_DATA) {
      Sht4x_SensorMessage_t* sensorMsg = (Sht4x_SensorMessage_t*)msg;
      if (sensorMsg->head.parameter1 == SHT4X_COMMAND_READ_SERIAL_NUMBER) {
        _sht4xController.serialNumber = sensorMsg->data.serialNumer;
        return true;
      } else {
        _sht4xController.temperatureTicks =
            sensorMsg->data.measurement.temperatureTicks;
        _sht4xController.humidityTicks =
            sensorMsg->data.measurement.humidityTicks;
      }
      _sht4xController.listener.currentMessageHandlerCb = IdleStateCb;
      _sht4xController.consecutiveErrors = 0;
      return true;
    }
    if (msg->header.id == SHT4X_MESSAGE_ID_ERROR) {
      HandleError(ERROR_CODE_SENSOR_READOUT);
      return true;
    }
  }
  if (msg->header.category == MESSAGE_BROKER_CATEGORY_RECOVERABLE_ERROR) {
    HandleError(msg->parameter2);
    return true;
  }
  return false;
}

static void HandleError(uint32_t errorCode) {
  _sht4xController.consecutiveErrors += 1;
  if (_sht4xController.consecutiveErrors > MAX_CONSECUTIVE_ERRORS) {
    // blocking call
    ErrorHandler_UnrecoverableError(errorCode);
  }
  // just ignore the error and try in the next readout
  _sht4xController.listener.currentMessageHandlerCb = IdleStateCb;
}
