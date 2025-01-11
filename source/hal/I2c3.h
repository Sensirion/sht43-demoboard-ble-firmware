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

/// @file I2c3.h
///
/// Initialization and access to the I2c3 peripheral

#ifndef I2C3_H
#define I2C3_H

#include "stm32wbxx_ll_i2c.h"

#include <stdbool.h>
#include <stdint.h>

/// Definition of function pointer type that is used to notify
/// clients about the completion of an operation.
typedef void (*I2c3_OperationCompleteCb_t)(void);

/// Get the initialized pointer of the I2C3 driver instance
///
/// The driver is initialized upon the first call to this function.
/// Subsequent calls return the initialized instance
/// After a call of the release function, the freed resources are
/// reinitialized.
///
/// @return pointer to the initialized driver pointer
I2C_HandleTypeDef* I2c3_Instance();

/// Release the i2c instance in order to save power
///
/// @param force if true the i2c block will be released and reset
///              unconditionally.
///              Otherwise a reset and release of the block will only
///              happen, if no request is pending.
///              This option is needed for error recovery.
void I2c3_Release(bool force);

/// Trigger a write transaction on i2c
/// @param address The i2c slave address
/// @param data The data to be written
/// @param dataLength The number of bytes to be written
/// @param doneCb A callback to notify the client that the operation has
///               completed
void I2c3_Write(uint8_t address,
                uint8_t* data,
                uint16_t dataLength,
                I2c3_OperationCompleteCb_t doneCb);

/// Trigger a read transaction on i2c
/// @param address The i2c slave address (no read-bit set)
/// @param data A buffer to receive the read data
/// @param dataLength The length of the data to be read
/// @param doneCb A callback to notify the client that the operation has
///               completed
void I2c3_Read(uint8_t address,
               uint8_t* data,
               uint16_t dataLength,
               I2c3_OperationCompleteCb_t doneCb);

#endif  // I2C3_H
