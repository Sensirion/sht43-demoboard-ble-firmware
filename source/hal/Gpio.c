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

/// @file Gpio.c
///
/// Implementation of Gpio.h

#include "Gpio.h"

#include "IrqPrio.h"
#include "stm32wbxx_hal_gpio.h"
#include "utility/ErrorHandler.h"

#include <stdbool.h>

/// Alias for Port C
#define UserButton_GPIO_Port GPIOC

/// Exti signal handler
static Gpio_HandleGpioExtiSignalCb_t gPc10ExtiHandler;

// Enable the gpio clocks of port A, B, C, D
//
// This function is derived from the cube generated function
// `MX_GPIO_Init()`
void Gpio_InitClocks() {
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
}

void Gpio_RegisterOnExtiSignalPc10(Gpio_HandleGpioExtiSignalCb_t handler) {
  static bool initialized = false;
  if (!initialized) {
    GPIO_InitTypeDef gpio10Config = {0};
    gpio10Config.Pin = UserButton_Pin;
    gpio10Config.Mode = GPIO_MODE_IT_FALLING;
    gpio10Config.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(UserButton_GPIO_Port, &gpio10Config);
    initialized = true;
  }
  // we do not want to overwrite an existing listener
  ASSERT(gPc10ExtiHandler == 0);

  ASSERT(handler);

  // since this interrupt is not yet enabled we do not need to
  // block the interrupts
  gPc10ExtiHandler = handler;

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, IRQ_PRIO_APP, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void Gpio_UnregisterOnExtiSignalPc10() {
  HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
  gPc10ExtiHandler = 0;
}

bool Gpio_IsPc10Set() {
  return HAL_GPIO_ReadPin(UserButton_GPIO_Port, UserButton_Pin) == GPIO_PIN_SET;
}

/// Interrupt handler for external gpio interrupt
/// overrides weak definition name must not be changed
void EXTI15_10_IRQHandler() {
  if (__HAL_GPIO_EXTI_GET_IT(UserButton_Pin) != 0x00u) {
    __HAL_GPIO_EXTI_CLEAR_IT(UserButton_Pin);
    if (gPc10ExtiHandler != 0) {
      gPc10ExtiHandler();
    }
  }
}
