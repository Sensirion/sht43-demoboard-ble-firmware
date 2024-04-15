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

/// @file ProductionParameters.c
///
/// Implementation of the production parameter interface

#include "ProductionParameters.h"

#include "otp.h"
#include "stm32wbxx_hal.h"
#include "utility/ErrorHandler.h"

#include <stdbool.h>

/// Read the brown out level of the option byte register
#define GET_BOR_LEVEL(x) ((x)&FLASH_OPTR_BOR_LEV_Msk) >> FLASH_OPTR_BOR_LEV_Pos

/// Read brown out level and update it if needed
///
/// In order to provide a stable system, the system will be switched off, when
/// the voltage level drops below 2.2V.
/// The Check of brown out level is done on each start.
static void ConfigureBrownOutLevel();

/// Pointer to the OTP data struct
static OTP_ID0_t* gOtpId0;

/// Load data from OTP
///
/// @return Pointer to the OTP entry
static OTP_ID0_t* LoadOtpData();

void ProductionParameters_Init(void) {
  // The OPTVERR flag is wrongly set at power on
  // It shall be cleared before using any HAL_FLASH_xxx() api
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);

  // check brownout level and configure if not at expected value
  ConfigureBrownOutLevel();
  // Load data from OTP
  gOtpId0 = LoadOtpData();
}

uint8_t ProductionParameters_GetHseTuning(void) {
  return gOtpId0->hse_tuning;
}

uint8_t* ProductionParameters_GetBtDeviceAddress(void) {
  return gOtpId0->bd_address;
}

uint32_t ProductionParameters_GetUniqueDeviceId(void) {
  union {
    uint8_t byteAccess[4];  // byte access to order the bytes
    uint32_t wordAccess;
  } uuId;
  // as we need them
  uint32_t udn = LL_FLASH_GetUDN();
  // use same bytes as in OTA App!
  uuId.byteAccess[0] = udn & 0x00FF;
  uuId.byteAccess[1] = (udn & 0xFF00) >> 8;
  uuId.byteAccess[2] = LL_FLASH_GetDeviceID() & 0xFF;
  uuId.byteAccess[3] = 0;
  return uuId.wordAccess;
}

const char* ProductionParameters_GetDeviceName(void) {
  static const char deviceName[] = "SHT43 DB";
  return deviceName;
}

static OTP_ID0_t* LoadOtpData() {
  // Id0 contains hse_tuning and the bluetooth device address. We are not aware
  // of any other contents in the OTP
  uint8_t hseAndBtDeviceAddressId = 0;

  return (OTP_ID0_t*)OTP_Read(hseAndBtDeviceAddressId);
}

static void ConfigureBrownOutLevel() {
  uint32_t option_byte_0 = *(uint32_t*)OPTION_BYTE_BASE;
  // the proper level is already set
  if ((option_byte_0 & (0x7 << FLASH_OPTR_BOR_LEV_Pos)) == OB_BOR_LEVEL_0) {
    return;
  }

  // set the level to 2.2V
  FLASH_OBProgramInitTypeDef optionByteProgramData;
  HAL_FLASHEx_OBGetConfig(&optionByteProgramData);
  HAL_FLASH_Unlock();
  HAL_FLASH_OB_Unlock();

  optionByteProgramData.OptionType = OPTIONBYTE_USER;
  optionByteProgramData.UserType = OB_USER_BOR_LEV;
  optionByteProgramData.UserConfig = OB_BOR_LEVEL_0;

  HAL_StatusTypeDef status = HAL_FLASHEx_OBProgram(&optionByteProgramData);
  ASSERT(status == HAL_OK);

  // resets the system and applies the new settings
  HAL_FLASH_OB_Launch();
  // this function never returns!
  ASSERT(false);
}
