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

/// @file Clock.c
///
/// Implementation of Clock.h

#include "Clock.h"

#include "stm32wbxx_hal.h"
#include "stm32wbxx_ll_rcc.h"
#include "utility/ErrorHandler.h"

/// Initialize and configure the  peripheral clocks
static void ConfigurePeripheralClocks();

/// Force a proper Rtc-backup in case of a hard-reset
///
/// This needs to be done before system clock initialization.
/// Otherwise the RTC does not start anymore in case of a hard reset!
///
static void ResetRtcBackupDomain();

void Clock_ConfigureSystemAndPeripheralClocks(uint8_t hseTuning) {
  ResetRtcBackupDomain();

  RCC_OscInitTypeDef rccOscInitStruct = {0};
  RCC_ClkInitTypeDef rccClkInitStruct = {0};

  // Config code for STM32_WPAN (HSE Tuning must be done before system clock
  // configuration)
  LL_RCC_HSE_SetCapacitorTuning(hseTuning);

  // Configure the main internal regulator output voltage
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  // Initializes the RCC Oscillators according to the specified parameters
  // in the RCC_OscInitTypeDef structure.
  rccOscInitStruct.OscillatorType =
      RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSE;
  rccOscInitStruct.HSEState = RCC_HSE_ON;
  rccOscInitStruct.HSIState = RCC_HSI_ON;
  rccOscInitStruct.LSEState = RCC_LSE_ON;
  rccOscInitStruct.MSIState = RCC_MSI_OFF;
  rccOscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  rccOscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  rccOscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&rccOscInitStruct) != HAL_OK) {
    ErrorHandler_UnrecoverableError(ERROR_CODE_HARDWARE);
  }

  // Configure the SYSCLKSource, HCLK, PCLK1 and PCLK2 clocks dividers
  rccClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK4 | RCC_CLOCKTYPE_HCLK2 |
                               RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                               RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  rccClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE;
  rccClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  rccClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  rccClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  rccClkInitStruct.AHBCLK2Divider = RCC_SYSCLK_DIV1;
  rccClkInitStruct.AHBCLK4Divider = RCC_SYSCLK_DIV1;

  if (HAL_RCC_ClockConfig(&rccClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
    ErrorHandler_UnrecoverableError(ERROR_CODE_HARDWARE);
  }
  // when we run below 16Mhz we can use the regulator range 2
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
  ConfigurePeripheralClocks();
}

// This function is derived from the cube generated function
// `PeriphCommonClock_Config()`
static void ConfigurePeripheralClocks() {
  RCC_PeriphCLKInitTypeDef periphClkInitStruct = {0};

  // Initializes the peripherals clock
  periphClkInitStruct.PeriphClockSelection =
      RCC_PERIPHCLK_SMPS | RCC_PERIPHCLK_RFWAKEUP;
  periphClkInitStruct.RFWakeUpClockSelection = RCC_RFWKPCLKSOURCE_LSE;

  periphClkInitStruct.SmpsClockSelection = RCC_SMPSCLKSOURCE_HSI;
  periphClkInitStruct.SmpsDivSelection = RCC_SMPSCLKDIV_RANGE1;

  if (HAL_RCCEx_PeriphCLKConfig(&periphClkInitStruct) != HAL_OK) {
    ErrorHandler_UnrecoverableError(ERROR_CODE_HARDWARE);
  }
}

static void ResetRtcBackupDomain() {
  // Reset the backup domain

  if ((LL_RCC_IsActiveFlag_PINRST() != 0) &&
      (LL_RCC_IsActiveFlag_SFTRST() == 0)) {
    // Enable access to the RTC registers
    HAL_PWR_EnableBkUpAccess();

    // Write twice the value to flush the APB-AHB bridge
    // This bit shall be written in the register before writing the next one
    HAL_PWR_EnableBkUpAccess();

    __HAL_RCC_BACKUPRESET_FORCE();
    __HAL_RCC_BACKUPRESET_RELEASE();
  }
}

bool Clock_ReadAndClearPorActiveFlag() {
  bool por = LL_RCC_IsActiveFlag_BORRST();
  LL_RCC_ClearResetFlags();
  return por;
}
