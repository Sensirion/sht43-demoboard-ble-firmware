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

/// @file Qspi.h
///
/// Initialization of the QSPI peripheral and interface with
/// easy to use function primitives.

#ifndef QSPI_H
#define QSPI_H

#include "stm32wbxx_hal.h"

/// Defines the signature for a handler that needs to be informed,
/// when an instruction is written
typedef void (*Qspi_OperationCompleteCb_t)();

/// Defines the direction of a bulk data transfer
typedef enum {
  QSPI_TRANSFER_DIRECTION_READ,
  QSPI_TRANSFER_DIRECTION_WRITE
} Qspi_TransferDirection_t;

/// Defines the size of the instruction data that may be sent
/// together with the instruction.
typedef enum {
  QSPI_INSTRUCTION_DATA_SIZE_NONE = 0,
  QSPI_INSTRUCTION_DATA_SIZE_ONE_BYTE = 1,
  QSPI_INSTRUCTION_DATA_SIZE_TWO_BYTE = 2,
  QSPI_INSTRUCTION_DATA_SIZE_THREE_BYTE = 3,
  QSPI_INSTRUCTION_DATA_SIZE_FOUR_BYTE = 4,
} Qspi_InstructionDataSize_t;

/// Get the initialized pointer of the QSPI driver instance
///
/// The driver is initialized upon the first call to this function.
/// Subsequent calls return the initialized instance. After a release
/// the halted clocks will be started again.
///
/// @return pointer to the initialized driver pointer
QSPI_HandleTypeDef* Qspi_Instance();

/// Release the QSPI peripheral in order to save power
///
void Qspi_Release();

/// Write an instruction to the device that is attached to the QUADSPI
/// peripheral.
///
/// Instructions are always transferred on only one data line (MOSI).
/// The may have up to four byte of parameters and up to 255 result bytes.
///
/// The instruction format has to be looked up in the data sheet of the used
/// spi device.
/// @param instruction The instruction byte to be sent to the SPI device
/// @param instructionData The parameter of the instruction packed
///                        into an uint32_t
/// @param dataSize An enumerate specifying the number of valid bytes
///                 instruction data.
/// @param instructionResultSize Number of expected result bytes
/// @param operationCompleteCb Callback that informs about completion of this
///                             function.
void Qspi_WriteInstruction(uint8_t instruction,
                           uint32_t instructionData,
                           Qspi_InstructionDataSize_t dataSize,
                           uint8_t instructionResultSize,
                           Qspi_OperationCompleteCb_t operationCompleteCb);

/// Read the data returned by the spi device as a result of a call to the
/// Qspi_WriteInstruction()
/// @param buffer The data buffer that will be filled with the data
/// @param operationCompleteCb Callback that informs about completion of
///                                the function.
void Qspi_ReadInstructionData(uint8_t* buffer,
                              Qspi_OperationCompleteCb_t operationCompleteCb);

/// Initiate a bulk data transfer that uses all four data lines for
/// data transmission.
/// @param instruction Spi device instruction that shall be executed.
/// @param address Pointer to address
/// @param nrOfAddressBytes Size of address in bytes
/// @param buffer The data buffer
/// @param nrOfBytes The number of bytes to be transferred
/// @param waitCycles Clock cycles between writing address and start of transfer
/// @param direction Read or write operation
/// @param operationCompleteCb Callback that indicates the end of the transfer.
/// @note The device needs to support quad spi instructions. The instruction
/// is transferred on a single line.
void Qspi_QuadInitiateBulkTransfer(
    uint8_t instruction,
    uint8_t* address,
    uint8_t nrOfAddressBytes,
    uint8_t* buffer,
    uint16_t nrOfBytes,
    uint8_t waitCycles,
    Qspi_TransferDirection_t direction,
    Qspi_OperationCompleteCb_t operationCompleteCb);

#endif  // QSPI_H
