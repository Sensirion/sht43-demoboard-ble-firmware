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

/// @file AppDefines.h

/// This header defines some globally used macros and typedefs.

#ifndef APP_DEFINES_H
#define APP_DEFINES_H

/// Defines the lower bound for the advertisement interval after
/// starting the radio ~80ms
/// All defines are in ticks t: t / 1.6 = ms
#define SHORT_ADVERTISE_INTERVAL_MIN (0x0080)

/// Defines the upper bound for the advertisement interval after
/// starting the radio ~100ms
#define SHORT_ADVERTISE_INTERVAL_MAX (0x00A0)

/// Defines the lower bound for the advertisement interval after
/// the application was running for some time 2s
#define LONG_ADVERTISE_INTERVAL_MIN (0xC80)

/// Defines the upper bound for the advertisement interval after
/// the application was running for some time 2.5s
#define LONG_ADVERTISE_INTERVAL_MAX (0xFA0)

/// Defines the lower bound for the advertisement interval after
/// the application was running even longer  5s
#define LONG_LONG_ADVERTISE_INTERVAL_MIN (0x1F40)

/// Defines the upper bound for the advertisement interval after
/// the application was running for some time 6s
#define LONG_LONG_ADVERTISE_INTERVAL_MAX (0x2580)

// Define the various readout intervals

/// Readout interval that is selected after interaction with user
#define SHORT_READOUT_INTERVAL_S 1

/// Readout interval that is selected after 30 seconds without user interaction
#define MEDIUM_READOUT_INTERVAL_S 2

/// Readout interval that is selected after 5' without user interaction
#define LONG_READOUT_INTERVAL_S 5

/// Defines the tx power that is used for ble transmission
/// Current value 0dBm
/// The values are defined in AN5270.pdf
#define BLE_TX_POWER 0x19

/// Defines the application components that may prevent the low power manager
/// to enter a specific power down mode.
typedef enum {
  APP_DEFINE_LPM_CLIENT_APP,  ///< the main application
  APP_DEFINE_LPM_CLIENT_BLE,  ///< the ble context
  APP_DEFINE_LPM_CLIENT_I2C   ///< an i2c transaction
} AppDefine_LpmClient_t;

#endif  // APP_DEFINES_H
