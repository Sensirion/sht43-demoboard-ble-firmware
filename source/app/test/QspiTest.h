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
/// @file QspiTest.h
///
/// Here we can list test cases that need to be executed on application
/// level.

#ifndef QSPI_TEST_H
#define QSPI_TEST_H

/// Receive the flash uuid and print it to uart
///
/// this is an example of an instruction that returns data
/// and has a dummy parameter.
void QspiTest_W25Q80ReadUuid();

/// Enter the power down state of the flash
///
/// This is an example of an instruction with
/// neither parameters nor results.
void QspiTest_W25Q80EnterPowerDown();

/// Leave the power down state of the flash
///
/// This is an example of an instruction with
/// neither parameters nor results.
void QspiTest_W25Q80LeavePowerDown();

/// Set the write enable
///
/// This needed to write to the flash
void QspiTest_W25Q80WriteEnable();

/// Read status register 1
///
/// Needed in many flash operations
void QspiTest_W25Q80ReadStatusRegister1();

/// Read status register 2
///
/// Needed in many flash operations
void QspiTest_W25Q80ReadStatusRegister2();

/// Writes status register 1 and 2 and sets the bit Quad Enable
///
/// Needed to enable quad mode in flash
void QspiTest_W25Q80EnableQuadOperation();

/// Release and reinitialize QSPI
void QspiTest_RequestReleaseRequest();

/// Fast read
///
/// Needed to test quad spi functionality
void QspiTest_W25Q80FastRead();

/// Program flash page
void QspiTest_W25Q80ProgramPage();

#endif  // QSPI_TEST_H
