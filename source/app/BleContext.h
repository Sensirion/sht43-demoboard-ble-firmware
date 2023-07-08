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

/// @file BleContext.h
///
/// Defines the public interface of the BLE application context.
/// The main communication with the BLE application is through message passing
/// The available messages are defined in BleInterface.h

#ifndef BLE_CONTEXT_H
#define BLE_CONTEXT_H

#include "app_service/networking/ble/BleTypes.h"
#include "utility/scheduler/MessageListener.h"

/// Initialize and start the bluetooth stack and initializes the application
/// context.
void BleContext_StartBluetoothApp();

/// Return the instance to the BLE application FSM
///
/// This instance is registered in the ble message bus and processes all
/// ble messages originating from the ble application on CPU2 and the
/// main application on CPU1
///
/// @return The pointer to the Ble application FSM
MessageListener_Listener_t* BleContext_Instance();

/// Return a listener that can forward messages to the BLE task
///
/// As the ble message bus must have its own task, it is required to forward
/// messages from the main application to the ble application context
/// @return The pointer to the bridge
MessageListener_Listener_t* BleContext_BridgeInstance();

#endif  // BLE_CONTEXT_H
