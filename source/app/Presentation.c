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

/// @file Presentation.c
///
/// Implementation of presentation controller
///

#include "Presentation.h"

#include "app_service/item_store/ItemStore.h"
#include "app_service/networking/ble/BleGatt.h"
#include "app_service/networking/ble/BleInterface.h"
#include "app_service/nvm/ProductionParameters.h"
#include "app_service/power_manager/BatteryMonitor.h"
#include "app_service/screen/Screen.h"
#include "app_service/sensor/Sht4x.h"
#include "app_service/timer_server/TimerServer.h"
#include "app_service/user_button/Button.h"
#include "hal/Uart.h"
#include "stm32wbxx_ll_cortex.h"
#include "utility/AppDefines.h"
#include "utility/ErrorHandler.h"
#include "utility/log/Log.h"
#include "utility/scheduler/Message.h"
#include "utility/scheduler/MessageId.h"

#include <stdbool.h>
#include <string.h>

/// Battery State evaluation macro
#define SHOW_BATTERY_SYMBOL(x)                             \
  (((x) == BATTERY_MONITOR_APP_STATE_REDUCED_OPERATION) || \
   ((x) == BATTERY_MONITOR_APP_STATE_CRITICAL_BATTERY_LEVEL))
/// Defines the timeout in seconds when the pairing is aborted
#define PAIRING_TIMEOUT_S 30
/// Timer ID sensor readout trigger timer
static uint8_t _sht4xReadoutTimer;

/// The time in seconds between two successive TIME_ELAPSES messages
/// Initially this delta is set to one seconds.
static uint8_t _timeStepDeltaSeconds = 1;

/// The presentation controller is the state-machine that is
/// responsible for the proper screen layout and logging formats depending
/// on the state of the application and the device.
typedef struct _tPresentation_Controller {
  MessageListener_Listener_t listener;     ///< listens to the message bus
  BatteryMonitor_AppState_t batteryState;  ///< application state of battery
  bool lowBatterySymbolOn;                 ///< Flag to indicate if low battery
                                           ///< Symbol is displayed on screen.
  bool bleOn;                              ///< display BLE symbol on screen
  uint8_t blinkTimer;                      ///< Id of battery symbol blink
                                           ///< timer.
  float temperatureC;                      ///< Temperature in degrees celsius
  float humidity;                          ///< evaluated humidity
  uint64_t uptimeSeconds;                  ///< nr of seconds the system is up
  uint64_t uptimeSecondsSinceUserEvent;    ///< nr of seconds since last user
                                           ///< user interaction
  uint32_t pairingWaitTimeSeconds;         ///< the time the application is
                                           ///< waiting for a pairing request
  uint32_t pairingCode;                    ///< key that is shown on the screen

  /// Callback to display either relative humidity or dewPoint
  void (*DisplayValueRow1)(float temperature, float relativeHumidity);

  /// Callback to display temperature unit on row 1 on the screen
  void (*DisplayTemperatureUnit1Cb)(bool on);

  /// Callback to display temperature unit on row 2 the screen
  void (*DisplayTemperatureUnit2Cb)(bool on);

  float (*TemperatureConversionCb)(
      float temperature);  ///< Convert celsius to fahrenheit if required

} Presentation_Controller_t;

/// Callback that is invoked on the sensor readout trigger
/// could be avoided
static void PublishAppTimeTickCb(void);

/// Register a timer in the time server that switches off the battery symbol
/// After a specified time interval
static void StartBatterySymbolBlinkTimer();

/// Toggle the battery low symbol on the LCD screen.
///
/// When the voltage level goes below 2.4 volt the battery low symbol starts blinking.
/// This function is used to turn the symbol alternating on and off in a timer interrupt.
static void ToggleBatteryLowSymbol();

/// Handles the presentation of the application while booting.
/// @param msg message that can be received
/// @return bool if the message was handled, false otherwise
static bool AppBootStateCb(Message_Message_t* msg);

/// Handles the presentation of the application for the two first time ticks.
/// @param msg message that can be received
/// @return bool if the message was handled, false otherwise
static bool AppShowVersionStateCb(Message_Message_t* msg);

/// Handles the presentation of the application while in normal operation.
/// @param msg message that can be received
/// @return bool if the message was handled, false otherwise
static bool AppNormalOperationStateCb(Message_Message_t* msg);

/// Handles the presentation of application while pairing.
/// @param msg message that can be received
/// @return bool if the message was handled, false otherwise
static bool AppPairingStateCb(Message_Message_t* msg);

/// Handles BatteryEvents in all states of the Presentation controller.
/// @param msg message that can be received
/// @return bool if the message was handled, false otherwise
static bool EvalBatteryEventCb(Message_Message_t* msg);

/// Default screen that displays the actual state from SHT43 demo board when
/// the application is up and running. This includes the sensor measurement
/// values, battery state information and more.
///
/// @param controller instance of the presentation controller
static void DisplayNormalOperationScreen(Presentation_Controller_t* controller);

/// Default screen that displays the sensor values from SHT gadget
///
/// @param controller instance of the presentation controller
static void LogRhtValues(Presentation_Controller_t* controller);

/// Convert to temperature to fahrenheit
/// @param temperatureC Temperature in celsius
/// @return temperature in degrees fahrenheit
static float TemperatureToFahrenheit(float temperatureC);

/// Return temperature as it is
/// @param temperatureC Temperature in celsius
/// @return temperature in degrees celsius
static float TemperatureToCelsius(float temperatureC);

/// Log the firmware version to uart
///
static void LogFirmwareVersion();

/// Default screen that is displayed when the system boots
/// until the first message arrives
///
/// @param controller instance of the presentation controller
static void DisplayVersionScreen(Presentation_Controller_t* controller);

/// Default screen that is displayed when a user is pairing.
///
/// The screen is shown until the pairing is completed either by
/// pressing the button or after a time of `PAIRING_TIMEOUT_S` seconds.
///
/// @param controller instance of the presentation controller
static void DisplayPairingScreen(Presentation_Controller_t* controller);

/// Function to display relative humidity on screen
/// @param temperature measured temperature
/// @param relativeHumidity measured relative humidity
static void DisplayRhOnScreen(float temperature, float relativeHumidity);

/// Function to display dew point on screen
/// @param temperature measured temperature
/// @param relativeHumidity measured relative humidity
static void DisplayDewPointOnScreen(float temperature, float relativeHumidity);

/// helper function to present the new sensor values
/// @param controller pointer to presentation controller
/// @param msg message that contains the new values
static void HandleNewSensorValues(Presentation_Controller_t* controller,
                                  Sht4x_SensorMessage_t* msg);

/// Display the error screen and stay in infinite loop
/// @param errorCode Error code to be displayed on the screen
static void HandleUnrecoverableError(uint32_t errorCode);

/// Select temperature unit Fahrenheit
static void SelectTemperatureUnitFahrenheit();

/// Toggle between temperature unit Fahrenheit and Celsius
static void ToggleTemperatureUnitFahrenheit();

/// Set the logger function.
/// @param enabled Flag to tell if the log is enabled or not
static void SetLogEnabled(bool enabled);

/// Function to handle all System State Changes.
/// Device state change is handled in all states of the presentation controller.
/// @param msg A message that my contain a device state change information
/// @return true if the message was a DeviceStateChange message.
static bool HandleSystemStateChange(Message_Message_t* msg);

/// Publish new readout interval
///
/// By publishing this event explicitly we prevent the ble context
/// to keep its own up-time since last user interaction.
/// @param readoutIntervalSec the new requested readout interval
static void PublishReadoutIntervalIfChanged(uint8_t readoutIntervalSec) {
  // in case it is already set, we don't need to publish anything
  if (readoutIntervalSec == _timeStepDeltaSeconds) {
    return;
  }
  Message_Message_t msg = {
      .header.category = MESSAGE_BROKER_CATEGORY_SYSTEM_STATE_CHANGE,
      .header.id = MESSAGE_ID_READOUT_INTERVAL_CHANGE,
      .parameter2 = readoutIntervalSec};
  Message_PublishAppMessage(&msg);
}

/// presentation controller instance
Presentation_Controller_t _controller = {
    .listener = {.receiveMask = MESSAGE_BROKER_CATEGORY_TIME_INFORMATION |
                                MESSAGE_BROKER_CATEGORY_BLE_EVENT |
                                MESSAGE_BROKER_CATEGORY_BATTERY_EVENT |
                                MESSAGE_BROKER_CATEGORY_SYSTEM_STATE_CHANGE |
                                MESSAGE_BROKER_CATEGORY_BUTTON_EVENT |
                                MESSAGE_BROKER_CATEGORY_SENSOR_VALUE,
                 .currentMessageHandlerCb = AppBootStateCb},
    .TemperatureConversionCb = TemperatureToCelsius,
    .bleOn = false,
    .batteryState = BATTERY_MONITOR_APP_STATE_NO_RESTRICTION,
    .DisplayValueRow1 = DisplayRhOnScreen,
    .DisplayTemperatureUnit1Cb = Screen_DisplayCelsius1,
    .DisplayTemperatureUnit2Cb = Screen_DisplayCelsius2};

MessageListener_Listener_t* Presentation_ControllerInstance() {
  return &_controller.listener;
}

// used for testing and current consumption evaluations
void Presentation_setTimeStep(uint8_t timeStepSeconds) {
  _timeStepDeltaSeconds = timeStepSeconds;
  TimerServer_Stop(_sht4xReadoutTimer);
  TimerServer_Start(_sht4xReadoutTimer, _timeStepDeltaSeconds * 1000);
}

static bool AppBootStateCb(Message_Message_t* msg) {
  if (HandleSystemStateChange(msg)) {
    return true;
  }
  if (msg->header.category == MESSAGE_BROKER_CATEGORY_TIME_INFORMATION) {
    if (msg->header.id == MESSAGE_ID_TIME_INFO_TIME_ELAPSED) {
      _controller.uptimeSeconds += msg->header.parameter1;
      _controller.uptimeSecondsSinceUserEvent += msg->header.parameter1;
      if (_controller.uptimeSeconds > 1) {
        Screen_ClearAll();
        DisplayVersionScreen(&_controller);
        // the uart log output was done earlier already!
        _controller.listener.currentMessageHandlerCb = AppShowVersionStateCb;
      }
      return true;
    }
  }
  if (msg->header.category == MESSAGE_BROKER_CATEGORY_BUTTON_EVENT) {
    SelectTemperatureUnitFahrenheit();
    return true;
  }
  return EvalBatteryEventCb(msg);
}

static bool AppShowVersionStateCb(Message_Message_t* msg) {
  if (msg->header.category == MESSAGE_BROKER_CATEGORY_TIME_INFORMATION) {
    if (msg->header.id == MESSAGE_ID_TIME_INFO_TIME_ELAPSED) {
      _controller.uptimeSeconds += msg->header.parameter1;
      _controller.uptimeSecondsSinceUserEvent += msg->header.parameter1;
      if (_controller.uptimeSeconds > 3) {
        _controller.listener.currentMessageHandlerCb =
            AppNormalOperationStateCb;
      }
      return true;
    }
  }
  if (msg->header.category == MESSAGE_BROKER_CATEGORY_BUTTON_EVENT) {
    SelectTemperatureUnitFahrenheit();
    return true;
  }
  if (HandleSystemStateChange(msg)) {
    return true;
  }
  return EvalBatteryEventCb(msg);
}

static bool AppNormalOperationStateCb(Message_Message_t* msg) {
  if ((msg->header.category == MESSAGE_BROKER_CATEGORY_SENSOR_VALUE) &&
      (msg->header.id == SHT4X_MESSAGE_ID_SENSOR_DATA) &&
      (msg->header.parameter1 != SHT4X_COMMAND_READ_SERIAL_NUMBER)) {
    HandleNewSensorValues(&_controller, (Sht4x_SensorMessage_t*)msg);
    return true;
  }
  if ((msg->header.category == MESSAGE_BROKER_CATEGORY_TIME_INFORMATION) &&
      (msg->header.id == MESSAGE_ID_TIME_INFO_TIME_ELAPSED)) {
    _controller.uptimeSeconds += msg->header.parameter1;
    _controller.uptimeSecondsSinceUserEvent += msg->header.parameter1;
    if (_controller.uptimeSecondsSinceUserEvent > 300) {
      PublishReadoutIntervalIfChanged(LONG_READOUT_INTERVAL_S);
    } else if (_controller.uptimeSecondsSinceUserEvent > 30) {
      PublishReadoutIntervalIfChanged(MEDIUM_READOUT_INTERVAL_S);
    }
    return true;
  }
  if (msg->header.category == MESSAGE_BROKER_CATEGORY_BUTTON_EVENT) {
    if (msg->header.id == BUTTON_EVENT_DOUBLE_CLICK) {
      ToggleTemperatureUnitFahrenheit();
    } else if (msg->header.id == BUTTON_EVENT_SHORT_PRESS) {
      if (_controller.DisplayValueRow1 == DisplayDewPointOnScreen) {
        _controller.DisplayValueRow1 = DisplayRhOnScreen;
      } else {
        _controller.DisplayValueRow1 = DisplayDewPointOnScreen;
      }
    }
    DisplayNormalOperationScreen(&_controller);
    _controller.uptimeSecondsSinceUserEvent = 0;
    PublishReadoutIntervalIfChanged(SHORT_READOUT_INTERVAL_S);
    return true;
  }
  if (msg->header.category == MESSAGE_BROKER_CATEGORY_BLE_EVENT) {
    if (msg->header.id == BLE_INTERFACE_MSG_ID_DISCONNECT) {
      _controller.uptimeSecondsSinceUserEvent = 0;
      PublishReadoutIntervalIfChanged(SHORT_READOUT_INTERVAL_S);
    }
    if (msg->header.id == BLE_INTERFACE_MSG_ID_ASK_USER_ACCEPT_PAIRING) {
      BleInterface_Message_t* bleMessage = (BleInterface_Message_t*)msg;
      _controller.pairingWaitTimeSeconds = 0;
      _controller.pairingCode = bleMessage->parameter.pairingCode;
      _controller.listener.currentMessageHandlerCb = AppPairingStateCb;
      DisplayPairingScreen(&_controller);
    }
    return true;
  }
  if (HandleSystemStateChange(msg)) {
    return true;
  }
  return EvalBatteryEventCb(msg);
}

static bool EvalBatteryEventCb(Message_Message_t* msg) {
  if ((msg->header.category == MESSAGE_BROKER_CATEGORY_BATTERY_EVENT) &&
      (msg->header.id == BATTERY_MONITOR_MESSAGE_ID_STATE_CHANGE)) {
    BatteryMonitor_Message_t* batteryMsg = (BatteryMonitor_Message_t*)msg;
    _controller.batteryState = batteryMsg->currentState;
    _controller.lowBatterySymbolOn =
        SHOW_BATTERY_SYMBOL(_controller.batteryState);
    if (_controller.batteryState ==
        BATTERY_MONITOR_APP_STATE_CRITICAL_BATTERY_LEVEL) {
      // these two indicators are not switched of anymore!
      StartBatterySymbolBlinkTimer();
      Screen_ForceHighContrast();
    }
    return true;
  }
  return false;
}

static bool HandleSystemStateChange(Message_Message_t* msg) {
  if (msg->header.category != MESSAGE_BROKER_CATEGORY_SYSTEM_STATE_CHANGE) {
    return false;
  }
  if (msg->header.id == MESSAGE_ID_BLE_SUBSYSTEM_OFF) {
    _controller.bleOn = false;
  }
  if (msg->header.id == MESSAGE_ID_BLE_SUBSYSTEM_ON) {
    _controller.bleOn = true;
  }
  if (msg->header.id == MESSAGE_ID_PERIPHERALS_INITIALIZED) {
    _controller.uptimeSeconds = 0;
    _controller.uptimeSecondsSinceUserEvent = 0;
    _sht4xReadoutTimer = TimerServer_CreateTimer(TIMER_SERVER_MODE_REPEATED,
                                                 PublishAppTimeTickCb);

    _controller.blinkTimer = TimerServer_CreateTimer(TIMER_SERVER_MODE_REPEATED,
                                                     ToggleBatteryLowSymbol);

    TimerServer_Start(_sht4xReadoutTimer, _timeStepDeltaSeconds * 1000);
    // At this point we are sure that the peripherals are up and running.
    // Now it is save to show the Firmware version.
    // We have to make sure that the firmware version is logged before the log
    // is switched off.
    LogFirmwareVersion();
    return true;
  }
  if (msg->header.id == MESSAGE_ID_READOUT_INTERVAL_CHANGE) {
    Presentation_setTimeStep((uint8_t)msg->parameter2);
    return true;
  }

  if (msg->header.id == MESSAGE_ID_STATE_CHANGE_ERROR) {
    // block system!
    HandleUnrecoverableError(msg->parameter2);
    return true;
  }
  if (msg->header.id == MESSAGE_ID_DEVICE_SETTINGS_READ) {
    ItemStore_SystemConfig_t* cfg = (ItemStore_SystemConfig_t*)msg->parameter2;
    SetLogEnabled(cfg->isLogEnabled);
  }
  if (msg->header.id == MESSAGE_ID_DEVICE_SETTINGS_CHANGED &&
      msg->header.parameter1 ==
          SERVICE_REQUEST_MESSAGE_ID_SET_DEBUG_LOG_ENABLE) {
    SetLogEnabled((bool)msg->parameter2);
  }
  return true;
}

static bool AppPairingStateCb(Message_Message_t* msg) {
  if ((msg->header.category == MESSAGE_BROKER_CATEGORY_SENSOR_VALUE) &&
      (msg->header.id == SHT4X_MESSAGE_ID_SENSOR_DATA) &&
      (msg->header.parameter1 != SHT4X_COMMAND_READ_SERIAL_NUMBER)) {
    Sht4x_SensorMessage_t* shtMessage = (Sht4x_SensorMessage_t*)msg;
    _controller.humidity =
        Sht4x_TicksToHumidity(shtMessage->data.measurement.humidityTicks);
    _controller.temperatureC = Sht4x_TicksToTemperatureCelsius(
        shtMessage->data.measurement.temperatureTicks);
    LogRhtValues(&_controller);
    return true;
  }
  if ((msg->header.category == MESSAGE_BROKER_CATEGORY_TIME_INFORMATION) &&
      (msg->header.id == MESSAGE_ID_TIME_INFO_TIME_ELAPSED)) {
    _controller.uptimeSeconds += msg->header.parameter1;
    _controller.uptimeSecondsSinceUserEvent += msg->header.parameter1;
    _controller.pairingWaitTimeSeconds += msg->header.parameter1;
    if (_controller.pairingWaitTimeSeconds >= PAIRING_TIMEOUT_S) {
      Message_Message_t timeoutMsg = {
          .header.category = MESSAGE_BROKER_CATEGORY_BLE_EVENT,
          .header.id = BLE_INTERFACE_MSG_ID_PAIRING_TIMEOUT,
          .header.parameter1 = 0,
          .parameter2 = 0};
      BleInterface_PublishBleMessage(&timeoutMsg);
      _controller.listener.currentMessageHandlerCb = AppNormalOperationStateCb;
      DisplayNormalOperationScreen(&_controller);
    }
    return true;
  }
  if (msg->header.category == MESSAGE_BROKER_CATEGORY_BUTTON_EVENT) {
    Message_Message_t pairingOkMsg = {
        .header.category = MESSAGE_BROKER_CATEGORY_BLE_EVENT,
        .header.id = BLE_INTERFACE_MSG_ID_USER_ACCEPTED_PAIRING,
        .header.parameter1 = 0,
        .parameter2 = 0};
    BleInterface_PublishBleMessage(&pairingOkMsg);
    _controller.listener.currentMessageHandlerCb = AppNormalOperationStateCb;
    DisplayNormalOperationScreen(&_controller);
    return true;
  }
  if (HandleSystemStateChange(msg)) {
    return true;
  }
  return EvalBatteryEventCb(msg);
}

static void SetLogEnabled(bool enabled) {
  Trace_TraceFunctionCb_t traceFun = Trace_DevNull;
  if (enabled) {
    traceFun = Uart_WriteBlocking;
  }
  Trace_RegisterTraceFunction(traceFun);
}

static void DisplayVersionScreen(Presentation_Controller_t* controller) {
  // first row: Id tag
  Screen_DisplaySymbol1(SCREEN_I);
  Screen_DisplaySymbol2(SCREEN_d);
  Screen_DisplaySymbol3(SCREEN_MINUS);

  // second row: Two bytes of the device id in Hex
  uint32_t deviceId = ProductionParameters_GetUniqueDeviceId();
  uint8_t byte1 = deviceId & 0xFF;
  Screen_DisplaySymbol8(Screen_DigitToBitmap(byte1));
  Screen_DisplaySymbol7(Screen_DigitToBitmap(byte1 >> 4));

  uint8_t byte2 = (deviceId >> 8) & 0xFF;
  Screen_DisplaySymbol6(Screen_DigitToBitmap(byte2));
  Screen_DisplaySymbol5(Screen_DigitToBitmap(byte2 >> 4));

  // the two bytes are separated by '.'
  Screen_DisplayPoint6(true);
  Screen_UpdatePendingRequests();
}

static void DisplayPairingScreen(Presentation_Controller_t* controller) {
  // turn off unit symbols regardless of the display
  Screen_DisplayCelsius1(false);
  Screen_DisplayCelsius2(false);
  Screen_DisplayFahrenheit1(false);
  Screen_DisplayFahrenheit2(false);
  Screen_DisplayRh(false);

  // turn of unused digits
  Screen_DisplaySymbol1(SCREEN_BLANK);
  Screen_DisplaySymbol5(SCREEN_BLANK);

  // show the 6 digits of the pass key
  Screen_DisplaySymbolCb_t passKey[] = {
      Screen_DisplaySymbol8, Screen_DisplaySymbol7, Screen_DisplaySymbol6,
      Screen_DisplaySymbol4, Screen_DisplaySymbol3, Screen_DisplaySymbol2};
  uint32_t absValue = _controller.pairingCode;
  uint8_t position = 0;
  uint8_t digit = 0;
  for (; position < COUNT_OF(passKey); position++) {
    digit = absValue % 10;
    passKey[position](Screen_DigitToBitmap(digit));
    absValue /= 10;
  }
  Screen_UpdatePendingRequests();
}

static void DisplayNormalOperationScreen(
    Presentation_Controller_t* controller) {
  // Display the measured values for temperature and humidity

  Screen_DisplayFahrenheit1(false);
  Screen_DisplayFahrenheit2(false);
  Screen_DisplayCelsius1(false);
  Screen_DisplayCelsius2(false);

  Screen_DisplaySymbolCb_t rowBottom[] = {
      Screen_DisplaySymbol8, Screen_DisplaySymbol7, Screen_DisplaySymbol6,
      Screen_DisplaySymbol5};

  Screen_DisplayFourDigits(
      _controller.TemperatureConversionCb(controller->temperatureC) * 100,
      rowBottom, Screen_DisplayMinusBottom);

  Screen_DisplayPoint6(true);

  _controller.DisplayValueRow1(controller->temperatureC, controller->humidity);
  _controller.DisplayTemperatureUnit2Cb(true);

  // just display the last change; in case of blinking
  // battery symbol, this will just redraw the last active value
  Screen_DisplayLowBattery(_controller.lowBatterySymbolOn);
  Screen_DisplayBluetoothSymbol(_controller.bleOn);
  Screen_DisplayCmoSens(true);

  Screen_UpdatePendingRequests();
}

static void DisplayRhOnScreen(float temperature, float relativeHumidity) {
  Screen_DisplaySymbolCb_t rowTop[] = {
      Screen_DisplaySymbol4, Screen_DisplaySymbol3, Screen_DisplaySymbol2,
      Screen_DisplaySymbol1};
  Screen_DisplayFourDigits(relativeHumidity * 100, rowTop,
                           Screen_DisplayMinusTop);
  Screen_DisplayPoint2(true);
  Screen_DisplayRh(true);
  Screen_DisplayDewPointSymbol(false);
  _controller.DisplayTemperatureUnit1Cb(false);
}

static void DisplayDewPointOnScreen(float temperature, float relativeHumidity) {
  Screen_DisplaySymbolCb_t rowTop[] = {
      Screen_DisplaySymbol4, Screen_DisplaySymbol3, Screen_DisplaySymbol2,
      Screen_DisplaySymbol1};
  float dewPoint = Sht4x_DewPointC(temperature, relativeHumidity);
  Screen_DisplayFourDigits(_controller.TemperatureConversionCb(dewPoint) * 100,
                           rowTop, Screen_DisplayMinusTop);
  Screen_DisplayPoint2(true);
  Screen_DisplayDewPointSymbol(true);
  _controller.DisplayTemperatureUnit1Cb(true);
  Screen_DisplayRh(false);
}

static void LogRhtValues(Presentation_Controller_t* controller) {
  // for some reason the floating point is not handled properly
  // by sprintf
  int humidityInt = (int)controller->humidity;
  int humidityDec =
      (int)((controller->humidity - (float)humidityInt + 0.5f) * 100);

  int tempInt = (int)controller->temperatureC;
  int tempDec = (int)((controller->temperatureC - (float)tempInt + 0.5f) * 100);
  LOG_INFO(
      "SHT43 read out -> "
      "\tTemperature = %i.%i; Humidity = %i.%i\n",
      tempInt, tempDec, humidityInt, humidityDec);
}

static void HandleNewSensorValues(Presentation_Controller_t* controller,
                                  Sht4x_SensorMessage_t* msg) {
  controller->humidity =
      Sht4x_TicksToHumidity(msg->data.measurement.humidityTicks);

  controller->temperatureC =
      Sht4x_TicksToTemperatureCelsius(msg->data.measurement.temperatureTicks);
  DisplayNormalOperationScreen(controller);
  LogRhtValues(controller);
}

static void StartBatterySymbolBlinkTimer() {
  TimerServer_Start(_controller.blinkTimer, 500U);
}

static void ToggleBatteryLowSymbol() {
  Screen_DisplayLowBattery(_controller.lowBatterySymbolOn);
  _controller.lowBatterySymbolOn = !_controller.lowBatterySymbolOn;
  Screen_UpdatePendingRequests();
}

static void HandleUnrecoverableError(uint32_t errorCode) {
  Screen_ClearAll();
  Screen_DisplaySymbol1(SCREEN_E);
  Screen_DisplaySymbol2(SCREEN_r);
  Screen_DisplaySymbol3(SCREEN_r);
  Screen_DisplaySymbolCb_t rowBottom[] = {
      Screen_DisplaySymbol8, Screen_DisplaySymbol7, Screen_DisplaySymbol6,
      Screen_DisplaySymbol5};
  Screen_DisplayFourDigits(errorCode, rowBottom, Screen_DisplayMinusBottom);
  Screen_UpdatePendingRequests();
  LOG_ERROR("Unrecoverable error %lu!!\nThe system needs to be rebooted\n!",
            (unsigned long)errorCode);
}

static void LogFirmwareVersion(void) {
  const char* firmware_suffix = FIRMWARE_VERSION_DEVELOP ? "-develop" : "";
  LOG_INFO("Firmware Version: %i.%i.%i%s\n", FIRMWARE_VERSION_MAJOR,
           FIRMWARE_VERSION_MINOR, FIRMWARE_VERSION_PATCH, firmware_suffix);
}

static void PublishAppTimeTickCb(void) {
  static uint32_t _elapsedSeconds = 0;
  _elapsedSeconds += _timeStepDeltaSeconds;
  Message_Message_t message = {
      .header.id = MESSAGE_ID_TIME_INFO_TIME_ELAPSED,
      .header.category = MESSAGE_BROKER_CATEGORY_TIME_INFORMATION,
      .header.parameter1 = _timeStepDeltaSeconds,
      .parameter2 = _elapsedSeconds};

  Message_PublishAppMessage(&message);
}

static void SelectTemperatureUnitFahrenheit() {
  _controller.TemperatureConversionCb = TemperatureToFahrenheit;
  _controller.DisplayTemperatureUnit1Cb = Screen_DisplayFahrenheit1;
  _controller.DisplayTemperatureUnit2Cb = Screen_DisplayFahrenheit2;
}

static void ToggleTemperatureUnitFahrenheit() {
  if (_controller.TemperatureConversionCb == TemperatureToFahrenheit) {
    _controller.TemperatureConversionCb = TemperatureToCelsius;
    _controller.DisplayTemperatureUnit1Cb = Screen_DisplayCelsius1;
    _controller.DisplayTemperatureUnit2Cb = Screen_DisplayCelsius2;
  } else {
    _controller.TemperatureConversionCb = TemperatureToFahrenheit;
    _controller.DisplayTemperatureUnit1Cb = Screen_DisplayFahrenheit1;
    _controller.DisplayTemperatureUnit2Cb = Screen_DisplayFahrenheit2;
  }
}

static float TemperatureToFahrenheit(float temperatureC) {
  return temperatureC * 1.8f + 32;
}

static float TemperatureToCelsius(float temperatureC) {
  return temperatureC;
}
