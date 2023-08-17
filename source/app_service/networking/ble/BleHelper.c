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

/// @file BleHelper.c
///
/// Implementations of BleHelper functions

#include "BleHelper.h"

#include <stdint.h>
#include <stdio.h>

/// buffer to format the various debug strings
static char gBleStringFormatBuffer[256];

const char* BleHelper_FormatCallStatus(const char* call, uint32_t status) {
  snprintf(gBleStringFormatBuffer, sizeof gBleStringFormatBuffer,
           "%s returned with code 0x%04lx\n", call, status);
  return gBleStringFormatBuffer;
}

const char* BleHelper_FormatMacAddress(const uint8_t address[6]) {
  snprintf(gBleStringFormatBuffer, sizeof gBleStringFormatBuffer,
           "MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n", address[5],
           address[4], address[3], address[2], address[1], address[0]);
  return gBleStringFormatBuffer;
}

const char* BleHelper_FormatConnectionParameters(uint16_t connection_interval,
                                                 uint16_t connection_latency,
                                                 uint16_t connection_timeout) {
  snprintf(gBleStringFormatBuffer, sizeof gBleStringFormatBuffer,
           "Connection Interval:   %.2f ms\n"
           "Connection latency: %d\n"
           "Supervision Timeout: %d ms\n",
           connection_interval * 1.25, connection_latency,
           connection_timeout * 10);
  return gBleStringFormatBuffer;
}
