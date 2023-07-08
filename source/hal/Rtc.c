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

/// @file Rtc.c
///
/// Implementation of Rtc.h

#include "Rtc.h"

#include "IrqPrio.h"
#include "stm32wbxx_ll_rtc.h"
#include "utility/ErrorHandler.h"
#include "utility/log/Trace.h"

#include <stdbool.h>

/// Initialize the real time clock.
///
/// @param rtc A pointer to the driver structure.
static void InitDriver(RTC_HandleTypeDef* rtc);

/// Custom configuration FOR RTC
/// It does not support 1Hz calendar
/// It divides the RTC CLK by 16
///
/// the following RTC configurations are copied form app_conf

/// clock divider
#define CFG_RTCCLK_DIV (16)

/// This sets the RTCCLK divider to the wakeup timer.
///
/// The lower is the value, the better is the power consumption and the accuracy
/// of the timerserver. The higher is the value, the finest is the granularity
#define CFG_RTC_WUCKSEL_DIVIDER (0)

/// asynchronous prescaler
#define CFG_RTC_ASYNCH_PRESCALER (0x0F)

/// synchronous prescaler
#define CFG_RTC_SYNCH_PRESCALER (0x7FFF)

RTC_HandleTypeDef* Rtc_Instance() {
  static bool initialized = false;
  static RTC_HandleTypeDef gRTC;
  if (!initialized) {
    initialized = true;
    InitDriver(&gRTC);
  }
  return &gRTC;
}

static void InitDriver(RTC_HandleTypeDef* rtc) {
  Trace_Message("Initialize RTC ...");
  ASSERT(rtc != 0);

  rtc->Instance = RTC;
  rtc->Init.HourFormat = RTC_HOURFORMAT_24;
  rtc->Init.AsynchPrediv = CFG_RTC_ASYNCH_PRESCALER;
  rtc->Init.SynchPrediv = CFG_RTC_SYNCH_PRESCALER;
  rtc->Init.OutPut = RTC_OUTPUT_DISABLE;
  rtc->Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  rtc->Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  rtc->Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  if (HAL_RTC_Init(rtc) != HAL_OK) {
    ErrorHandler_UnrecoverableError(ERROR_CODE_HARDWARE);
  }

  // Disable RTC registers write protection
  LL_RTC_DisableWriteProtection(RTC);

  LL_RTC_WAKEUP_SetClock(RTC, CFG_RTC_WUCKSEL_DIVIDER);

  // Enable RTC registers write protection
  LL_RTC_EnableWriteProtection(RTC);

  Trace_Message("SUCCESS!\n");
}

/// RTC MSP Initialization
///
/// Configure the hardware resources (clocks) needed by the RTC.
///
/// @param rtc
void HAL_RTC_MspInit(RTC_HandleTypeDef* rtc) {
  RCC_PeriphCLKInitTypeDef periphClkInitStruct = {0};
  if (rtc->Instance == RTC) {
    // Initialize the peripherals clock
    periphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    periphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    if (HAL_RCCEx_PeriphCLKConfig(&periphClkInitStruct) != HAL_OK) {
      ErrorHandler_UnrecoverableError(ERROR_CODE_HARDWARE);
    }
    // Peripheral clock enable
    __HAL_RCC_RTC_ENABLE();
    __HAL_RCC_RTCAPB_CLK_ENABLE();

    // RTC interrupt Init
    HAL_NVIC_SetPriority(RTC_WKUP_IRQn, IRQ_PRIO_RTC_WAKE_UP, 0);
    HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);
  }
}

/// RTC MSP De-Initialization
///
/// Freeze the hardware resources needed by the RTC.
///
/// @param rtc: RTC handle pointer
void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtc) {
  if (rtc->Instance == RTC) {
    // Peripheral clock disable
    __HAL_RCC_RTC_DISABLE();
    __HAL_RCC_RTCAPB_CLK_DISABLE();

    // RTC interrupt DeInit
    HAL_NVIC_DisableIRQ(RTC_WKUP_IRQn);
  }
}
