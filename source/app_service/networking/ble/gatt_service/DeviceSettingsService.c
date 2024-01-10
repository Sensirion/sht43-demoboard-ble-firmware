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

/// @file DeviceSettingsService.c
#include "DeviceSettingsService.h"

#include "app_service/networking/ble/BleGatt.h"
#include "app_service/nvm/ProductionParameters.h"
#include "utility/ErrorHandler.h"

#include <string.h>

/// Define an ID for each characteristic of the service
/// The ID is used as index into a function table
typedef enum {
  CHARACTERISTIC_ID_VERSION = 0,
  CHARACTERISTIC_ID_ALTERNATIVE_DEVICE_NAME,
  CHARACTERISTIC_ID_IS_LOG_ENABLED,
  CHARACTERISTIC_ID_IS_ADVERTISE_DATA_ENABLED,
  CHARACTERISTIC_ID_NR_OF_CHARS
} CharacteristicIds_t;

/// structure to hold the handles for gatt service and its characteristics
PLACE_IN_SECTION("BLE_DRIVER_CONTEXT") static struct _tService {
  uint16_t serviceHandle;  ///< Service handle
  /// table with characteristics
  BleGatt_ServiceCharacteristic_t characteristic[CHARACTERISTIC_ID_NR_OF_CHARS];
  uint16_t currentConnection;  ///< Handle to the current connection
} _service;                    ///< service instance

/// Uuid of device settings service
/// 00008100-B38D-4985-720E-0F993A68EE41
static BleTypes_Uuid_t _serviceId = {
    .uuidType = UUID_TYPE_128,
    .uuid.Char_UUID_128 = {0x41, 0xEE, 0x68, 0x3A, 0x99, 0x0F, 0x0E, 0x72, 0x85,
                           0x49, 0x8D, 0xB3, 0x00, 0x81, 0x00, 0x00}};

/// Handle service notification events
/// @param event received event
/// @return event handler status
static SVCCTL_EvtAckStatus_t EventHandler(void* event);

/// Add the device settings version characteristic.
/// @param service Service to which the characteristic belongs
static void AddVersionCharacteristic(struct _tService* service);

/// Add IsLogEnabled characteristic.
/// @param service Service to which the characteristic belongs
static void AddIsLogEnabledCharacteristic(struct _tService* service);

/// Add the IsAdvertiseDataEnabled characteristic.
/// @param service Service to which the characteristic belongs
static void AddIsAdvertiseDataEnabledCharacteristic(struct _tService* service);

/// Add the AlternativeDeviceName characteristic.
/// @param service Service to which the characteristic belongs
static void AddAlternativeDeviceNameCharacteristic(struct _tService* service);

/// Dummy event handler to be registered on characteristics without
/// event notification.
/// @param connectionHandle Handle to the connection to the peer device
/// @param data Data in the event
/// @param dataLength Length of data in the event
/// @return always returns SVCCTL_EvtAckFlowEnable
static SVCCTL_EvtAckStatus_t NopHandler(uint16_t connectionHandle,
                                        uint8_t* data,
                                        uint8_t dataLength);

/// Write the alternative device name.
/// @param connectionHandle Handle of the connection to the peer device
/// @param data Device name to be set
/// @param dataLength Length of the device name
/// @return always returns SVCCTL_EvtAckFlowEnable
static SVCCTL_EvtAckStatus_t WriteAlternativeDeviceName(
    uint16_t connectionHandle,
    uint8_t* data,
    uint8_t dataLength);

/// Write the isLogEnabled flag.
/// @param connectionHandle Handle of the connection to the peer device
/// @param data pointer to the flag
/// @param dataLength should be one
/// @return always returns SVCCTL_EvtAckFlowEnable
static SVCCTL_EvtAckStatus_t WriteIsLogEnabled(uint16_t connectionHandle,
                                               uint8_t* data,
                                               uint8_t dataLength);

/// Write the isAdvertiseDataEnabled flag.
/// @param connectionHandle Handle of the connection to the peer device
/// @param data pointer to the flag
/// @param dataLength should be one
/// @return always returns SVCCTL_EvtAckFlowEnable
static SVCCTL_EvtAckStatus_t WriteIsAdvertiseDataEnabled(
    uint16_t connectionHandle,
    uint8_t* data,
    uint8_t dataLength);

void DeviceSettingsService_Create() {
  // create service
  _service.serviceHandle = BleGatt_AddPrimaryService(_serviceId, 4);
  ASSERT(_service.serviceHandle != 0);

  // register service handle; needed for data logger service
  SVCCTL_RegisterSvcHandler(EventHandler);

  // add characteristics
  AddVersionCharacteristic(&_service);
  AddIsLogEnabledCharacteristic(&_service);
  AddIsAdvertiseDataEnabledCharacteristic(&_service);
  AddAlternativeDeviceNameCharacteristic(&_service);
}

void DeviceSettingsService_UpdateVersion(uint8_t version) {
  tBleStatus status = BleGatt_UpdateCharacteristic(
      _service.serviceHandle,
      _service.characteristic[CHARACTERISTIC_ID_VERSION].handle, &version,
      sizeof(version));
  ASSERT(status == BLE_STATUS_SUCCESS);
}

void DeviceSettingsService_UpdateIsLogEnabled(bool isLogEnabled) {
  tBleStatus status = BleGatt_UpdateCharacteristic(
      _service.serviceHandle,
      _service.characteristic[CHARACTERISTIC_ID_IS_LOG_ENABLED].handle,
      (uint8_t*)&isLogEnabled, sizeof(isLogEnabled));
  ASSERT(status == BLE_STATUS_SUCCESS);
}

void DeviceSettingsService_UpdateIsAdvertiseDataEnabled(
    bool isAdvertiseDataEnabled) {
  tBleStatus status = BleGatt_UpdateCharacteristic(
      _service.serviceHandle,
      _service.characteristic[CHARACTERISTIC_ID_IS_ADVERTISE_DATA_ENABLED]
          .handle,
      (uint8_t*)&isAdvertiseDataEnabled, sizeof(isAdvertiseDataEnabled));
  ASSERT(status == BLE_STATUS_SUCCESS);
}

void DeviceSettingsService_UpdateAlternativeDeviceName(
    const char* alternativeName) {
  size_t length = strnlen(alternativeName, 31);
  tBleStatus status = BleGatt_UpdateCharacteristic(
      _service.serviceHandle,
      _service.characteristic[CHARACTERISTIC_ID_ALTERNATIVE_DEVICE_NAME].handle,
      (uint8_t*)alternativeName, length);
  ASSERT(status == BLE_STATUS_SUCCESS);
}

// version characteristic
static void AddVersionCharacteristic(struct _tService* service) {
  BleTypes_Characteristic_t versionCharacteristic = {
      .uuid.uuid.Char_UUID_16 = 0x81FF,
      .maxValueLength = 1,
      .characteristicPropertyFlags = CHAR_PROP_READ,
      .securityFlags = SECURE_ACCESS,
      .eventFlags = GATT_DONT_NOTIFY_EVENTS,
      .encryptionKeySize = 10,
      .isVariableLengthValue = false};
  BleGatt_ExtendCharacteristicUuid(&versionCharacteristic.uuid, &_serviceId);

  uint8_t value = 1;

  uint16_t handle =
      BleGatt_AddCharacteristic(service->serviceHandle, &versionCharacteristic,
                                (uint8_t*)&value, sizeof(value));
  ASSERT(handle != 0);
  _service.characteristic[CHARACTERISTIC_ID_VERSION].handle = handle;
  _service.characteristic[CHARACTERISTIC_ID_VERSION].onRead = NopHandler;
  _service.characteristic[CHARACTERISTIC_ID_VERSION].onWrite = NopHandler;
}

// is log enabled characteristic
static void AddIsLogEnabledCharacteristic(struct _tService* service) {
  BleTypes_Characteristic_t versionCharacteristic = {
      .uuid.uuid.Char_UUID_16 = 0x81FE,
      .maxValueLength = 1,
      .characteristicPropertyFlags = CHAR_PROP_READ | CHAR_PROP_WRITE,
      .securityFlags = SECURE_ACCESS,
      .eventFlags = GATT_NOTIFY_ATTRIBUTE_WRITE,
      .encryptionKeySize = 10,
      .isVariableLengthValue = false};
  BleGatt_ExtendCharacteristicUuid(&versionCharacteristic.uuid, &_serviceId);

  bool value = false;

  uint16_t handle =
      BleGatt_AddCharacteristic(service->serviceHandle, &versionCharacteristic,
                                (uint8_t*)&value, sizeof(value));
  ASSERT(handle != 0);
  _service.characteristic[CHARACTERISTIC_ID_IS_LOG_ENABLED].handle = handle;
  _service.characteristic[CHARACTERISTIC_ID_IS_LOG_ENABLED].onRead = NopHandler;
  _service.characteristic[CHARACTERISTIC_ID_IS_LOG_ENABLED].onWrite =
      WriteIsLogEnabled;
}

// is advertise data enabled characteristic
static void AddIsAdvertiseDataEnabledCharacteristic(struct _tService* service) {
  BleTypes_Characteristic_t isAdvertiseEnabledCharacteristic = {
      .uuid.uuid.Char_UUID_16 = 0x8130,
      .maxValueLength = 1,
      .characteristicPropertyFlags = CHAR_PROP_READ | CHAR_PROP_WRITE,
      .securityFlags = SECURE_ACCESS,
      .eventFlags = GATT_NOTIFY_ATTRIBUTE_WRITE,
      .encryptionKeySize = 10,
      .isVariableLengthValue = false};
  BleGatt_ExtendCharacteristicUuid(&isAdvertiseEnabledCharacteristic.uuid,
                                   &_serviceId);

  bool value = true;

  uint16_t handle = BleGatt_AddCharacteristic(service->serviceHandle,
                                              &isAdvertiseEnabledCharacteristic,
                                              (uint8_t*)&value, sizeof(value));
  ASSERT(handle != 0);
  _service.characteristic[CHARACTERISTIC_ID_IS_ADVERTISE_DATA_ENABLED].handle =
      handle;
  _service.characteristic[CHARACTERISTIC_ID_IS_ADVERTISE_DATA_ENABLED].onRead =
      NopHandler;
  _service.characteristic[CHARACTERISTIC_ID_IS_ADVERTISE_DATA_ENABLED].onWrite =
      WriteIsAdvertiseDataEnabled;
}

// alternative device name characteristic
static void AddAlternativeDeviceNameCharacteristic(struct _tService* service) {
  BleTypes_Characteristic_t alternativeDeviceNameCharacteristic = {
      .uuid.uuid.Char_UUID_16 = 0x8120,
      .maxValueLength = 31,
      .characteristicPropertyFlags = CHAR_PROP_READ | CHAR_PROP_WRITE,
      .securityFlags = SECURE_ACCESS,
      .eventFlags = GATT_NOTIFY_ATTRIBUTE_WRITE,
      .encryptionKeySize = 10,
      .isVariableLengthValue = true};
  BleGatt_ExtendCharacteristicUuid(&alternativeDeviceNameCharacteristic.uuid,
                                   &_serviceId);

  const char* value = ProductionParameters_GetDeviceName();
  size_t nameLength = strnlen(value, 31);

  uint16_t handle = BleGatt_AddCharacteristic(
      service->serviceHandle, &alternativeDeviceNameCharacteristic,
      (uint8_t*)&value, nameLength);
  ASSERT(handle != 0);
  _service.characteristic[CHARACTERISTIC_ID_ALTERNATIVE_DEVICE_NAME].handle =
      handle;
  _service.characteristic[CHARACTERISTIC_ID_ALTERNATIVE_DEVICE_NAME].onRead =
      NopHandler;
  _service.characteristic[CHARACTERISTIC_ID_ALTERNATIVE_DEVICE_NAME].onWrite =
      WriteAlternativeDeviceName;
}

// event handler
static SVCCTL_EvtAckStatus_t EventHandler(void* void_event) {
  hci_event_pckt* event_pckt =
      (hci_event_pckt*)(((hci_uart_pckt*)void_event)->data);
  switch (event_pckt->evt) {
    case HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE: {
      evt_blecore_aci* event = (evt_blecore_aci*)event_pckt->data;
      return BleGatt_HandleBleCoreEvent(event, _service.characteristic,
                                        CHARACTERISTIC_ID_NR_OF_CHARS);
    }
    default:
      break;
  }
  return SVCCTL_EvtNotAck;
}

// Write the alternative device name.
static SVCCTL_EvtAckStatus_t WriteAlternativeDeviceName(
    uint16_t connectionHandle,
    uint8_t* data,
    uint8_t dataLength) {
  static char alternativeDeviceName[DEVICE_NAME_BUFFER_LENGTH] = {0};
  // the static variable is initialized only once!
  memset(alternativeDeviceName, 0, DEVICE_NAME_BUFFER_LENGTH);
  _service.currentConnection = connectionHandle;
  memcpy(alternativeDeviceName, data, MIN(dataLength, DEVICE_NAME_MAX_LEN));
  Message_Message_t msg = {
      .header.category = MESSAGE_BROKER_CATEGORY_BLE_SERVICE_REQUEST,
      .header.id = SERVICE_REQUEST_MESSAGE_ID_SET_ALTERNATIVE_DEVICE_NAME,
      .parameter2 = (uint32_t)alternativeDeviceName};

  Message_PublishAppMessage(&msg);
  return SVCCTL_EvtAckFlowEnable;
}

// Write the isLogEnabled flag.
static SVCCTL_EvtAckStatus_t WriteIsLogEnabled(uint16_t connectionHandle,
                                               uint8_t* data,
                                               uint8_t dataLength) {
  _service.currentConnection = connectionHandle;
  bool isLogEnabled = *data;
  Message_Message_t msg = {
      .header.category = MESSAGE_BROKER_CATEGORY_BLE_SERVICE_REQUEST,
      .header.id = SERVICE_REQUEST_MESSAGE_ID_SET_DEBUG_LOG_ENABLE,
      .parameter2 = isLogEnabled};

  Message_PublishAppMessage(&msg);
  return SVCCTL_EvtAckFlowEnable;
}

// Write the isAdvertiseDataEnabled flag.
static SVCCTL_EvtAckStatus_t WriteIsAdvertiseDataEnabled(
    uint16_t connectionHandle,
    uint8_t* data,
    uint8_t dataLength) {
  _service.currentConnection = connectionHandle;
  bool isAdvertiseEnabled = *data;
  Message_Message_t msg = {
      .header.category = MESSAGE_BROKER_CATEGORY_BLE_SERVICE_REQUEST,
      .header.id = SERVICE_REQUEST_MESSAGE_ID_SET_ADVERTISE_DATA_ENABLE,
      .parameter2 = isAdvertiseEnabled};

  Message_PublishAppMessage(&msg);
  return SVCCTL_EvtAckFlowEnable;
}

static SVCCTL_EvtAckStatus_t NopHandler(uint16_t connectionHandle,
                                        uint8_t* data,
                                        uint8_t dataLength) {
  return SVCCTL_EvtAckFlowEnable;
}
