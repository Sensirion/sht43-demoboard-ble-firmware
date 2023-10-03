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

/// @file Crc.h
///
/// This module integrates the hardware block to calculate cyclic redundancy
/// checks (CRC).
/// CRC's are used to guarantee data integrity of sensor data and device
/// settings.
/// For simplicity reasons we always use the same polynomial in this
/// application.

#ifndef CRC_H
#define CRC_H

#include <stdint.h>

/// Enable the CRC block. After a call to this function, the crc can be used
/// to compute crc's over a byte buffer.
/// The CRC block is configured to use the polynomial 0x31 and word in
/// byte input mode.
void Crc_Enable();

/// Disable the CRC block. After a call to this function the crc is not
/// operational anymore.
void Crc_Disable();

/// Compute a crc over a given buffer.
/// @param buffer
/// @param nrOfBytes
/// @return the computed crc
uint32_t Crc_ComputeCrc(uint8_t* buffer, uint16_t nrOfBytes);

#endif  // CRC_H
