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

/// @file BatteryService.c
#include "BatteryService.h"

#include "app_service/networking/ble/BleGatt.h"
#include "app_service/networking/ble/BleTypes.h"
#include "app_service/nvm/ProductionParameters.h"
#include "app_service/power_manager/BatteryMonitor.h"
#include "ble.h"
#include "string.h"
#include "svc/Inc/dis.h"
#include "tl.h"
#include "utility/ErrorHandler.h"
#include "utility/log/Trace.h"

#include <math.h>

/// structure to hold the handles for gatt service and its characteristics
PLACE_IN_SECTION("BLE_DRIVER_CONTEXT") static struct _tService {
  uint16_t serviceHandle;       ///< service handle
  uint16_t batteryLevelHandle;  ///< battery level value
} _batteryService;              ///< service instance

/// Uuid of device battery service
BleTypes_Uuid_t _batteryServiceId = {.uuidType = UUID_TYPE_16,
                                     .uuid.Char_UUID_16 = BATTERY_SERVICE_UUID};

/// Add battery level characteristic
/// @param service the service that holds the characteristic
static void AddBatteryLevelCharacteristic(struct _tService* service);

/// Setup the battery service
void BatteryService_Create() {
  // create service
  _batteryService.serviceHandle =
      BleGatt_AddPrimaryService(_batteryServiceId, 1);
  ASSERT(_batteryService.serviceHandle != 0);

  // add characteristics
  AddBatteryLevelCharacteristic(&_batteryService);
}

static void AddBatteryLevelCharacteristic(struct _tService* service) {
  BleTypes_Characteristic_t batteryLevelCharacteristic = {
      .uuid.uuidType = BLE_TYPES_UUID16,
      .uuid.uuid.Char_UUID_16 = BATTERY_LEVEL_CHAR_UUID,
      .maxValueLength = 1,
      .characteristicPropertyFlags = CHAR_PROP_READ,
      .securityFlags = SECURE_ACCESS,
      .eventFlags = GATT_DONT_NOTIFY_EVENTS,
      .encryptionKeySize = 10,
      .isVariableLengthValue = false};

  // set initial batteryLevel in %
  uint8_t batteryLevel = 0;

  service->batteryLevelHandle = BleGatt_AddCharacteristic(
      service->serviceHandle, &batteryLevelCharacteristic,
      (uint8_t*)&batteryLevel, sizeof batteryLevel);
  ASSERT(service->batteryLevelHandle != 0);
}

void BatteryService_SetBatteryLevel(uint8_t level) {
  tBleStatus status = BleGatt_UpdateCharacteristic(
      _batteryService.serviceHandle, _batteryService.batteryLevelHandle, &level,
      sizeof level);
  ASSERT(status == BLE_STATUS_SUCCESS);
}
