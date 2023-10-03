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

/// @file Crc.c
#include "Crc.h"

#include "stm32wbxx_hal.h"
#include "stm32wbxx_hal_crc.h"

/// CRC block handle
static CRC_HandleTypeDef _hcrc;

uint32_t Crc_ComputeCrc(uint8_t* buffer, uint16_t nrOfBytes) {
  uint32_t crc = HAL_CRC_Calculate(&_hcrc, (uint32_t*)buffer, nrOfBytes);
  return crc & 0xFF;
}

// enable crc
void Crc_Enable() {
  _hcrc.Instance = CRC;
  _hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_DISABLE;
  _hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
  _hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
  _hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  _hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
  _hcrc.Init.CRCLength = CRC_POLYLENGTH_8B;
  _hcrc.Init.GeneratingPolynomial = 0x31;
  HAL_CRC_Init(&_hcrc);
}

// disable crc
void Crc_Disable() {
  HAL_CRC_DeInit(&_hcrc);
}

/// Overrides from HAL library; just enable clock
/// @param hcrc Pointer to CRC_Handle
void HAL_CRC_MspDeInit(CRC_HandleTypeDef* hcrc) {
  __HAL_RCC_CRC_CLK_DISABLE();
}

/// Overrides from HAL library; just disable clock
/// @param hcrc Pointer to CRC_Handle
void HAL_CRC_MspInit(CRC_HandleTypeDef* hcrc) {
  __HAL_RCC_CRC_CLK_ENABLE();
}
