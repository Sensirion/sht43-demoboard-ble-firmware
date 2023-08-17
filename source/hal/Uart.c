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

/// @file Uart.c
/// Implementation of Uart.h

#include "Uart.h"

#include "IrqPrio.h"
#include "stm32wbxx_ll_usart.h"
#include "utility/ErrorHandler.h"
#include "utility/concurrency/Concurrency.h"

#include <stdbool.h>

/// Bit field to capture the busy bits of the tx state as defined by
/// **HAL_UART_StateTypeDef**
#define TX_BUSY 5

/// get a received byte from the UART peripheral
#define GET_RX_DATA(huart) \
  (uint8_t)(huart->Instance->RDR & 0xFF);  // 8 bit parity = None

/// The one and only rx handler
static Uart_Receiver_t* _rxHandler = 0;

/// UART peripheral handle
static UART_HandleTypeDef _uartInstance;

/// UART dma channel for transmit
static DMA_HandleTypeDef _dmaUsart1Tx;

/// Flag that indicates if the peripheral is initialized or not
static bool _initialized = false;

/// Initialize the driver
/// @param instance pointer to driver
static void InitDriver(UART_HandleTypeDef* instance);

/// Poll the interrupt flag for the next data available
///
/// The built in polling receive uses the HAL_GetTick() that does not work in
/// interrupt context.
/// @param uart instance of the uart peripheral
/// @return true if data is available, false if the polling timed out.
static inline bool PollDataReady(UART_HandleTypeDef* uart);

UART_HandleTypeDef* Uart_Instance() {
  if (!_initialized) {
    InitDriver(&_uartInstance);
    _initialized = true;
  }
  return &_uartInstance;
}

void Uart_Release() {
  if (!_initialized || _rxHandler != 0) {
    return;
  }
  _initialized = false;
  HAL_UART_DeInit(&_uartInstance);
}

static void InitDriver(UART_HandleTypeDef* instance) {
  _uartInstance.Instance = USART1;
  _uartInstance.Init.BaudRate = 19200;
  _uartInstance.Init.WordLength = UART_WORDLENGTH_8B;
  _uartInstance.Init.StopBits = UART_STOPBITS_1;
  _uartInstance.Init.Parity = UART_PARITY_NONE;
  _uartInstance.Init.Mode = UART_MODE_TX_RX;
  _uartInstance.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  _uartInstance.Init.OverSampling = UART_OVERSAMPLING_16;
  _uartInstance.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  _uartInstance.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  _uartInstance.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&_uartInstance) != HAL_OK) {
    ErrorHandler_UnrecoverableError(ERROR_CODE_HARDWARE);
  }
  if (HAL_UARTEx_SetTxFifoThreshold(instance, UART_TXFIFO_THRESHOLD_1_8) !=
      HAL_OK) {
    ErrorHandler_UnrecoverableError(ERROR_CODE_HARDWARE);
  }
  if (HAL_UARTEx_SetRxFifoThreshold(instance, UART_RXFIFO_THRESHOLD_1_8) !=
      HAL_OK) {
    ErrorHandler_UnrecoverableError(ERROR_CODE_HARDWARE);
  }
  if (HAL_UARTEx_DisableFifoMode(instance) != HAL_OK) {
    ErrorHandler_UnrecoverableError(ERROR_CODE_HARDWARE);
  }
}

/// UART MSP Initialization
///
/// Configure the hardware resources (clocks and GPIOs) needed by the UART.
///
/// @param huart: UART handle pointer
void HAL_UART_MspInit(UART_HandleTypeDef* huart) {
  GPIO_InitTypeDef gpioInitStruct = {0};
  RCC_PeriphCLKInitTypeDef periphClkInitStruct = {0};
  if (huart->Instance == USART1) {
    // Initializes the peripherals clock
    periphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART1;
    periphClkInitStruct.Usart1ClockSelection = LL_RCC_USART1_CLKSOURCE_HSI;
    if (HAL_RCCEx_PeriphCLKConfig(&periphClkInitStruct) != HAL_OK) {
      ErrorHandler_UnrecoverableError(ERROR_CODE_HARDWARE);
    }

    // Peripheral clock enable
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_DMAMUX1_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // USART1 GPIO Configuration
    // PB6     ------> USART1_RX
    // PB7     ------> USART1_TX
    gpioInitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    gpioInitStruct.Mode = GPIO_MODE_AF_PP;
    gpioInitStruct.Pull = GPIO_NOPULL;
    gpioInitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    gpioInitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOB, &gpioInitStruct);

    _dmaUsart1Tx.Instance = DMA1_Channel4;
    _dmaUsart1Tx.Init.Request = DMA_REQUEST_USART1_TX;
    _dmaUsart1Tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    _dmaUsart1Tx.Init.PeriphInc = DMA_PINC_DISABLE;
    _dmaUsart1Tx.Init.MemInc = DMA_MINC_ENABLE;
    _dmaUsart1Tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    _dmaUsart1Tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    _dmaUsart1Tx.Init.Mode = DMA_NORMAL;
    _dmaUsart1Tx.Init.Priority = DMA_PRIORITY_LOW;

    if (HAL_DMA_Init(&_dmaUsart1Tx) != HAL_OK) {
      ErrorHandler_UnrecoverableError(ERROR_CODE_HARDWARE);
    }
    __HAL_LINKDMA(huart, hdmatx, _dmaUsart1Tx);

    // Uart interrupt configuration
    HAL_NVIC_SetPriority(USART1_IRQn, IRQ_PRIO_APP, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);

    // DMA1_Channel4_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
  }
}

/// UART MSP De-Initialization
///
/// Freeze the hardware resources used in this application
///
/// @param huart: UART handle pointer
void HAL_UART_MspDeInit(UART_HandleTypeDef* huart) {
  if (huart->Instance == USART1) {
    // Peripheral clock disable
    __HAL_RCC_USART1_CLK_DISABLE();

    // USART1 GPIO Configuration
    // PB6     ------> USART1_RX
    // PB7     ------> USART1_TX
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6 | GPIO_PIN_7);
    HAL_DMA_DeInit(&_dmaUsart1Tx);
  }
}

void Uart_WriteBlocking(const uint8_t* data, uint16_t length) {
  UART_HandleTypeDef* instance = Uart_Instance();
  HAL_UART_Transmit(instance, data, length, 500);
}

// write data
void Uart_Write(const uint8_t* data, uint16_t length) {
  // wait until previous tx is done!
  UART_HandleTypeDef* instance = Uart_Instance();
  while ((instance->gState & TX_BUSY) != 0) {
    ;  // we need to poll until tx is done and not relay on interrupts or so..
  }
  HAL_UART_Transmit_DMA(Uart_Instance(), data, length);
}

// register handler
void Uart_RegisterRxHandler(Uart_Receiver_t* rxHandler) {
  // it is always allowed to clear the registered rx handler
  if (rxHandler == 0) {
    _rxHandler = rxHandler;
    return;
  }
  // it is not allowed to overwrite an valid registered rxHandler
  ASSERT(_rxHandler == 0);
  _rxHandler = rxHandler;
  UART_HandleTypeDef* huart = Uart_Instance();
  LL_USART_ClearFlag_WKUP(huart->Instance);
  LL_USART_EnableIT_WKUP(huart->Instance);
  LL_USART_EnableInStopMode(huart->Instance);

  // get the interrupt of the first byte
  HAL_UART_Receive_IT(huart, _rxHandler->receiveBuffer, 1);
}

/// Wake up from stop mode.
/// @param huart Pointer to uart instance.
void HAL_UART_WakeupCallback(UART_HandleTypeDef* huart) {
  if (_rxHandler != 0) {
    // start receiving
    HAL_UART_Receive_IT(huart, _rxHandler->receiveBuffer, 1);
  }
}

/// Called when a rx transfer completes.
/// @param huart Pointer to Uart instance.
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart) {
  if (_rxHandler == 0) {
    return;
  }

  // Receive the rest of the message with polling
  uint8_t rxIndex = 1;
  do {
    if (!PollDataReady(huart)) {
      break;
    }
    _rxHandler->receiveBuffer[rxIndex++] = GET_RX_DATA(huart);

  } while (rxIndex < _rxHandler->rxLength);

  if (rxIndex == _rxHandler->rxLength) {
    _rxHandler->ReceiveDataCb(_rxHandler->rxLength);
  }
  // start receive again
  HAL_UART_Receive_IT(huart, _rxHandler->receiveBuffer, 1);
}

/// Called when a rx transfer completes.
/// @param huart Pointer to Uart instance.
void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart) {
  if (_rxHandler != 0) {
    // restart receive operation
    HAL_UART_Receive_IT(huart, _rxHandler->receiveBuffer, 1);
  }
}

/// This function handles DMA1 channel4 global interrupt.
/// This dma channel is used by the uart tx channel
void DMA1_Channel4_IRQHandler() {
  HAL_DMA_IRQHandler(&_dmaUsart1Tx);
}

/// Usart interrupt handler
/// This interrupt handler is used for both rx and tx interrupts!
void USART1_IRQHandler() {
  HAL_UART_IRQHandler(Uart_Instance());
}

static inline bool PollDataReady(UART_HandleTypeDef* uart) {
  // timeout does not need to be exact, but we must not block on
  // incomplete messages!
  // We will just discard them;
  uint32_t retries = 1000;
  while (((uart->Instance->ISR & (USART_ISR_RXNE_RXFNE)) == 0) &&
         (retries > 0)) {
    retries--;
  };
  return (uart->Instance->ISR & USART_ISR_RXNE_RXFNE) == USART_ISR_RXNE_RXFNE;
}
