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

/// @file BleGatt.c
/// Implement the function s of BleGatt.h

#include "BleGatt.h"

uint16_t BleGatt_AddPrimaryService(BleTypes_Uuid_t uuid,
                                   uint8_t nrOfCharacteristics) {
  uint16_t serviceHandle = 0;
  tBleStatus status = aci_gatt_add_service(
      uuid.uuidType, (const Service_UUID_t*)&uuid.uuid, PRIMARY_SERVICE,
      nrOfCharacteristics * 2 + 1, &serviceHandle);

  if (status != BLE_STATUS_SUCCESS) {
    return 0;
  }
  return serviceHandle;
}

uint16_t BleGatt_AddCharacteristic(uint16_t serviceHandle,
                                   BleTypes_Characteristic_t* characteristic,
                                   uint8_t* value,
                                   uint16_t valueLength) {
  uint16_t characteristicHandle = 0;
  tBleStatus status = aci_gatt_add_char(
      serviceHandle, characteristic->uuid.uuidType, &characteristic->uuid.uuid,
      characteristic->maxValueLength,
      characteristic->characteristicPropertyFlags,
      characteristic->securityFlags, characteristic->eventFlags,
      characteristic->encryptionKeySize,
      (uint8_t)characteristic->isVariableLengthValue, &characteristicHandle);
  if (status != BLE_STATUS_SUCCESS) {
    return 0;
  }

  status = BleGatt_UpdateCharacteristic(serviceHandle, characteristicHandle,
                                        value, valueLength);

  if (status != BLE_STATUS_SUCCESS) {
    return 0;
  }
  return characteristicHandle;
}

tBleStatus BleGatt_UpdateCharacteristic(uint16_t serviceHandle,
                                        uint16_t characteristicHandle,
                                        uint8_t* value,
                                        uint16_t valueLength) {
  tBleStatus status = aci_gatt_update_char_value(
      serviceHandle, characteristicHandle, 0, valueLength, value);

  return status;
}

/// Build characteristic uuid from a service uuid and a 16-bit characteristic
/// id.
void BleGatt_ExtendCharacteristicUuid(BleTypes_Uuid_t* characteristicId,
                                      BleTypes_Uuid_t* serviceId) {
  uint16_t id = characteristicId->uuid.Char_UUID_16;
  memcpy(characteristicId, serviceId, sizeof(BleTypes_Uuid_t));
  characteristicId->uuid.Char_UUID_128[13] = (id >> 8) & 0xFF;
  characteristicId->uuid.Char_UUID_128[12] = id & 0xFF;
}
