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

/// @file    BleInterface.h
///
/// Interface for the functionality of the bluetooth module. With the BLE
/// interface, the bluetooth module is initialized and all GAP as well GATT
/// are defined.

#ifndef BLEINTERFACE_H
#define BLEINTERFACE_H

#include "BleTypes.h"
#include "utility/StaticCodeAnalysisHelper.h"
#include "utility/scheduler/Message.h"

#include <stdint.h>

///BLE messages
typedef enum {
  BLE_INTERFACE_MSG_ID_START_ADVERTISE,
  BLE_INTERFACE_MSG_ID_STOP_ADVERTISE,
  BLE_INTERFACE_MSG_ID_DISCONNECT,
  BLE_INTERFACE_MSG_ID_SVC_REQ_RESPONSE,
  BLE_INTERFACE_MSG_ID_TX_POOL_AVAILABLE,
  BLE_INTERFACE_MSG_ID_UPDATE_DEVICE_SETTINGS,
} BLE_INTERFACE_MSG_ID_t;

/// Defines the message structure of explicit BleInterface
/// related messages.
typedef struct _tBleInterface_Message {
  MessageBroker_MsgHead_t head;  ///< head of the message
  union {
    BleTypes_AdvertisementMode_t
        advertisementMode;  ///< Specify how to advertise
    uint32_t reserve;       ///< Reserve to fill up to 64 bits
    void* responsePtr;      ///< Pointer to response data
    uint32_t responseData;  ///< A response data item.
  } parameter;              ///< Message parameter
} BleInterface_Message_t;

ASSERT_SIZE_TYPE1_LESS_THAN_TYPE2(Message_Message_t, uint64_t);

/// Startup the Bluetooth Low Energy interface
/// @param appContext Ble application context.
void BleInterface_Start(BleTypes_ApplicationContext_t* appContext);

/// Publish a message to the BLE app
/// @param msg The message to be published
void BleInterface_PublishBleMessage(Message_Message_t* msg);

#endif  // BLEINTERFACE_H
