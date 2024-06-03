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

/// @file SettingsController.c
#include "SettingsController.h"

#include "app_service/item_store/ItemStore.h"
#include "app_service/networking/ble/BleGatt.h"
#include "app_service/networking/ble/BleInterface.h"
#include "app_service/nvm/ProductionParameters.h"
#include "hal/Crc.h"
#include "utility/scheduler/Message.h"
#include "utility/scheduler/MessageId.h"

#include <string.h>

/// Actual version of the settings
/// When the structure changes, a new version will be defined and a conversion
/// function that allows to initialize the new version from the fields already
/// stored in the device if required.
#define SETTINGS_VERSION 1

/// Data of the settings controller
typedef struct _tSettingsController {
  /// Message listener of controller
  MessageListener_Listener_t listener;
} SettingsController_t;

/// Default that is used to initialize the values in the flash
static ItemStore_SystemConfig_t _defaultSettings = {
    .version = SETTINGS_VERSION,
    .deviceName = "SHT43 DB",
    .isLogEnabled = false,
    .isAdvertiseDataEnabled = true,
    .loggingInterval = 60000};

/// Actual settings;
/// The structure needs to be 8 byte aligned!
static ItemStore_SystemConfig_t _actualSettings ALIGN(8);

/// Default message handler of the settings controller
/// @param message Message to be processed
/// @return true if the message was handled; false otherwise
static bool DefaultStateCB(Message_Message_t* message);

/// Message handler of BLE service requests
/// @param message Message to be processed
/// @return true if the message was about handling device settings
static bool HandleBleServiceRequestCB(Message_Message_t* message);

/// Initializes the settings with the values from flash or the default if
/// no data is available on the flash
/// @param ready Flag to indicate if the enumerator is valid and can be used.
static void InitializeSettings(bool ready);

/// Update the settings and notify the change
/// @param msg parameter2 and the id of this message are forwarded.
/// @return always returns true
static bool UpdateAndNotify(Message_Message_t* msg);

/// Compute the CRC on the actual setting.
/// This function is used to add the crc when a field was changed and to
/// check if a setting read from flash is not corrupted.
/// @return The computed CRC
static uint32_t ComputeCrcOnActualSetting();

/// Instance of the settings controller
static SettingsController_t _controller = {
    .listener.currentMessageHandlerCb = DefaultStateCB,
    .listener.receiveMask = MESSAGE_BROKER_CATEGORY_BLE_SERVICE_REQUEST |
                            MESSAGE_BROKER_CATEGORY_SYSTEM_STATE_CHANGE};

/// Enumerator to retrieve the most recent setting
static ItemStore_Enumerator_t _settingsEnumerator = {.startIndex = -1};

MessageListener_Listener_t* SettingsController_Instance() {
  return &_controller.listener;
}

static bool DefaultStateCB(Message_Message_t* message) {
  if (message->header.category == MESSAGE_BROKER_CATEGORY_SYSTEM_STATE_CHANGE) {
    if (message->header.id == MESSAGE_ID_PERIPHERALS_INITIALIZED) {
      // make sure that the proper device name is used
      uint32_t deviceId = ProductionParameters_GetUniqueDeviceId() & 0xFFFF;
      snprintf(_defaultSettings.deviceName, DEVICE_NAME_BUFFER_LENGTH,
               "%s %02lx%c%02lx", ProductionParameters_GetDeviceName(),
               (deviceId >> 8), ':', (deviceId & 0xFF));  // NOLINT
      ItemStore_BeginEnumerate(ITEM_DEF_SYSTEM_CONFIG, &_settingsEnumerator,
                               InitializeSettings);
      return true;
    }
    if (message->header.id == MESSAGE_ID_BLE_SUBSYSTEM_READY) {
      Message_Message_t msg = {
          .header.category = MESSAGE_BROKER_CATEGORY_SYSTEM_STATE_CHANGE,
          .header.id = MESSAGE_ID_DEVICE_SETTINGS_READ,
          .parameter2 = (uint32_t)&_actualSettings};
      Message_PublishAppMessage(&msg);
      return true;
    }
  }
  if (message->header.category == MESSAGE_BROKER_CATEGORY_BLE_SERVICE_REQUEST) {
    return HandleBleServiceRequestCB(message);
  }
  return false;
}

static void InitializeSettings(bool ready) {
  if (!ready || !_settingsEnumerator.hasMoreItems) {
    _actualSettings = _defaultSettings;
    return;
  }
  ItemStore_GetNext(&_settingsEnumerator,
                    (ItemStore_ItemStruct_t*)&_actualSettings);

  ItemStore_EndEnumerate(&_settingsEnumerator, ITEM_DEF_SYSTEM_CONFIG);
  if (ComputeCrcOnActualSetting() != _actualSettings.crc) {
    _actualSettings = _defaultSettings;
  }
}

static bool HandleBleServiceRequestCB(Message_Message_t* message) {
  if (message->header.id == SERVICE_REQUEST_MESSAGE_ID_SAVE_LOGGING_INTERVAL) {
    uint32_t loggingInterval = message->parameter2;
    if (_actualSettings.loggingInterval == loggingInterval) {
      return true;
    }
    _actualSettings.loggingInterval = loggingInterval;
    return UpdateAndNotify(message);
  }
  if (message->header.id ==
      SERVICE_REQUEST_MESSAGE_ID_SET_ALTERNATIVE_DEVICE_NAME) {
    const char* deviceName = (const char*)message->parameter2;
    if (strncmp(deviceName, _actualSettings.deviceName, DEVICE_NAME_MAX_LEN) ==
        0) {
      return true;
    }
    strncpy(_actualSettings.deviceName, deviceName, DEVICE_NAME_MAX_LEN);
    return UpdateAndNotify(message);
  }
  if (message->header.id ==
      SERVICE_REQUEST_MESSAGE_ID_SET_ADVERTISE_DATA_ENABLE) {
    bool isAdvertiseDataEnabled = (bool)message->parameter2;
    if (isAdvertiseDataEnabled == _actualSettings.isAdvertiseDataEnabled) {
      return true;
    }
    _actualSettings.isAdvertiseDataEnabled = isAdvertiseDataEnabled;
    return UpdateAndNotify(message);
  }
  if (message->header.id == SERVICE_REQUEST_MESSAGE_ID_SET_DEBUG_LOG_ENABLE) {
    bool isLogEnabled = (bool)message->parameter2;
    if (isLogEnabled == _actualSettings.isLogEnabled) {
      return true;
    }
    _actualSettings.isLogEnabled = isLogEnabled;
    return UpdateAndNotify(message);
  }
  return false;
}

static bool UpdateAndNotify(Message_Message_t* msg) {
  uint32_t crc = ComputeCrcOnActualSetting();
  _actualSettings.crc = crc;
  ItemStore_AddItem(ITEM_DEF_SYSTEM_CONFIG,
                    (ItemStore_ItemStruct_t*)&_actualSettings);
  BleInterface_Message_t bleMessage = {
      .head.category = MESSAGE_BROKER_CATEGORY_SYSTEM_STATE_CHANGE,
      .head.id = MESSAGE_ID_DEVICE_SETTINGS_CHANGED,
      .head.parameter1 = msg->header.id,
      .parameter.responseData = msg->parameter2};
  Message_PublishAppMessage((Message_Message_t*)&bleMessage);
  return true;
}

static uint32_t ComputeCrcOnActualSetting() {
  Crc_Enable();
  uint32_t crc = Crc_ComputeCrc((uint8_t*)&_actualSettings,
                                sizeof(_actualSettings) - sizeof(uint32_t));
  // the crc is not disabled here as we might interfere with the sensor
  // that is not switching the crc on for each crc computation!
  return crc;
}
