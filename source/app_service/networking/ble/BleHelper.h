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

/// @file BleHelper.h
///
/// Helper functions to support the BLE implementation

#ifndef BLE_HELPER_H
#define BLE_HELPER_H

#include "utility/log/Log.h"

#include <stdint.h>

/// quote the token x
/// @param x a token that needs to be quoted
#define QUOTE(x) #x

/// log a case label
/// @param x a case label to be logged
#define LOG_DEBUG_CASE(x) LOG_DEBUG("case " QUOTE(x) "\n")

/// log the status of a call or event
#define LOG_DEBUG_CALLSTATUS(...) \
  LOG_DEBUG(BleHelper_FormatCallStatus(__VA_ARGS__))

/// log a blue tooth address
#define LOG_DEBUG_BLUETOOTH_ADDR(...) \
  LOG_DEBUG(BleHelper_FormatMacAddress(__VA_ARGS__))

/// log the connection parameters
#define LOG_DEBUG_CONNECTION_PARAMS(...) \
  LOG_DEBUG(BleHelper_FormatConnectionParameters(__VA_ARGS__))

/// Helper function to format the name of a call and its result.
///
/// @param call The name that shall be printed
/// @param status A status code returned by the called function.
/// @return The formatted string
const char* BleHelper_FormatCallStatus(const char* call, uint32_t status);

/// Helper function to format a bluetooth address as hex string
/// @param address address to be printed (6 bytes are used).
/// @return The formatted string
const char* BleHelper_FormatMacAddress(const uint8_t address[6]);

/// Helper function to format the connection parameters
/// @param connection_interval connection interval
/// @param connection_latency connection latency
/// @param connection_timeout connection timeout
/// @return The formatted string
const char* BleHelper_FormatConnectionParameters(uint16_t connection_interval,
                                                 uint16_t connection_latency,
                                                 uint16_t connection_timeout);

#endif  // BLE_HELPER_H
