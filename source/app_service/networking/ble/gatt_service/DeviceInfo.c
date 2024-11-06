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

/// @file DeviceInfo.c
/// Implements the device information service

#include "DeviceInfo.h"

#include "app_service/networking/ble/BleGatt.h"
#include "app_service/networking/ble/BleTypes.h"
#include "app_service/nvm/ProductionParameters.h"
#include "ble.h"
#include "string.h"
#include "svc/Inc/dis.h"
#include "tl.h"
#include "utility/ErrorHandler.h"

#if defined(DEBUG)
/// Used suffix of firmware version
#define VERSION_SUFFIX "-dev.d"
#else
/// Used suffix of firmware version
#define VERSION_SUFFIX "-dev.r"
#endif

/// structure to hold the handles for gatt service and its characteristics
PLACE_IN_SECTION("BLE_DRIVER_CONTEXT") static struct _tService {
  uint16_t serviceHandle;               ///< service handle
  uint16_t modelNumberHandle;           ///< model number characteristic
  uint16_t manufacturerHandle;          ///< manufacturer name characteristic
  uint16_t firmwareVersionHandle;       ///< firmware revision characteristic
  uint16_t systemIdHandle;              ///< system id characteristic
  uint16_t serialNumberHandle;          ///< serial number characteristic
  uint16_t rebootCharacteristicHandle;  ///< reboot request characteristic
} _deviceInfoService;                   ///< device info service instance

/// Uuid of device information service
BleTypes_Uuid_t deviceInfoServiceId = {
    .uuidType = BLE_TYPES_UUID16,
    .uuid.Char_UUID_16 = DEVICE_INFORMATION_SERVICE_UUID};

/// Add model number characteristic
/// @param service the service that holds the characteristic
static void AddModelNumberCharacteristic(struct _tService* service);

/// Add manufacturer name characteristic
/// @param service the service that holds the characteristic
static void AddManufacturerNameCharacteristic(struct _tService* service);

/// Add firmware revision characteristic
/// @param service the service that holds the characteristic
static void AddFirmwareVersionCharacteristic(struct _tService* service);

/// Add serial number characteristic
/// @param service the service that holds the characteristic
static void AddSerialNumberCharacteristic(struct _tService* service);

/// Add system id characteristic
/// @param service the service that holds the characteristic
static void AddSystemIdCharacteristic(struct _tService* service);

/// Setup the device information service
void DeviceInfo_Create() {
  // create service
  _deviceInfoService.serviceHandle =
      BleGatt_AddPrimaryService(deviceInfoServiceId, 6);
  ASSERT(_deviceInfoService.serviceHandle != 0);

  // add characteristics
  AddModelNumberCharacteristic(&_deviceInfoService);
  AddSystemIdCharacteristic(&_deviceInfoService);
  AddFirmwareVersionCharacteristic(&_deviceInfoService);
  AddManufacturerNameCharacteristic(&_deviceInfoService);
  AddSerialNumberCharacteristic(&_deviceInfoService);
}

static void AddModelNumberCharacteristic(struct _tService* service) {
  BleTypes_Characteristic_t modelNumberCharacteristic = {
      .uuid.uuidType = BLE_TYPES_UUID16,
      .uuid.uuid.Char_UUID_16 = MODEL_NUMBER_UUID,
      .maxValueLength = 10,
      .characteristicPropertyFlags = CHAR_PROP_READ,
      .securityFlags = SECURE_ACCESS,
      .eventFlags = GATT_DONT_NOTIFY_EVENTS,
      .encryptionKeySize = 10,
      .isVariableLengthValue = true};

  uint16_t length = 0;
  uint8_t buffer[16];

  // add and initialize model number characteristic
  strncpy((char*)buffer, ProductionParameters_GetDeviceName(),
          sizeof(buffer) - 1);
  length = strnlen((char*)buffer, sizeof(buffer));
  service->modelNumberHandle = BleGatt_AddCharacteristic(
      service->serviceHandle, &modelNumberCharacteristic, buffer, length);
  ASSERT(service->modelNumberHandle != 0);
}

static void AddManufacturerNameCharacteristic(struct _tService* service) {
  BleTypes_Characteristic_t characteristic = {
      .uuid.uuidType = BLE_TYPES_UUID16,
      .uuid.uuid.Char_UUID_16 = MANUFACTURER_NAME_UUID,
      .maxValueLength = 10,
      .characteristicPropertyFlags = CHAR_PROP_READ,
      .securityFlags = SECURE_ACCESS,
      .eventFlags = GATT_DONT_NOTIFY_EVENTS,
      .encryptionKeySize = 10,
      .isVariableLengthValue = true};

  uint16_t length = 0;
  char buffer[] = "Sensirion";

  length = strnlen((char*)buffer, sizeof(buffer));
  service->manufacturerHandle = BleGatt_AddCharacteristic(
      service->serviceHandle, &characteristic, (uint8_t*)buffer, length);
  ASSERT(service->manufacturerHandle != 0);
}

static void AddFirmwareVersionCharacteristic(struct _tService* service) {
  uint8_t buffer[16] = {0};
  BleTypes_Characteristic_t characteristic = {
      .uuid.uuidType = BLE_TYPES_UUID16,
      .uuid.uuid.Char_UUID_16 = FIRMWARE_REVISION_UUID,
      .maxValueLength = sizeof(buffer),
      .characteristicPropertyFlags = CHAR_PROP_READ,
      .securityFlags = SECURE_ACCESS,
      .eventFlags = GATT_DONT_NOTIFY_EVENTS,
      .encryptionKeySize = 10,
      .isVariableLengthValue = true};

  if (FIRMWARE_VERSION_DEVELOP) {
    snprintf((char*)buffer, sizeof(buffer), "%i.%i.%i" VERSION_SUFFIX,
             FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR,
             FIRMWARE_VERSION_PATCH);

  } else {
    snprintf((char*)buffer, sizeof(buffer), "%i.%i.%i", FIRMWARE_VERSION_MAJOR,
             FIRMWARE_VERSION_MINOR, FIRMWARE_VERSION_PATCH);
  }
  uint8_t length = strnlen((char*)buffer, sizeof(buffer));
  service->firmwareVersionHandle = BleGatt_AddCharacteristic(
      service->serviceHandle, &characteristic, buffer, length);
}

static void AddSerialNumberCharacteristic(struct _tService* service) {
  uint32_t deviceId = ProductionParameters_GetUniqueDeviceId();
  uint8_t buffer[16] = {0};
  BleTypes_Characteristic_t characteristic = {
      .uuid.uuidType = BLE_TYPES_UUID16,
      .uuid.uuid.Char_UUID_16 = SERIAL_NUMBER_UUID,
      .maxValueLength = sizeof(buffer),
      .characteristicPropertyFlags = CHAR_PROP_READ,
      .securityFlags = SECURE_ACCESS,
      .eventFlags = GATT_DONT_NOTIFY_EVENTS,
      .encryptionKeySize = 10,
      .isVariableLengthValue = true};

  snprintf((char*)buffer, sizeof(buffer), "%lx", (unsigned long)deviceId);
  service->serialNumberHandle =
      BleGatt_AddCharacteristic(service->serviceHandle, &characteristic, buffer,
                                strnlen((char*)buffer, sizeof(buffer)));
  ASSERT(service->serialNumberHandle != 0);
}

static void AddSystemIdCharacteristic(struct _tService* service) {
  uint32_t deviceId = ProductionParameters_GetUniqueDeviceId();
  uint16_t serialNumberHigh = BLE_TYPES_SENSIRION_VENDOR_ID | 0xC000;
  uint8_t buffer[sizeof(deviceId) + sizeof(serialNumberHigh)] = {0};
  BleTypes_Characteristic_t characteristic = {
      .uuid.uuidType = BLE_TYPES_UUID16,
      .uuid.uuid.Char_UUID_16 = SYSTEM_ID_UUID,
      .maxValueLength = sizeof(buffer),
      .characteristicPropertyFlags = CHAR_PROP_READ,
      .securityFlags = SECURE_ACCESS,
      .eventFlags = GATT_DONT_NOTIFY_EVENTS,
      .encryptionKeySize = 10,
      .isVariableLengthValue = false};

  memcpy(&buffer[0], &deviceId, sizeof(deviceId));
  memcpy(&buffer[sizeof(deviceId)], &serialNumberHigh,
         sizeof(serialNumberHigh));

  service->systemIdHandle = BleGatt_AddCharacteristic(
      service->serviceHandle, &characteristic, buffer, sizeof(buffer));
  ASSERT(service->systemIdHandle != 0);
}
