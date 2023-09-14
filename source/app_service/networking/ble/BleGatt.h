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

/// Defines the messages that are coming from a BLE service to the application.
///
/// Not all getters require a specific request since these data is published
/// without asking for it.
typedef enum {
  SERVICE_REQUEST_MESSAGE_ID_GET_LOGGING_INTERVAL,
  SERVICE_REQUEST_MESSAGE_ID_SET_LOGGING_INTERVAL,
  SERVICE_REQUEST_MESSAGE_ID_GET_AVAILABLE_SAMPLES,
  SERVICE_REQUEST_MESSAGE_ID_SET_REQUESTED_SAMPLES,
  SERVICE_REQUEST_MESSAGE_ID_SET_GADGET_NAME,
} BleGatt_ServiceRequestMessageId_t;

/// Defines the signature of an event handler that is associated with
/// a characteristic.
typedef SVCCTL_EvtAckStatus_t (*BleGatt_ClientRequestHandlerCb_t)(
    uint16_t connectionHandle,
    uint8_t* data,
    uint8_t dataLength);

/// Represents a service characteristic that reacts on ble events
/// When declaring the characteristics the eventFlags need to be set
/// appropriately.
typedef struct _tBleGatt_ServiceCharacteristic {
  uint16_t handle;  ///< The handle of the characteristic
  /// The callback that is invoked if a read is signalled by the ble core.
  /// eventFlag = GATT_NOTIFY_READ_REQ_AND_WAIT_FOR_APPL_RESP
  BleGatt_ClientRequestHandlerCb_t onRead;
  /// The callback that is invoked if a write is signalled by the ble core
  /// eventFlag = GATT_NOTIFY_ATTRIBUTE_WRITE or
  /// GATT_NOTIFY_WRITE_REQ_AND_WAIT_FOR_APPL_RESP
  BleGatt_ClientRequestHandlerCb_t onWrite;
} BleGatt_ServiceCharacteristic_t;

/// Build characteristic uuid from a service uuid and a 16-bit characteristic
/// id.
/// @param characteristicId Pointer to the characteristic id; The 16bit id is
///                         assumed to be set
/// @param serviceId Pointer to the service id, the characteristic uuid is
///                  extended from.
void BleGatt_ExtendCharacteristicUuid(BleTypes_Uuid_t* characteristicId,
                                      BleTypes_Uuid_t* serviceId);

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

/// Update an already created characteristic.
///
/// This method serves as a wrapper of the function
/// aci_gatt_update_char_value(..)
///
/// @param serviceHandle The handle of a gatt service
/// @param characteristicHandle The handle of the characteristic
/// @param value A pointer to a byte array containing the value of
///              the characteristic.
/// @param valueLength number of used bytes
/// @return The status of the update operation
tBleStatus BleGatt_UpdateCharacteristic(uint16_t serviceHandle,
                                        uint16_t characteristicHandle,
                                        uint8_t* value,
                                        uint16_t valueLength);

/// Helper function to support the implementation of service specific
/// event handlers.
/// @param event Event received from the BLE core
/// @param characteristics Array of characteristics that may handle the event
/// @param nrOfCharacteristics Size of the characteristic table
/// @return Status of the handler. This information is used by the BLE core
///         in order to continue with the client request handling.
SVCCTL_EvtAckStatus_t BleGatt_HandleBleCoreEvent(
    evt_blecore_aci* event,
    BleGatt_ServiceCharacteristic_t* characteristics,
    uint8_t nrOfCharacteristics);

#endif  // BLE_GATT_H
