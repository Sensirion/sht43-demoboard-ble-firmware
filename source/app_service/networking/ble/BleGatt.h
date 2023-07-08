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

/// @file BleGatt.h
#ifndef BLE_GATT_H
#define BLE_GATT_H

#include "app_service/networking/ble/BleTypes.h"

/// Add a gatt service with specified uuid and a max number of characteristics
///
/// This method service as facade to the function aci_gatt_add_service()
/// @param uuid The uuid of the gatt service to be added.
/// @param nrOfCharacteristics The maximal number of characteristics the service
///                            exposes.
/// @return The service handle of the new gatt service. If an error occurred
///         the returned value will be 0.
uint16_t BleGatt_AddPrimaryService(BleTypes_Uuid_t uuid,
                                   uint8_t nrOfCharacteristics);

/// Add a predefined characteristic to a service.
///
/// This method serves as a facade to the two functions aci_gatt_add_char(..)
/// and aci_gatt_update_char_value(..)
/// It helps to implement a service in a more declarative manner.
///
/// @param serviceHandle The handle of a gatt service
/// @param characteristic The declaration of the characteristic to be added
/// @param value A pointer to a byte array containing the value of
///              the characteristic.
/// @param valueLength number of used bytes
/// @return The handle of the added characteristic
uint16_t BleGatt_AddCharacteristic(uint16_t serviceHandle,
                                   BleTypes_Characteristic_t* characteristic,
                                   uint8_t* value,
                                   uint16_t valueLength);

#endif  // BLE_GATT_H
