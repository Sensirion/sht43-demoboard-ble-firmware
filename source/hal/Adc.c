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

/// @file Adc.c
#include "Adc.h"

#include "IrqPrio.h"
#include "utility/ErrorHandler.h"

#include <stdbool.h>

/// Adc1 instance
ADC_HandleTypeDef _adcInstance;

/// Flag to indicate that the driver is initialized;
static bool _initialized = false;

/// Flag to indicate that the driver is used for the first time
static bool _firstTimeInitialized = false;

/// callback that will be notified when a measurement is complete
static Adc_MeasureVbatDoneCB_t _measurementDoneCb;

/// Get the instance of the initialized Adc
/// @return initialized instance
static ADC_HandleTypeDef* Instance();

/// Release the instance after using it to save power.
static void ReleaseInstance();

void Adc_MeasureVbat(Adc_MeasureVbatDoneCB_t measurementDoneCb) {
  // only one measurement can be executed at once
  if (_measurementDoneCb != 0) {
    return;
  }
  _measurementDoneCb = measurementDoneCb;
  ADC_HandleTypeDef* instance = Instance();

  // configure the measurement settings of the ADC
  ADC_ChannelConfTypeDef adcConfig = {0};

  adcConfig.Channel =
      ADC_CHANNEL_VREFINT;  // this measures the battery voltage!
  adcConfig.Rank = ADC_REGULAR_RANK_1;
  adcConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
  adcConfig.SingleDiff = ADC_SINGLE_ENDED;
  adcConfig.OffsetNumber = ADC_OFFSET_NONE;
  adcConfig.Offset = 0;
  ASSERT(HAL_ADC_ConfigChannel(&_adcInstance, &adcConfig) == HAL_OK);

  // trigger the measurement in interrupt mode
  HAL_ADC_Start_IT(instance);
}

static void ReleaseInstance() {
  if (!_initialized) {
    return;
  }
  HAL_ADC_DeInit(&_adcInstance);
  _initialized = false;
}

static ADC_HandleTypeDef* Instance(void) {
  if (_initialized) {
    return &_adcInstance;
  }
  if (!_firstTimeInitialized) {
    // common configuration
    _adcInstance.Instance = ADC1;
    _adcInstance.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV4;
    _adcInstance.Init.Resolution = ADC_RESOLUTION_8B;
    _adcInstance.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    _adcInstance.Init.ScanConvMode = ADC_SCAN_DISABLE;
    _adcInstance.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    _adcInstance.Init.LowPowerAutoWait = DISABLE;
    _adcInstance.Init.ContinuousConvMode = DISABLE;
    _adcInstance.Init.NbrOfConversion = 1;
    _adcInstance.Init.DiscontinuousConvMode = DISABLE;
    _adcInstance.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    _adcInstance.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    _adcInstance.Init.DMAContinuousRequests = DISABLE;
    _adcInstance.Init.Overrun = ADC_OVR_DATA_PRESERVED;
    _adcInstance.Init.OversamplingMode = DISABLE;
    _firstTimeInitialized = true;
  }
  ASSERT(HAL_ADC_Init(&_adcInstance) == HAL_OK);
  _initialized = true;
  return &_adcInstance;
}

/// ADC MSP Initialization
///
/// This function configures the hardware resources of the ADC
/// Function Override.
/// @param hadc: ADC handle pointer
void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc) {
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if (hadc->Instance == ADC1) {
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    PeriphClkInitStruct.AdcClockSelection = RCC_ADCCLKSOURCE_SYSCLK;
    ASSERT(HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) == HAL_OK);
    __HAL_RCC_ADC_CLK_ENABLE();
    HAL_NVIC_SetPriority(ADC1_IRQn, IRQ_PRIO_APP, 0);
    HAL_NVIC_EnableIRQ(ADC1_IRQn);
  }
}

/// Release the hardware resources used by the adc
/// Function Override.
/// @param hadc: ADC handle pointer
void HAL_ADC_MspDeInit(ADC_HandleTypeDef* hadc) {
  if (hadc->Instance == ADC1) {
    __HAL_RCC_ADC_CLK_DISABLE();
    HAL_NVIC_DisableIRQ(ADC1_IRQn);
  }
}

/// ADC global interrupt handler
void ADC1_IRQHandler(void) {
  HAL_ADC_IRQHandler(&_adcInstance);
}

/// User specific IRQ handler
/// Function Override.
/// @param instance Adc instance pointer
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* instance) {
  uint32_t raw = HAL_ADC_GetValue(instance);
  uint32_t vref = __HAL_ADC_CALC_VREFANALOG_VOLTAGE(raw, ADC_RESOLUTION_8B);
  _measurementDoneCb(vref);
  _measurementDoneCb = 0;
  ReleaseInstance();
}
