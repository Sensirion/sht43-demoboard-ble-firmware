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

/// @file Ipcc.c
///
/// Implementation of Ipcc.h

#include "Ipcc.h"

#include "app_conf.h"
#include "utility/ErrorHandler.h"
#include "utility/log/Log.h"

#include <stdbool.h>

/// Initialize the IPCC peripheral
///
/// @param ipcc pointer to the IPCC driver structure.
static void InitDriver(IPCC_HandleTypeDef* ipcc);

IPCC_HandleTypeDef* Ipcc_Instance() {
  static bool initialized = false;
  static IPCC_HandleTypeDef gIpcc;
  if (!initialized) {
    initialized = true;
    InitDriver(&gIpcc);
  }
  return &gIpcc;
}

static void InitDriver(IPCC_HandleTypeDef* ipcc) {
  LOG_DEBUG("Initialize IPCC ...");
  ASSERT(ipcc != 0);
  ipcc->Instance = IPCC;
  if (HAL_IPCC_Init(ipcc) != HAL_OK) {
    ErrorHandler_UnrecoverableError(ERROR_CODE_HARDWARE);
  }

  // Reset IPCC
  LL_AHB3_GRP1_EnableClock(LL_AHB3_GRP1_PERIPH_IPCC);

  LL_C1_IPCC_ClearFlag_CHx(IPCC, LL_IPCC_CHANNEL_1 | LL_IPCC_CHANNEL_2 |
                                     LL_IPCC_CHANNEL_3 | LL_IPCC_CHANNEL_4 |
                                     LL_IPCC_CHANNEL_5 | LL_IPCC_CHANNEL_6);

  LL_C2_IPCC_ClearFlag_CHx(IPCC, LL_IPCC_CHANNEL_1 | LL_IPCC_CHANNEL_2 |
                                     LL_IPCC_CHANNEL_3 | LL_IPCC_CHANNEL_4 |
                                     LL_IPCC_CHANNEL_5 | LL_IPCC_CHANNEL_6);

  LL_C1_IPCC_DisableTransmitChannel(
      IPCC, LL_IPCC_CHANNEL_1 | LL_IPCC_CHANNEL_2 | LL_IPCC_CHANNEL_3 |
                LL_IPCC_CHANNEL_4 | LL_IPCC_CHANNEL_5 | LL_IPCC_CHANNEL_6);

  LL_C2_IPCC_DisableTransmitChannel(
      IPCC, LL_IPCC_CHANNEL_1 | LL_IPCC_CHANNEL_2 | LL_IPCC_CHANNEL_3 |
                LL_IPCC_CHANNEL_4 | LL_IPCC_CHANNEL_5 | LL_IPCC_CHANNEL_6);

  LL_C1_IPCC_DisableReceiveChannel(
      IPCC, LL_IPCC_CHANNEL_1 | LL_IPCC_CHANNEL_2 | LL_IPCC_CHANNEL_3 |
                LL_IPCC_CHANNEL_4 | LL_IPCC_CHANNEL_5 | LL_IPCC_CHANNEL_6);

  LL_C2_IPCC_DisableReceiveChannel(
      IPCC, LL_IPCC_CHANNEL_1 | LL_IPCC_CHANNEL_2 | LL_IPCC_CHANNEL_3 |
                LL_IPCC_CHANNEL_4 | LL_IPCC_CHANNEL_5 | LL_IPCC_CHANNEL_6);

  // Enable IPCC(36), HSEM(38) wakeup interrupts on CPU1
  LL_EXTI_EnableIT_32_63(LL_EXTI_LINE_36 | LL_EXTI_LINE_38);

  LOG_DEBUG("SUCCESS!\n");
}

/// Handler for IPCC RX occupied interrupt.
void IPCC_C1_RX_IRQHandler(void) {
  HW_IPCC_Rx_Handler();
}

/// Handler for IPCC TX free interrupt.
void IPCC_C1_TX_IRQHandler(void) {
  HW_IPCC_Tx_Handler();
}

/// IPCC MSP Initialization
///
/// This function configures the hardware resources used in this application
///
/// @param hipcc: IPCC handle pointer
void HAL_IPCC_MspInit(IPCC_HandleTypeDef* hipcc) {
  if (hipcc->Instance == IPCC) {
    // Peripheral clock enable
    __HAL_RCC_IPCC_CLK_ENABLE();
  }
}

/// IPCC MSP De-Initialization
///
/// This function freeze the hardware resources used in this application
///
/// @param hipcc: IPCC handle pointer
void HAL_IPCC_MspDeInit(IPCC_HandleTypeDef* hipcc) {
  if (hipcc->Instance == IPCC) {
    // Peripheral clock disable
    __HAL_RCC_IPCC_CLK_DISABLE();
  }
}
