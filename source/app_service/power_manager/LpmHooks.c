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

/// @file LpmHooks.c
///
/// Implementation of the the hooks for the low power manager (lpm)
///
/// The implementation in this file are based on the Cube generated file
/// smt32_lpm_if.c
/// As our application requires its own clock settings when entering and
/// exiting low power, we use this implementation instead of stm32_lpm_if.c
///

#include "app_conf.h"
#include "stm32_lpm.h"
#include "utility/ErrorHandler.h"

/// Enter sleep mode
/// called by low power manager (lpm)
static void EnterSleepMode();
/// Exit sleep mode
/// called by low power manager (lpm)
static void ExitSleepMode();
/// Enter stop mode
/// called by low power manager (lpm)
static void EnterStopMode();
/// Exit stop mode
/// called by low power manager (lpm)
static void ExitStopMode();
/// Enter off mode
/// called by low power manager (lpm)
static void EnterOffMode();
/// Exit off mode
static void ExitOffMode();

/// Switch on the HSI oscillator and make it the system clock
/// internal helper function
static void SwitchOnHSI();

/// Switch on the HSE oscillator and make it the system clock
/// internal helper function
static void SwitchOnHSE();

/// Enter low power mode
/// When entering to low power mode the system clock is switched to HSI
static void EnterLowPower(void);
/// Exit low power mode
/// When exiting low power mode the system clock is switched back to HSE
/// When BLE is not running this would not be required.
static void ExitLowPower();

/// The UTIL_PowerDriver is a global function table that is used
/// by the tiny_lpm from ST.
/// All function are implemented in this file.
const struct UTIL_LPM_Driver_s UTIL_PowerDriver = {
    .EnterSleepMode = EnterSleepMode,
    .ExitSleepMode = ExitSleepMode,
    .EnterStopMode = EnterStopMode,
    .ExitStopMode = ExitStopMode,
    .EnterOffMode = EnterOffMode,
    .ExitOffMode = ExitOffMode};

void EnterOffMode(void) {
  // The systick should be disabled for the same reason than when the device
  // enters stop mode because at this time, the device may enter either
  // OffMode or StopMode.
  HAL_SuspendTick();

  EnterLowPower();

  // Clear wake-up flags (WU)
  // There is no risk to clear all the WUF here because in the current
  // implementation, this API is called in critical section.
  // If an interrupt occurs while in that critical section before that point,
  // the flag is set and will be cleared here but the system will not enter Off Mode
  // because an interrupt is pending in the NVIC.
  // The ISR will be executed when moving out of this critical section

  LL_PWR_ClearFlag_WU();

  LL_PWR_SetPowerMode(LL_PWR_MODE_STANDBY);

  LL_LPM_EnableDeepSleep();

  __WFI();

  return;
}

void ExitOffMode(void) {
  HAL_ResumeTick();
  return;
}

void EnterStopMode(void) {
  // When HAL_DBGMCU_EnableDBGStopMode() is called to keep the debugger active
  // in Stop Mode, the systick shall be disabled.
  // Otherwise the cpu may crash when moving out from stop mode.
  // When in production, the HAL_DBGMCU_EnableDBGStopMode() is not called so
  // that the device can reach best power consumption
  // However, the systick should be disabled anyway to avoid the case when it
  // is about to expire at the same time the device enters stop mode
  // (this will abort the Stop Mode entry).
  HAL_SuspendTick();

  EnterLowPower();

  LL_PWR_SetPowerMode(LL_PWR_MODE_STOP2);

  LL_LPM_EnableDeepSleep();

  __WFI();

  return;
}

void ExitStopMode(void) {
  ExitLowPower();

  HAL_ResumeTick();

  return;
}

void EnterSleepMode(void) {
  HAL_SuspendTick();

  LL_LPM_EnableSleep();

  __WFI();
  return;
}

void ExitSleepMode(void) {
  HAL_ResumeTick();
  return;
}

static void EnterLowPower(void) {
  // called in critical section
  // Acquire RCC semaphore
  while (LL_HSEM_1StepLock(HSEM, CFG_HW_RCC_SEMID))
    ;

  if (!LL_HSEM_1StepLock(HSEM, CFG_HW_ENTRY_STOP_MODE_SEMID)) {
    if (LL_PWR_IsActiveFlag_C2DS() || LL_PWR_IsActiveFlag_C2SB()) {
      // Release ENTRY_STOP_MODE semaphore
      LL_HSEM_ReleaseLock(HSEM, CFG_HW_ENTRY_STOP_MODE_SEMID, 0);

      SwitchOnHSI();
      __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_0);
    }
  } else {
    SwitchOnHSI();
    __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_0);
  }

  // Release RCC semaphore
  LL_HSEM_ReleaseLock(HSEM, CFG_HW_RCC_SEMID, 0);

  return;
}

static void ExitLowPower(void) {
  // Release ENTRY_STOP_MODE semaphore
  LL_HSEM_ReleaseLock(HSEM, CFG_HW_ENTRY_STOP_MODE_SEMID, 0);

  // Acquire RCC semaphore
  while (LL_HSEM_1StepLock(HSEM, CFG_HW_RCC_SEMID))
    ;
  if (LL_RCC_GetSysClkSource() == LL_RCC_SYS_CLKSOURCE_STATUS_HSI) {
    SwitchOnHSE();
  }
  LL_HSEM_ReleaseLock(HSEM, CFG_HW_RCC_SEMID, 0);
  return;
}

static void SwitchOnHSI() {
  LL_RCC_HSI_Enable();
  while (!LL_RCC_HSI_IsReady())
    ;
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);
  LL_RCC_SetSMPSClockSource(LL_RCC_SMPS_CLKSOURCE_HSI);
  while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI)
    ;
  return;
}

static void SwitchOnHSE() {
  LL_RCC_HSE_Enable();
  while (!LL_RCC_HSE_IsReady())
    ;
  __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSE);
  while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSE)
    ;
  return;
}
