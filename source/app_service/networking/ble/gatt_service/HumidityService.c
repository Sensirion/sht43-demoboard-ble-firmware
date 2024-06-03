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

/// @file HumidityService.c
#include "HumidityService.h"

#include "app_service/networking/ble/BleGatt.h"
#include "app_service/networking/ble/BleTypes.h"
#include "app_service/nvm/ProductionParameters.h"
#include "ble.h"
#include "string.h"
#include "svc/Inc/dis.h"
#include "tl.h"
#include "utility/ErrorHandler.h"

#include <math.h>

/// structure to hold the handles for gatt service and its characteristics
PLACE_IN_SECTION("BLE_DRIVER_CONTEXT") static struct _tService {
  uint16_t serviceHandle;   ///< service handle
  uint16_t humidityHandle;  ///< last humidity value
} _humidityService;         ///< service instance

/// Uuid of device humidity service
/// 00001234-B38D-4985-720E-0F993A68EE41
BleTypes_Uuid_t _humidityServiceId = {
    .uuidType = UUID_TYPE_128,
    .uuid.Char_UUID_128 = {0x41, 0xEE, 0x68, 0x3A, 0x99, 0x0F, 0x0E, 0x72, 0x85,
                           0x49, 0x8D, 0xB3, 0x34, 0x12, 0x00, 0x00}};

/// Add humidity characteristic
/// @param service the service that holds the characteristic
static void AddHumidityCharacteristic(struct _tService* service);

// Setup the temperature service
void HumidityService_Create() {
  // create service; since the characteristic includes the property notify
  // we count it twice; else we get an out_of_memory error!
  _humidityService.serviceHandle =
      BleGatt_AddPrimaryService(_humidityServiceId, 2);
  ASSERT(_humidityService.serviceHandle != 0);

  // add characteristics
  AddHumidityCharacteristic(&_humidityService);
}

void HumidityService_SetHumidity(float humidity) {
  tBleStatus status = BleGatt_UpdateCharacteristic(
      _humidityService.serviceHandle, _humidityService.humidityHandle,
      (uint8_t*)&humidity, sizeof humidity);

  ASSERT(status == BLE_STATUS_SUCCESS);
}

static void AddHumidityCharacteristic(struct _tService* service) {
  BleTypes_Characteristic_t humidityCharacteristic = {
      .uuid.uuid.Char_UUID_16 = 0x1235,
      .maxValueLength = 4,
      .characteristicPropertyFlags = CHAR_PROP_READ | CHAR_PROP_NOTIFY,
      .securityFlags = SECURE_ACCESS,
      .eventFlags = GATT_DONT_NOTIFY_EVENTS,
      .encryptionKeySize = 10,
      .isVariableLengthValue = false};
  BleGatt_ExtendCharacteristicUuid(&humidityCharacteristic.uuid,
                                   &_humidityServiceId);

  // set initial humidity
  float initialHumidity = NAN;

  service->humidityHandle = BleGatt_AddCharacteristic(
      service->serviceHandle, &humidityCharacteristic,
      (uint8_t*)&initialHumidity, sizeof initialHumidity);
  ASSERT(service->humidityHandle != 0);
}
