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

/// @file Screen.c
///
/// Implementation of the screen module.

#include "Screen.h"

#include "ScreenDefines.h"
#include "hal/Gpio.h"
#include "hal/IrqPrio.h"
#include "stm32wbxx.h"
#include "stm32wbxx_hal_lcd.h"
#include "utility/ErrorHandler.h"

#include <stdlib.h>

static LCD_HandleTypeDef gLcd;  ///< LCD peripheral handle

/// Initialize the LCD peripheral
static void InitLcdHal(void);

/// Data values to write the according number
const Screen_SegmentBitmap_t gScreen_Digit[] = {
    SCREEN_0,      // 0
    SCREEN_1,      // 1
    SCREEN_2,      // 2
    SCREEN_3,      // 3
    SCREEN_4,      // 4
    SCREEN_5,      // 5
    SCREEN_6,      // 6
    SCREEN_7,      // 7
    SCREEN_8,      // 8
    SCREEN_9,      // 9
    SCREEN_A,      // A
    SCREEN_b,      // b
    SCREEN_C,      // C
    SCREEN_d,      // d
    SCREEN_E,      // E
    SCREEN_F,      // f
    SCREEN_BLANK,  // empty
    SCREEN_MINUS,  // display "-"
};

/// LCD Initialization Function
static void InitLcdHal(void) {
  gLcd.Instance = LCD;

  // LSE CLK: LSE = 32744Hz
  // freq = LSE_CLK/ (2*PS*(16+DIV)) according spec page 529
  //
  gLcd.Init.Prescaler = LCD_PRESCALER_8;
  gLcd.Init.Divider = LCD_DIVIDER_16;
  gLcd.Init.Duty = LCD_DUTY_1_4;
  gLcd.Init.Bias = LCD_BIAS_1_3;
  gLcd.Init.VoltageSource = LCD_VOLTAGESOURCE_INTERNAL;
  gLcd.Init.Contrast = LCD_CONTRASTLEVEL_3;
  gLcd.Init.DeadTime = LCD_DEADTIME_0;
  gLcd.Init.PulseOnDuration = LCD_PULSEONDURATION_0;
  gLcd.Init.MuxSegment = LCD_MUXSEGMENT_DISABLE;
  gLcd.Init.BlinkMode = LCD_BLINKMODE_OFF;
  gLcd.Init.BlinkFrequency = LCD_BLINKFREQUENCY_DIV8;
  gLcd.Init.HighDrive = LCD_HIGHDRIVE_ENABLE;

  if (HAL_LCD_Init(&gLcd) != HAL_OK) {
    ErrorHandler_UnrecoverableError(ERROR_CODE_HARDWARE);
  }
}

void Screen_Init() {
  // Initialize LCD HAL
  InitLcdHal();
  Screen_TurnAllSegmentsOn();
}

void Screen_TurnAllSegmentsOn() {
  HAL_LCD_Write(&gLcd, LCD_COM_0_0, LCD_ALL_SEG_MASK_1, ~LCD_ALL_SEG_MASK_1);
  HAL_LCD_Write(&gLcd, LCD_COM_0_1, LCD_ALL_SEG_MASK_2, ~LCD_ALL_SEG_MASK_2);
  HAL_LCD_Write(&gLcd, LCD_COM_1_0, LCD_ALL_SEG_MASK_1, ~LCD_ALL_SEG_MASK_1);
  HAL_LCD_Write(&gLcd, LCD_COM_1_1, LCD_ALL_SEG_MASK_2, ~LCD_ALL_SEG_MASK_2);
  HAL_LCD_Write(&gLcd, LCD_COM_2_0, LCD_ALL_SEG_MASK_1, ~LCD_ALL_SEG_MASK_1);
  HAL_LCD_Write(&gLcd, LCD_COM_2_1, LCD_ALL_SEG_MASK_2, ~LCD_ALL_SEG_MASK_2);
  HAL_LCD_Write(&gLcd, LCD_COM_3_0, LCD_ALL_SEG_MASK_1, ~LCD_ALL_SEG_MASK_1);
  HAL_LCD_Write(&gLcd, LCD_COM_3_1, LCD_ALL_SEG_MASK_2, ~LCD_ALL_SEG_MASK_2);
  HAL_LCD_UpdateDisplayRequest(&gLcd);
}

inline Screen_SegmentBitmap_t Screen_DigitToBitmap(uint8_t digit) {
  return gScreen_Digit[digit % 16];
}

void Screen_DisplaySymbol1(Screen_SegmentBitmap_t bitmap) {
  uint32_t data = ((bitmap & 0x01) << LCD_SEG32_SHIFT) |
                  (((bitmap & 0x02) >> 1) << LCD_SEG39_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_0_1, LCD_DIGIT1_SEG_MASK, data);
  data = (((bitmap & 0x04) >> 2) << LCD_SEG32_SHIFT) |
         (((bitmap & 0x08) >> 3) << LCD_SEG39_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_1_1, LCD_DIGIT1_SEG_MASK, data);
  data = (((bitmap & 0x10) >> 4) << LCD_SEG32_SHIFT) |
         (((bitmap & 0x20) >> 5) << LCD_SEG39_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_2_1, LCD_DIGIT1_SEG_MASK, data);
  data = (((bitmap & 0x40) >> 6) << LCD_SEG32_SHIFT) |
         (((bitmap & 0x80) >> 7) << LCD_SEG39_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_3_1, LCD_DIGIT1_SEG_MASK, data);
}

void Screen_DisplaySymbol2(Screen_SegmentBitmap_t bitmap) {
  uint32_t data = ((bitmap & 0x01) << LCD_SEG30_SHIFT) |
                  (((bitmap & 0x02) >> 1) << LCD_SEG17_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_0_0, LCD_DIGIT2_SEG_MASK_1, data);
  data = ((bitmap & 0x01) << LCD_SEG42_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_0_1, LCD_DIGIT2_SEG_MASK_2, data);
  data = (((bitmap & 0x04) >> 2) << LCD_SEG30_SHIFT) |
         (((bitmap & 0x08) >> 3) << LCD_SEG17_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_1_0, LCD_DIGIT2_SEG_MASK_1, data);
  data = (((bitmap & 0x04) >> 2) << LCD_SEG42_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_1_1, LCD_DIGIT2_SEG_MASK_2, data);
  data = (((bitmap & 0x10) >> 4) << LCD_SEG30_SHIFT) |
         (((bitmap & 0x20) >> 5) << LCD_SEG17_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_2_0, LCD_DIGIT2_SEG_MASK_1, data);
  data = (((bitmap & 0x10) >> 4) << LCD_SEG42_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_2_1, LCD_DIGIT2_SEG_MASK_2, data);
  data = (((bitmap & 0x40) >> 6) << LCD_SEG30_SHIFT) |
         (((bitmap & 0x80) >> 7) << LCD_SEG17_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_3_0, LCD_DIGIT2_SEG_MASK_1, data);
  data = (((bitmap & 0x40) >> 6) << LCD_SEG42_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_3_1, LCD_DIGIT2_SEG_MASK_2, data);
}

void Screen_DisplaySymbol3(Screen_SegmentBitmap_t bitmap) {
  uint32_t data = ((bitmap & 0x01) << LCD_SEG7_SHIFT) |
                  (((bitmap & 0x02) >> 1) << LCD_SEG29_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_0_0, LCD_DIGIT3_SEG_MASK_1, data);
  data = (((bitmap & 0x02) >> 1) << LCD_SEG41_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_0_1, LCD_DIGIT3_SEG_MASK_2, data);
  data = (((bitmap & 0x04) >> 2) << LCD_SEG7_SHIFT) |
         (((bitmap & 0x08) >> 3) << LCD_SEG29_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_1_0, LCD_DIGIT3_SEG_MASK_1, data);
  data = (((bitmap & 0x08) >> 3) << LCD_SEG41_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_1_1, LCD_DIGIT3_SEG_MASK_2, data);
  data = (((bitmap & 0x10) >> 4) << LCD_SEG7_SHIFT) |
         (((bitmap & 0x20) >> 5) << LCD_SEG29_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_2_0, LCD_DIGIT3_SEG_MASK_1, data);
  data = (((bitmap & 0x20) >> 5) << LCD_SEG41_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_2_1, LCD_DIGIT3_SEG_MASK_2, data);
  data = (((bitmap & 0x40) >> 6) << LCD_SEG7_SHIFT) |
         (((bitmap & 0x80) >> 7) << LCD_SEG29_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_3_0, LCD_DIGIT3_SEG_MASK_1, data);
  data = (((bitmap & 0x80) >> 7) << LCD_SEG41_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_3_1, LCD_DIGIT3_SEG_MASK_2, data);
}

void Screen_DisplaySymbol4(Screen_SegmentBitmap_t bitmap) {
  uint32_t data = ((bitmap & 0x01) << LCD_SEG9_SHIFT) |
                  (((bitmap & 0x02) >> 1) << LCD_SEG8_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_0_0, LCD_DIGIT4_SEG_MASK, data);
  data = (((bitmap & 0x04) >> 2) << LCD_SEG9_SHIFT) |
         (((bitmap & 0x08) >> 3) << LCD_SEG8_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_1_0, LCD_DIGIT4_SEG_MASK, data);
  data = (((bitmap & 0x10) >> 4) << LCD_SEG9_SHIFT) |
         (((bitmap & 0x20) >> 5) << LCD_SEG8_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_2_0, LCD_DIGIT4_SEG_MASK, data);
  data = (((bitmap & 0x40) >> 6) << LCD_SEG9_SHIFT) |
         (((bitmap & 0x80) >> 7) << LCD_SEG8_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_3_0, LCD_DIGIT4_SEG_MASK, data);
}

void Screen_DisplaySymbol5(Screen_SegmentBitmap_t bitmap) {
  uint32_t data = ((bitmap & 0x01) << LCD_SEG14_SHIFT) |
                  (((bitmap & 0x02) >> 1) << LCD_SEG12_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_0_0, LCD_DIGIT5_SEG_MASK, data);
  data = (((bitmap & 0x04) >> 2) << LCD_SEG14_SHIFT) |
         (((bitmap & 0x08) >> 3) << LCD_SEG12_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_1_0, LCD_DIGIT5_SEG_MASK, data);
  data = (((bitmap & 0x10) >> 4) << LCD_SEG14_SHIFT) |
         (((bitmap & 0x20) >> 5) << LCD_SEG12_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_2_0, LCD_DIGIT5_SEG_MASK, data);
  data = (((bitmap & 0x40) >> 6) << LCD_SEG14_SHIFT) |
         (((bitmap & 0x80) >> 7) << LCD_SEG12_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_3_0, LCD_DIGIT5_SEG_MASK, data);
}

void Screen_DisplaySymbol6(Screen_SegmentBitmap_t bitmap) {
  uint32_t data = (((bitmap & 0x02) >> 1) << LCD_SEG15_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_0_0, LCD_DIGIT6_SEG_MASK_1, data);
  data = ((bitmap & 0x01) << LCD_SEG33_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_0_1, LCD_DIGIT6_SEG_MASK_2, data);
  data = (((bitmap & 0x08) >> 3) << LCD_SEG15_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_1_0, LCD_DIGIT6_SEG_MASK_1, data);
  data = (((bitmap & 0x04) >> 2) << LCD_SEG33_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_1_1, LCD_DIGIT6_SEG_MASK_2, data);
  data = (((bitmap & 0x20) >> 5) << LCD_SEG15_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_2_0, LCD_DIGIT6_SEG_MASK_1, data);
  data = (((bitmap & 0x10) >> 4) << LCD_SEG33_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_2_1, LCD_DIGIT6_SEG_MASK_2, data);
  data = (((bitmap & 0x80) >> 7) << LCD_SEG15_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_3_0, LCD_DIGIT6_SEG_MASK_1, data);
  data = (((bitmap & 0x40) >> 6) << LCD_SEG33_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_3_1, LCD_DIGIT6_SEG_MASK_2, data);
}

void Screen_DisplaySymbol7(Screen_SegmentBitmap_t bitmap) {
  uint32_t data = ((uint32_t)(bitmap & 0x01) << LCD_SEG31_SHIFT) |
                  (((bitmap & 0x02) >> 1) << LCD_SEG27_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_0_0, LCD_DIGIT7_SEG_MASK_1, data);
  data = ((bitmap & 0x01) << LCD_SEG43_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_0_1, LCD_DIGIT7_SEG_MASK_2, data);
  data = (((uint32_t)(bitmap & 0x04) >> 2) << LCD_SEG31_SHIFT) |
         (((bitmap & 0x08) >> 3) << LCD_SEG27_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_1_0, LCD_DIGIT7_SEG_MASK_1, data);
  data = (((bitmap & 0x04) >> 2) << LCD_SEG43_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_1_1, LCD_DIGIT7_SEG_MASK_2, data);
  data = (((uint32_t)(bitmap & 0x10) >> 4) << LCD_SEG31_SHIFT) |
         (((bitmap & 0x20) >> 5) << LCD_SEG27_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_2_0, LCD_DIGIT7_SEG_MASK_1, data);
  data = (((bitmap & 0x10) >> 4) << LCD_SEG43_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_2_1, LCD_DIGIT7_SEG_MASK_2, data);
  data = (((uint32_t)(bitmap & 0x40) >> 6) << LCD_SEG31_SHIFT) |
         (((bitmap & 0x80) >> 7) << LCD_SEG27_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_3_0, LCD_DIGIT7_SEG_MASK_1, data);
  data = (((bitmap & 0x40) >> 6) << LCD_SEG43_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_3_1, LCD_DIGIT7_SEG_MASK_2, data);
}

void Screen_DisplaySymbol8(Screen_SegmentBitmap_t bitmap) {
  uint32_t data = (((bitmap & 0x02) >> 1) << LCD_SEG24_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_0_0, LCD_DIGIT8_SEG_MASK, data);  // 8D
  data = (((bitmap & 0x04) >> 2) << LCD_SEG13_SHIFT) |
         (((bitmap & 0x08) >> 3) << LCD_SEG24_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_1_0, LCD_DIGIT8_SEG_MASK, data);  // 8E, 8C
  data = (((bitmap & 0x10) >> 4) << LCD_SEG13_SHIFT) |
         (((bitmap & 0x20) >> 5) << LCD_SEG24_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_2_0, LCD_DIGIT8_SEG_MASK, data);  // 8F, 8G
  data = (((bitmap & 0x40) >> 6) << LCD_SEG13_SHIFT) |
         (((bitmap & 0x80) >> 7) << LCD_SEG24_SHIFT);
  HAL_LCD_Write(&gLcd, LCD_COM_3_0, LCD_DIGIT8_SEG_MASK, data);  // 8A, 8B
}

void Screen_DisplayFourDigits(int32_t value,
                              Screen_DisplaySymbolCb_t* row,
                              Screen_DisplaySegmentCb_t sign) {
  int absValue = abs(value);

  uint8_t position = 0;
  uint8_t digit = 0;
  for (; position < 3; position++) {
    digit = absValue % 10;
    row[position](gScreen_Digit[digit]);
    absValue /= 10;
  }
  digit = absValue % 10;
  if (digit == 0) {  // no significant digit anymore
    row[position](SCREEN_BLANK);
    if (value < 0) {
      row[position](SCREEN_MINUS);  // display a -
    }
    return;
  }
  // print last digit and sign if needed
  row[position](gScreen_Digit[digit]);
  sign(value < 0);
}

void Screen_DisplayMinusTop(bool on) {
  HAL_LCD_Write(&gLcd, LCD_COM_1_0, ~(1U << LCD_SEG22_SHIFT),
                (on << LCD_SEG22_SHIFT));
}

void Screen_DisplayMinusBottom(bool on) {
  HAL_LCD_Write(&gLcd, LCD_COM_0_0, ~(1U << LCD_SEG22_SHIFT),
                (on << LCD_SEG22_SHIFT));
}

void Screen_DisplayPoint1(bool on) {
  HAL_LCD_Write(&gLcd, LCD_COM_0_1, ~(1U << LCD_SEG32_SHIFT),
                (uint32_t)on << LCD_SEG32_SHIFT);
}

void Screen_DisplayPoint2(bool on) {
  HAL_LCD_Write(&gLcd, LCD_COM_0_1, ~(1U << LCD_SEG42_SHIFT),
                (uint32_t)on << LCD_SEG42_SHIFT);
}

void Screen_DisplayPoint3(bool on) {
  HAL_LCD_Write(&gLcd, LCD_COM_0_0, ~(1U << LCD_SEG7_SHIFT),
                (uint32_t)on << LCD_SEG7_SHIFT);
}

void Screen_DisplayPoint5(bool on) {
  HAL_LCD_Write(&gLcd, LCD_COM_0_0, ~(1U << LCD_SEG14_SHIFT),
                (uint32_t)on << LCD_SEG14_SHIFT);
}

void Screen_DisplayPoint6(bool on) {
  HAL_LCD_Write(&gLcd, LCD_COM_0_1, ~(1U << LCD_SEG33_SHIFT),
                (uint32_t)on << LCD_SEG33_SHIFT);
}

void Screen_DisplayPoint7(bool on) {
  HAL_LCD_Write(&gLcd, LCD_COM_0_1, ~(1U << LCD_SEG43_SHIFT),
                (uint32_t)on << LCD_SEG43_SHIFT);
}

void Screen_DisplayCelsius1(bool on) {
  HAL_LCD_Write(&gLcd, LCD_COM_1_0, ~(1U << LCD_SEG20_SHIFT),
                (uint32_t)on << LCD_SEG20_SHIFT);
}

void Screen_DisplayFahrenheit1(bool on) {
  HAL_LCD_Write(&gLcd, LCD_COM_3_0, ~(1U << LCD_SEG0_SHIFT),
                (uint32_t)on << LCD_SEG0_SHIFT);
}

void Screen_DisplayCelsius2(bool on) {
  HAL_LCD_Write(&gLcd, LCD_COM_0_0, ~(1U << LCD_SEG20_SHIFT),
                (uint32_t)on << LCD_SEG20_SHIFT);
}

void Screen_DisplayFahrenheit2(bool on) {
  HAL_LCD_Write(&gLcd, LCD_COM_0_0, ~(1U << LCD_SEG25_SHIFT),
                (uint32_t)on << LCD_SEG25_SHIFT);
}

void Screen_DisplayRh(bool on) {
  HAL_LCD_Write(&gLcd, LCD_COM_2_0, ~(1U << LCD_SEG20_SHIFT),
                (uint32_t)on << LCD_SEG20_SHIFT);
}

void Screen_DisplayBluetoothSymbol(bool on) {
  HAL_LCD_Write(&gLcd, LCD_COM_3_0, ~(1U << LCD_SEG22_SHIFT),
                (on << LCD_SEG22_SHIFT));
}

void Screen_DisplayCmoSens(bool on) {
  HAL_LCD_Write(&gLcd, LCD_COM_2_0, ~(1U << LCD_SEG22_SHIFT),
                (on << LCD_SEG22_SHIFT));
}

void Screen_DisplayLowBattery(bool on) {
  HAL_LCD_Write(&gLcd, LCD_COM_0_0, ~(1U << LCD_SEG13_SHIFT),
                (uint32_t)on << LCD_SEG13_SHIFT);
}

void Screen_DisplayDewPointSymbol(bool on) {
  HAL_LCD_Write(&gLcd, LCD_COM_0_0, ~(1U << LCD_SEG9_SHIFT),
                (on << LCD_SEG9_SHIFT));
}

void Screen_UpdatePendingRequests() {
  // HAL_LCD_UpdateDisplayRequest(&gLcd) blocks until the
  // request is completed - this is not required and takes a lot
  // of time in which the system stays running

  // Clear the Update Display Done flag before starting the update request
  __HAL_LCD_CLEAR_FLAG(&gLcd, LCD_FLAG_UDD);

  // Trigger the request without waiting on completion!
  gLcd.Instance->SR |= LCD_SR_UDR;
}

void Screen_ClearAll() {
  HAL_LCD_Clear(&gLcd);
}

/// LCD MSP Initialization
///
/// This function configures the hardware resources used in this example
///
/// @param hlcd: LCD handle pointer
void HAL_LCD_MspInit(LCD_HandleTypeDef* hlcd) {
  GPIO_InitTypeDef gpioInitStruct = {0};
  RCC_PeriphCLKInitTypeDef periphClkInitStruct = {0};
  if (hlcd->Instance == LCD) {
    // Initializes the peripherals clock
    periphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    periphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    if (HAL_RCCEx_PeriphCLKConfig(&periphClkInitStruct) != HAL_OK) {
      ErrorHandler_UnrecoverableError(ERROR_CODE_HARDWARE);
    }

    // Peripheral clock enable
    __HAL_RCC_LCD_CLK_ENABLE();
    Gpio_InitClocks();

    // PA0     ------> LCD_SEG0
    // PA8     ------> LCD_COM0
    // PA9     ------> LCD_COM1
    // PA10    ------> LCD_COM2
    // PA15    ------> LCD_SEG17
    // PB3     ------> LCD_SEG7
    // PB4     ------> LCD_SEG8
    // PB5     ------> LCD_SEG9
    // PB9     ------> LCD_COM3
    // PB12    ------> LCD_SEG12
    // PB13    ------> LCD_SEG13
    // PB14    ------> LCD_SEG14
    // PB15    ------> LCD_SEG15
    // PC2     ------> LCD_SEG20
    // PC4     ------> LCD_SEG22
    // PC6     ------> LCD_SEG24
    // PC7     ------> LCD_SEG25
    // PC9     ------> LCD_SEG27
    // PC11    ------> LCD_SEG29
    // PC12    ------> LCD_SEG30
    // PD2     ------> LCD_SEG31
    // PD7     ------> LCD_SEG39
    // PD12    ------> LCD_SEG32
    // PD13    ------> LCD_SEG33
    gpioInitStruct.Pin =
        GPIO_PIN_1 | GPIO_PIN_15 | GPIO_PIN_10 | GPIO_PIN_8 | GPIO_PIN_9;
    gpioInitStruct.Mode = GPIO_MODE_AF_PP;
    gpioInitStruct.Pull = GPIO_NOPULL;
    gpioInitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    gpioInitStruct.Alternate = GPIO_AF11_LCD;
    HAL_GPIO_Init(GPIOA, &gpioInitStruct);

    gpioInitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_3 |
                         GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15 | GPIO_PIN_12;
    gpioInitStruct.Mode = GPIO_MODE_AF_PP;
    gpioInitStruct.Pull = GPIO_NOPULL;
    gpioInitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    gpioInitStruct.Alternate = GPIO_AF11_LCD;
    HAL_GPIO_Init(GPIOB, &gpioInitStruct);

    gpioInitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_2 | GPIO_PIN_11 | GPIO_PIN_12 |
                         GPIO_PIN_6 | GPIO_PIN_4 | GPIO_PIN_9 | GPIO_PIN_7;
    gpioInitStruct.Mode = GPIO_MODE_AF_PP;
    gpioInitStruct.Pull = GPIO_NOPULL;
    gpioInitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    gpioInitStruct.Alternate = GPIO_AF11_LCD;
    HAL_GPIO_Init(GPIOC, &gpioInitStruct);

    gpioInitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_12 | GPIO_PIN_7 | GPIO_PIN_2;
    gpioInitStruct.Mode = GPIO_MODE_AF_PP;
    gpioInitStruct.Pull = GPIO_NOPULL;
    gpioInitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    gpioInitStruct.Alternate = GPIO_AF11_LCD;
    HAL_GPIO_Init(GPIOD, &gpioInitStruct);

    // enable voltage buffer
    __HAL_LCD_VOLTAGE_BUFFER_ENABLE(hlcd);
    // LCD interrupt Init
    HAL_NVIC_SetPriority(LCD_IRQn, IRQ_PRIO_APP, 0);
    HAL_NVIC_EnableIRQ(LCD_IRQn);
  }
}
/// LCD MSP De-Initialization
///
/// This function freeze the hardware resources used in this example
///
/// @param hlcd: LCD handle pointer
void HAL_LCD_MspDeInit(LCD_HandleTypeDef* hlcd) {
  if (hlcd->Instance == LCD) {
    // Peripheral clock disable
    __HAL_RCC_LCD_CLK_DISABLE();

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_0 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_15);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_12 |
                               GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);

    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_6 | GPIO_PIN_7 |
                               GPIO_PIN_9 | GPIO_PIN_11 | GPIO_PIN_12);

    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_2 | GPIO_PIN_7 | GPIO_PIN_12 | GPIO_PIN_13);

    // LCD interrupt DeInit
    HAL_NVIC_DisableIRQ(LCD_IRQn);
  }
}
