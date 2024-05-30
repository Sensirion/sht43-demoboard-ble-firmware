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

/// @file I2c3.c
///
/// Implementation of I2c3.h

#include "I2c3.h"

#include "IrqPrio.h"
#include "stm32_lpm.h"
#include "utility/AppDefines.h"
#include "utility/ErrorHandler.h"
#include "utility/log/Trace.h"

#include <stdbool.h>

/// Flag to tell if driver is already initialized
static bool _initialized = false;

/// I2c driver instance
static I2C_HandleTypeDef _i2c3Instance;

/// Callback function that handles the notification of the current transfer
/// In case no transfer is ongoing, this member is set to 0.
static I2c3_OperationCompleteCb_t _operationCompleteCb;

/// instance of rx dma channel
DMA_HandleTypeDef _i2c3RxDma;

/// instance of tx dma channel
DMA_HandleTypeDef _i2c3TxDma;

/// Initialize the i2c driver structure
///
/// @param i2c a pointer to the driver structure
static void InitDriver(I2C_HandleTypeDef* i2c);

I2C_HandleTypeDef* I2c3_Instance() {
  static bool firstTimeInitialized = false;

  if (!firstTimeInitialized) {
    _initialized = true;
    firstTimeInitialized = true;
    InitDriver(&_i2c3Instance);
  }

  if (!_initialized) {
    HAL_I2C_MspInit(&_i2c3Instance);
    __HAL_I2C_ENABLE(&_i2c3Instance);
    _initialized = true;
  }
  return &_i2c3Instance;
}

void I2c3_Release() {
  // the releasing is not yet working properly and needs to be
  // detailed out in a subsequent story to optimize power consumption
  if (!_initialized || _operationCompleteCb != 0) {
    return;
  }
  _initialized = false;
  __HAL_I2C_DISABLE(&_i2c3Instance);
  HAL_I2C_MspDeInit(&_i2c3Instance);
}

void I2c3_Write(uint8_t address,
                uint8_t* data,
                uint16_t dataLength,
                I2c3_OperationCompleteCb_t doneCb) {
  ASSERT(_operationCompleteCb == 0);
  _operationCompleteCb = doneCb;
  // the system must not go to sleep while transfer is ongoing
  UTIL_LPM_SetStopMode(1 << APP_DEFINE_LPM_CLIENT_I2C, UTIL_LPM_DISABLE);
  HAL_I2C_Master_Transmit_DMA(I2c3_Instance(), address, data, dataLength);
}

void I2c3_Read(uint8_t address,
               uint8_t* data,
               uint16_t dataLength,
               I2c3_OperationCompleteCb_t doneCb) {
  ASSERT(_operationCompleteCb == 0);
  _operationCompleteCb = doneCb;
  // the system must not go to sleep while transfer is ongoing
  UTIL_LPM_SetStopMode(1 << APP_DEFINE_LPM_CLIENT_I2C, UTIL_LPM_DISABLE);
  HAL_I2C_Master_Receive_DMA(I2c3_Instance(), address, data, dataLength);
}

static void InitDriver(I2C_HandleTypeDef* i2c) {
  Trace_Message("Initialize I2C ...");
  ASSERT(i2c != 0);
  i2c->Instance = I2C3;
  // I2c clock frequency: Set the I2C_TIMINGR (the I2C timing register).
  // The value has been generated with the STM32CubeMX clock generator.
  // Target I2C SCL clock: 400 kHz (I2c fast)
  // Setting from cube 0x0010061A when using HSI as clock
  //
  // Measured I2C clock with this configuration: ~350 kHz%
  i2c->Init.Timing = 0x0010061A;
  i2c->Init.OwnAddress1 = 0;
  i2c->Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  i2c->Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  i2c->Init.OwnAddress2 = 0;
  i2c->Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  i2c->Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  i2c->Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(i2c) != HAL_OK) {
    ErrorHandler_UnrecoverableError(ERROR_CODE_HARDWARE);
  }

  if (HAL_I2CEx_ConfigAnalogFilter(i2c, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
    ErrorHandler_RecoverableError(ERROR_CODE_HARDWARE);
  }
  if (HAL_I2CEx_ConfigDigitalFilter(i2c, 0) != HAL_OK) {
    ErrorHandler_RecoverableError(ERROR_CODE_HARDWARE);
  }

  Trace_Message("SUCCESS!\n");
}

/// Handles termination of master Tx complete
/// Forwards the call to the client
/// Function override
/// @param hi2c Pointer to I2c instance
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef* hi2c) {
  if (_operationCompleteCb != 0) {
    _operationCompleteCb();
    // the transfer is complete; we can allow to go to stop mode again
    UTIL_LPM_SetStopMode(1 << APP_DEFINE_LPM_CLIENT_I2C, UTIL_LPM_ENABLE);
    _operationCompleteCb = 0;
  }
}

/// Handles termination of master Rx complete
/// Forwards the call to the client
/// Function override
/// @param hi2c Pointer to I2c instance
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef* hi2c) {
  if (_operationCompleteCb != 0) {
    _operationCompleteCb();
    // the transfer is complete; we can allow to go to stop mode again
    UTIL_LPM_SetStopMode(1 << APP_DEFINE_LPM_CLIENT_I2C, UTIL_LPM_ENABLE);
    _operationCompleteCb = 0;
  }
}

/// I2c error handler
/// @param hi2c Pointer to I2c instance
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef* hi2c) {
  ErrorHandler_RecoverableError(ERROR_CODE_HARDWARE);
  // the transfer is complete; we can allow to go to stop mode again
  UTIL_LPM_SetStopMode(1 << APP_DEFINE_LPM_CLIENT_I2C, UTIL_LPM_ENABLE);
}

/// I2C MSP Initialization
/// This function configures the hardware resources used in this example
/// Function override
/// @param hi2c: I2C handle pointer
void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if (hi2c->Instance == I2C3) {
    // Initializes the peripherals clock
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2C3;
    PeriphClkInitStruct.I2c3ClockSelection = RCC_I2C3CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
      ErrorHandler_UnrecoverableError(ERROR_CODE_HARDWARE);
    }

    __HAL_RCC_GPIOC_CLK_ENABLE();

    // I2C3 GPIO Configuration
    // PC1     ------> I2C3_SDA
    // PC0     ------> I2C3_SCL
    GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    __HAL_RCC_I2C3_CLK_ENABLE();

    // DMA controller clock enable
    __HAL_RCC_DMAMUX1_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    // I2C3 DMA I2C3_RX Init
    _i2c3RxDma.Instance = DMA1_Channel2;
    _i2c3RxDma.Init.Request = DMA_REQUEST_I2C3_RX;
    _i2c3RxDma.Init.Direction = DMA_PERIPH_TO_MEMORY;
    _i2c3RxDma.Init.PeriphInc = DMA_PINC_DISABLE;
    _i2c3RxDma.Init.MemInc = DMA_MINC_ENABLE;
    _i2c3RxDma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    _i2c3RxDma.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    _i2c3RxDma.Init.Mode = DMA_NORMAL;
    _i2c3RxDma.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&_i2c3RxDma) != HAL_OK) {
      ErrorHandler_UnrecoverableError(ERROR_CODE_HARDWARE);
    }

    __HAL_LINKDMA(hi2c, hdmarx, _i2c3RxDma);

    // I2C3_TX Init
    _i2c3TxDma.Instance = DMA1_Channel3;
    _i2c3TxDma.Init.Request = DMA_REQUEST_I2C3_TX;
    _i2c3TxDma.Init.Direction = DMA_MEMORY_TO_PERIPH;
    _i2c3TxDma.Init.PeriphInc = DMA_PINC_DISABLE;
    _i2c3TxDma.Init.MemInc = DMA_MINC_ENABLE;
    _i2c3TxDma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    _i2c3TxDma.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    _i2c3TxDma.Init.Mode = DMA_NORMAL;
    _i2c3TxDma.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&_i2c3TxDma) != HAL_OK) {
      ErrorHandler_UnrecoverableError(ERROR_CODE_HARDWARE);
    }

    __HAL_LINKDMA(hi2c, hdmatx, _i2c3TxDma);

    // the dma requires the i2c event
    HAL_NVIC_SetPriority(I2C3_EV_IRQn, IRQ_PRIO_SYSTEM, 0);
    HAL_NVIC_EnableIRQ(I2C3_EV_IRQn);

    // DMA1_Channel2_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, IRQ_PRIO_SYSTEM, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);

    // DMA1_Channel3_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, IRQ_PRIO_SYSTEM, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
  }
}

/// I2C MSP De-Initialization
/// This function frees the hardware resources used in this example
/// Function override
/// @param hi2c: I2C handle pointer
void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c) {
  if (hi2c->Instance == I2C3) {
    __HAL_RCC_I2C3_CLK_DISABLE();
    // I2C3 GPIO Configuration
    // PC1     ------> I2C3_SDA
    // PC0     ------> I2C3_SCL
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_1);

    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_0);

    HAL_DMA_DeInit(hi2c->hdmarx);
    HAL_DMA_DeInit(hi2c->hdmatx);

    HAL_NVIC_DisableIRQ(DMA1_Channel2_IRQn);
    HAL_NVIC_DisableIRQ(DMA1_Channel3_IRQn);
    HAL_NVIC_DisableIRQ(I2C3_EV_IRQn);

    __HAL_RCC_DMAMUX1_CLK_DISABLE();
    __HAL_RCC_DMA1_CLK_DISABLE();
  }
}

/// DMA Channel 2 irq handler
/// Function override
void DMA1_Channel2_IRQHandler() {
  HAL_DMA_IRQHandler(&_i2c3RxDma);
}

/// DMA Channel 3 irq handler
/// Function override
void DMA1_Channel3_IRQHandler() {
  HAL_DMA_IRQHandler(&_i2c3TxDma);
}

/// I2CE event handler
/// The i2c event is needed to synchronize with the DMA
/// Function override
void I2C3_EV_IRQHandler() {
  HAL_I2C_EV_IRQHandler(&_i2c3Instance);
}
