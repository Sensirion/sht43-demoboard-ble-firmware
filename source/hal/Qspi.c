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
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSEqDmaInstance
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////

/// @file Qspi.c
///
/// Implementation of Qspi.h

#include "Qspi.h"

#include "utility/ErrorHandler.h"
#include "utility/log/Trace.h"

#include <stdbool.h>
#include <string.h>

/// Size of external flash
///
/// EXTERNAL_FLASH_SIZE + 1 is the number of address bits to address every
/// byte address of the flash.
/// or 1<<(EXTERNAL_FLASH_SIZE +1) == size of flash in bytes
#define EXTERNAL_FLASH_SIZE 19

/// Qspi instance
static QSPI_HandleTypeDef gQspiInstance;

/// Dma instance
static DMA_HandleTypeDef gDmaInstance;

/// Current instruction written handler
static Qspi_OperationCompleteCb_t gCurrentInstructionWrittenCb;

/// Current instruction data received handler
static Qspi_OperationCompleteCb_t gCurrentDataReceivedCb;

/// We need this mapping for an efficient translation of
/// byte value to register value
static const uint32_t gAddressSizeToRegValue[] = {
    QSPI_ADDRESS_NONE, QSPI_ADDRESS_8_BITS, QSPI_ADDRESS_16_BITS,
    QSPI_ADDRESS_24_BITS, QSPI_ADDRESS_32_BITS};

/// Helper function to configure the data parameters of a command
///
/// Depending on the data, dataSize and resultSize different fields of the
/// command need to be set.
/// @param command Command that defines the transfer for the instruction
/// @param data Data that is either instruction address or parameter
/// @param dataSize Number of bytes of the data
/// @param resultSize Number of bytes that will be returned when the command
///                   is executed.
static void ConfigureInstructionDataParameters(
    QSPI_CommandTypeDef* command,
    uint32_t data,
    Qspi_InstructionDataSize_t dataSize,
    uint8_t resultSize);

/// Initialize the driver instance
/// @param qspi QSPI instanece that will be initialized.
static void InitDriver(QSPI_HandleTypeDef* qspi);

/// Set the direction of the DMA transfer
///
/// The dma channel needs to be configured either as peripheral to memory
/// or memory to peripheral.
/// @param direction Direction of the transfer
static void SetDmaDirection(Qspi_TransferDirection_t direction);

/// Handles the CmdComplete callback of the driver
///
/// Forwards the call to he callback that was given as a parameter
/// to Qspi_WriteInstruction()
/// @param qspiInstance Driver instance
static void HandleTransmitComplete(QSPI_HandleTypeDef* qspiInstance);

/// Handles the instruction written DataReceivedCallback of the driver
///
/// Forwards the call to he callback that was given as a parameter
/// to Qspi_ReadInstructionData()
/// @param qspiInstance Driver instance
static void HandleReceiveComplete(QSPI_HandleTypeDef* qspiInstance);

/// Flag to indacate that the clocks are not running
///
/// In low power mode the peripheral clocks are stopped. (The io's are kept
/// as with the default configuration they draw more current).
static bool _initialized = false;

////////////////////////////////////////////////
// exported functions
////////////////////////////////////////////////

QSPI_HandleTypeDef* Qspi_Instance() {
  static bool firstTimeInitialized = false;
  if (!firstTimeInitialized) {
    InitDriver(&gQspiInstance);
    firstTimeInitialized = true;
    _initialized = firstTimeInitialized;
  }
  if (!_initialized) {
    HAL_QSPI_MspInit(&gQspiInstance);
    __HAL_QSPI_ENABLE(&gQspiInstance);
    _initialized = true;
  }
  return &gQspiInstance;
}

void Qspi_Release() {
  if (!_initialized) {
    return;
  }
  /// resetting the other parts causes higher current consumption
  __HAL_QSPI_DISABLE(&gQspiInstance);
  HAL_QSPI_MspDeInit(&gQspiInstance);
  _initialized = false;
}

void Qspi_WriteInstruction(uint8_t instruction,
                           uint32_t instructionData,
                           Qspi_InstructionDataSize_t dataSize,
                           uint8_t instructionResultSize,
                           Qspi_OperationCompleteCb_t onInstructionWritten) {
  QSPI_CommandTypeDef cmd = {0};

  cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;  // QSPI_INSTRUCTION_...
  cmd.Instruction = instruction;                  // Command

  ConfigureInstructionDataParameters(&cmd, instructionData, dataSize,
                                     instructionResultSize);

  ASSERT(gCurrentInstructionWrittenCb == 0);
  gCurrentInstructionWrittenCb = onInstructionWritten;

  HAL_QSPI_Command_IT(Qspi_Instance(), &cmd);

  // the callback occurs only in case no data is written
  // in order to have a uniform interface for the client we trigger the call
  // now!
  if (cmd.DataMode != QSPI_DATA_NONE && instructionResultSize > 0) {
    gCurrentInstructionWrittenCb();
    gCurrentInstructionWrittenCb = 0;
    return;
  }
  if (cmd.DataMode != QSPI_DATA_NONE && instructionResultSize == 0) {
    HAL_QSPI_Transmit_IT(Qspi_Instance(), (uint8_t*)&instructionData);
    return;
  }
}

void Qspi_ReadInstructionData(
    uint8_t* buffer,
    Qspi_OperationCompleteCb_t onInstructionDataRead) {
  ASSERT(gCurrentDataReceivedCb == 0);
  gCurrentDataReceivedCb = onInstructionDataRead;
  HAL_QSPI_Receive_IT(Qspi_Instance(), buffer);
}

void Qspi_QuadInitiateBulkTransfer(
    uint8_t instruction,
    uint8_t* address,
    uint8_t nrOfAddressBytes,
    uint8_t* data,
    uint16_t nrOfBytes,
    uint8_t waitCycles,
    Qspi_TransferDirection_t direction,
    Qspi_OperationCompleteCb_t operationCompleteCb) {
  QSPI_CommandTypeDef cmd = {0};

  cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;  // QSPI_INSTRUCTION_...
  cmd.Instruction = instruction;                  // Command

  cmd.AddressMode = QSPI_ADDRESS_1_LINE;

  cmd.AddressSize = gAddressSizeToRegValue[nrOfAddressBytes];
  memcpy(&cmd.Address, address, nrOfAddressBytes);

  cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  cmd.AlternateBytes = QSPI_ALTERNATE_BYTES_NONE;
  cmd.AlternateBytesSize = QSPI_ALTERNATE_BYTES_NONE;

  cmd.DummyCycles = waitCycles;
  cmd.DataMode = QSPI_DATA_4_LINES;

  cmd.NbData = nrOfBytes;  // this is always smaller than 256

  cmd.DdrMode = QSPI_DDR_MODE_DISABLE;
  cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  HAL_QSPI_Command_IT(Qspi_Instance(), &cmd);

  SetDmaDirection(direction);
  // start the DMA transfer in either direction
  if (direction == QSPI_TRANSFER_DIRECTION_READ) {
    ASSERT(gCurrentDataReceivedCb == 0);
    gCurrentDataReceivedCb = operationCompleteCb;
    HAL_QSPI_Receive_DMA(Qspi_Instance(), data);
    return;
  }
  if (direction == QSPI_TRANSFER_DIRECTION_WRITE) {
    ASSERT(gCurrentInstructionWrittenCb == 0);
    gCurrentInstructionWrittenCb = operationCompleteCb;
    HAL_QSPI_Transmit_DMA(Qspi_Instance(), data);
    return;
  }
}

/// QSPI MSP Initialization
///
/// overrides library function; name must not be changed
/// @param hqspi: QSPI handle pointer
void HAL_QSPI_MspInit(QSPI_HandleTypeDef* hqspi) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if (hqspi->Instance == QUADSPI) {
    // Peripheral clock enable
    __HAL_RCC_QSPI_CLK_ENABLE();
    // the gpio clocks are already enabled.

    // QUADSPI GPIO Configuration
    // PA2     ------> QUADSPI_BK1_NCS
    // PB8     ------> QUADSPI_BK1_IO1
    // PA7     ------> QUADSPI_BK1_IO2
    // PA6     ------> QUADSPI_BK1_IO3
    // PA3     ------> QUADSPI_CLK
    // PD4     ------> QUADSPI_BK1_IO0

    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_7 | GPIO_PIN_6 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    // DMA controller clock enable
    __HAL_RCC_DMAMUX1_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    gDmaInstance.Instance = DMA1_Channel1;
    gDmaInstance.Init.Request = DMA_REQUEST_QUADSPI;
    gDmaInstance.Init.Direction = DMA_PERIPH_TO_MEMORY;
    gDmaInstance.Init.PeriphInc = DMA_PINC_DISABLE;
    gDmaInstance.Init.MemInc = DMA_MINC_ENABLE;
    gDmaInstance.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    gDmaInstance.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    gDmaInstance.Init.Mode = DMA_NORMAL;
    gDmaInstance.Init.Priority = DMA_PRIORITY_MEDIUM;
    if (HAL_DMA_Init(&gDmaInstance) != HAL_OK) {
      ErrorHandler_UnrecoverableError(ERROR_CODE_HARDWARE);
    }
    __HAL_LINKDMA(hqspi, hdma, gDmaInstance);

    // QUADSPI interrupt Init
    HAL_NVIC_SetPriority(QUADSPI_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(QUADSPI_IRQn);

    // DMA interrupt init
    // DMA1_Channel1_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  }
}

/// QSPI MSP De-Initialization
///
/// overrides library function; name must not be changed
/// @param hqspi: QSPI handle pointer
void HAL_QSPI_MspDeInit(QSPI_HandleTypeDef* hqspi) {
  if (hqspi->Instance == QUADSPI) {
    //
    __HAL_RCC_QSPI_CLK_DISABLE();

    // QUADSPI GPIO Configuration
    // PA2     ------> QUADSPI_BK1_NCS
    // PB8     ------> QUADSPI_BK1_IO1
    // PA7     ------> QUADSPI_BK1_IO2
    // PA6     ------> QUADSPI_BK1_IO3
    // PA3     ------> QUADSPI_CLK
    // PD4     ------> QUADSPI_BK1_IO0

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2 | GPIO_PIN_7 | GPIO_PIN_6 | GPIO_PIN_3);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8);

    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_4);

    HAL_DMA_DeInit(hqspi->hdma);

    HAL_NVIC_DisableIRQ(QUADSPI_IRQn);

    __HAL_RCC_DMA1_CLK_DISABLE();
    __HAL_RCC_DMAMUX1_CLK_DISABLE();
  }
}

/// This function handles QUADSPI global interrupt.
///
/// name must not be changed
void QUADSPI_IRQHandler(void) {
  HAL_QSPI_IRQHandler(Qspi_Instance());
}

/// This function handles DMA1 channel1 global interrupt.
///
/// name must not be changed
void DMA1_Channel1_IRQHandler(void) {
  HAL_DMA_IRQHandler(&gDmaInstance);
}

////////////////////////////////////////////////
// local functions
////////////////////////////////////////////////

static void InitDriver(QSPI_HandleTypeDef* hqspi) {
  hqspi->Instance = QUADSPI;
  hqspi->Init.ClockPrescaler = 2;
  hqspi->Init.FifoThreshold = 1;
  hqspi->Init.SampleShifting = QSPI_SAMPLE_SHIFTING_NONE;
  hqspi->Init.FlashSize = EXTERNAL_FLASH_SIZE;
  hqspi->Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_8_CYCLE;
  hqspi->Init.ClockMode = QSPI_CLOCK_MODE_0;
  if (HAL_QSPI_Init(hqspi) != HAL_OK) {
    ErrorHandler_UnrecoverableError(ERROR_CODE_HARDWARE);
  }

  HAL_QSPI_RegisterCallback(hqspi, HAL_QSPI_CMD_CPLT_CB_ID,
                            HandleTransmitComplete);

  HAL_QSPI_RegisterCallback(hqspi, HAL_QSPI_TX_CPLT_CB_ID,
                            HandleTransmitComplete);

  HAL_QSPI_RegisterCallback(hqspi, HAL_QSPI_RX_CPLT_CB_ID,
                            HandleReceiveComplete);
}

static void HandleTransmitComplete(QSPI_HandleTypeDef* qspiInstance) {
  if (gCurrentInstructionWrittenCb != 0) {
    gCurrentInstructionWrittenCb();
    // release the handler immediately after calling it
    gCurrentInstructionWrittenCb = 0;
  }
}

static void HandleReceiveComplete(QSPI_HandleTypeDef* qspiInstance) {
  if (gCurrentDataReceivedCb) {
    gCurrentDataReceivedCb();
    gCurrentDataReceivedCb = 0;
  }
}

static void ConfigureInstructionDataParameters(
    QSPI_CommandTypeDef* command,
    uint32_t data,
    Qspi_InstructionDataSize_t dataSize,
    uint8_t resultSize) {
  command->AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  command->AlternateBytes = QSPI_ALTERNATE_BYTES_NONE;
  command->AlternateBytesSize = QSPI_ALTERNATE_BYTES_NONE;

  command->DummyCycles = 0;
  command->DataMode = QSPI_DATA_NONE;

  command->DdrMode = QSPI_DDR_MODE_DISABLE;
  command->SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  if (resultSize != 0) {
    command->AddressMode = dataSize == QSPI_INSTRUCTION_DATA_SIZE_NONE
                               ? QSPI_ADDRESS_NONE
                               : QSPI_ADDRESS_1_LINE;
    command->AddressSize = gAddressSizeToRegValue[dataSize];
    command->Address = data;
    command->DataMode = QSPI_DATA_1_LINE;
    command->NbData = resultSize;  // this is always smaller than 256
  } else {                         // result size == 0
    command->AddressMode = QSPI_ADDRESS_NONE;
    command->NbData = dataSize;
    if (dataSize > 0) {
      command->DataMode = QSPI_DATA_1_LINE;
    }
  }
}

static void SetDmaDirection(Qspi_TransferDirection_t direction) {
  if (direction == QSPI_TRANSFER_DIRECTION_READ) {
    gDmaInstance.Init.Direction = DMA_PERIPH_TO_MEMORY;
    gDmaInstance.Init.PeriphInc = DMA_PINC_DISABLE;
    gDmaInstance.Init.MemInc = DMA_MINC_ENABLE;
    return;
  }
  if (direction == QSPI_TRANSFER_DIRECTION_WRITE) {
    gDmaInstance.Init.Direction = DMA_MEMORY_TO_PERIPH;
    gDmaInstance.Init.PeriphInc = DMA_PINC_ENABLE;
    gDmaInstance.Init.MemInc = DMA_MINC_DISABLE;
    return;
  }
}
