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

/// @file ScreenDefines.h
///
/// Provide macros for the screen implementation

#ifndef SCREEN_CONF_H
#define SCREEN_CONF_H

#define LCD_SEG0_SHIFT 0U    ///< bit shift for SEG0
#define LCD_SEG7_SHIFT 7U    ///< bit shift for SEG7
#define LCD_SEG8_SHIFT 8U    ///< bit shift for SEG8
#define LCD_SEG9_SHIFT 9U    ///< bit shift for SEG9
#define LCD_SEG12_SHIFT 12U  ///< bit shift for SEG12
#define LCD_SEG13_SHIFT 13U  ///< bit shift for SEG13
#define LCD_SEG14_SHIFT 14U  ///< bit shift for SEG14
#define LCD_SEG15_SHIFT 15U  ///< bit shift for SEG15
#define LCD_SEG17_SHIFT 17U  ///< bit shift for SEG17
#define LCD_SEG20_SHIFT 20U  ///< bit shift for SEG20
#define LCD_SEG22_SHIFT 22U  ///< bit shift for SEG22
#define LCD_SEG24_SHIFT 24U  ///< bit shift for SEG24
#define LCD_SEG25_SHIFT 25U  ///< bit shift for SEG25
#define LCD_SEG27_SHIFT 27U  ///< bit shift for SEG27
#define LCD_SEG29_SHIFT 29U  ///< bit shift for SEG29
#define LCD_SEG30_SHIFT 30U  ///< bit shift for SEG30
#define LCD_SEG31_SHIFT 31U  ///< bit shift for SEG31
#define LCD_SEG32_SHIFT 0U   ///< bit shift for SEG32
#define LCD_SEG33_SHIFT 1U   ///< bit shift for SEG33
#define LCD_SEG39_SHIFT 7U   ///< bit shift for SEG39
#define LCD_SEG41_SHIFT 9U   ///< bit shift for SEG41
#define LCD_SEG42_SHIFT 10U  ///< bit shift for SEG42
#define LCD_SEG43_SHIFT 11U  ///< bit shift for SEG43

#define LCD_COM_0_0 LCD_RAM_REGISTER0  ///< Register Mask for COM0_0
#define LCD_COM_0_1 LCD_RAM_REGISTER1  ///< Register Mask for COM0_1
#define LCD_COM_1_0 LCD_RAM_REGISTER2  ///< Register Mask for COM1_0
#define LCD_COM_1_1 LCD_RAM_REGISTER3  ///< Register Mask for COM1_1
#define LCD_COM_2_0 LCD_RAM_REGISTER4  ///< Register Mask for COM2_0
#define LCD_COM_2_1 LCD_RAM_REGISTER5  ///< Register Mask for COM2_1
#define LCD_COM_3_0 LCD_RAM_REGISTER6  ///< Register Mask for COM3_0
#define LCD_COM_3_1 LCD_RAM_REGISTER7  ///< Register Mask for COM3_1

/// Mask to use to write Segments from 0 to 31 in one command.
#define LCD_ALL_SEG_MASK_1                                         \
  (uint32_t) ~((1U << LCD_SEG0_SHIFT) | (1U << LCD_SEG7_SHIFT) |   \
               (1U << LCD_SEG8_SHIFT) | (1U << LCD_SEG9_SHIFT) |   \
               (1U << LCD_SEG12_SHIFT) | (1U << LCD_SEG13_SHIFT) | \
               (1U << LCD_SEG14_SHIFT) | (1U << LCD_SEG15_SHIFT) | \
               (1U << LCD_SEG17_SHIFT) | (1U << LCD_SEG20_SHIFT) | \
               (1U << LCD_SEG22_SHIFT) | (1U << LCD_SEG24_SHIFT) | \
               (1U << LCD_SEG25_SHIFT) | (1U << LCD_SEG27_SHIFT) | \
               (1U << LCD_SEG29_SHIFT) | (1U << LCD_SEG30_SHIFT) | \
               (1U << LCD_SEG31_SHIFT))

/// Mask to use to write Segments from 32 to 43 in one command.
#define LCD_ALL_SEG_MASK_2                                         \
  (uint32_t) ~((1U << LCD_SEG32_SHIFT) | (1U << LCD_SEG33_SHIFT) | \
               (1U << LCD_SEG39_SHIFT) | (1U << LCD_SEG41_SHIFT) | \
               (1U << LCD_SEG42_SHIFT) | (1U << LCD_SEG43_SHIFT))

/// Mask for digit one (SEG32 and 39 are connected to Pin 21 and 22 on LCD)
#define LCD_DIGIT1_SEG_MASK ~((1U << LCD_SEG32_SHIFT) | (1U << LCD_SEG39_SHIFT))
/// Mask for digit two (SEG30/42 and 17 are connected to Pin 19 and 20 on LCD)
#define LCD_DIGIT2_SEG_MASK_1 \
  ~((1U << LCD_SEG30_SHIFT) | (1U << LCD_SEG17_SHIFT))
/// Mask for digit two (SEG30/42 and 17 are connected to Pin 19 and 20 on LCD)
#define LCD_DIGIT2_SEG_MASK_2 ~(1U << LCD_SEG42_SHIFT)
/// Mask for digit three (SEG7 and 29 are connected to Pin 17 and 18 on LCD)
#define LCD_DIGIT3_SEG_MASK_1 \
  ~((1U << LCD_SEG7_SHIFT) | (1U << LCD_SEG29_SHIFT))
/// Mask for digit three (SEG7 and 29 are connected to Pin 17 and 18 on LCD)
#define LCD_DIGIT3_SEG_MASK_2 ~(1U << LCD_SEG41_SHIFT)
/// Mask for digit four (SEG9 and 8 are connected to Pin 15 and 16 on LCD)
#define LCD_DIGIT4_SEG_MASK ~((1U << LCD_SEG9_SHIFT) | (1U << LCD_SEG8_SHIFT))
/// Mask for digit five (SEG12 and 14 are connected to Pin 2 and 3 on LCD)
#define LCD_DIGIT5_SEG_MASK ~((1U << LCD_SEG12_SHIFT) | (1U << LCD_SEG14_SHIFT))
/// Mask for digit six (SEG15 and 33 are connected to Pin 4 and 5 on LCD)
#define LCD_DIGIT6_SEG_MASK_1 ~(1U << LCD_SEG15_SHIFT)
/// Mask for digit six (SEG15 and 33 are connected to Pin 4 and 5 on LCD)
#define LCD_DIGIT6_SEG_MASK_2 ~(1U << LCD_SEG33_SHIFT)
/// Mask for digit seven (SEG27 and 31 are connected to Pin 6 and 7 on LCD)
#define LCD_DIGIT7_SEG_MASK_1 \
  ~((1U << LCD_SEG27_SHIFT) | (1U << LCD_SEG31_SHIFT))
/// Mask for digit seven (SEG27 and 31 are connected to Pin 6 and 7 on LCD)
#define LCD_DIGIT7_SEG_MASK_2 ~(1U << LCD_SEG43_SHIFT)
/// Mask for digit eight (SEG24 and 13 are connected to Pin 8 and 9 on LCD)
#define LCD_DIGIT8_SEG_MASK ~((1U << LCD_SEG24_SHIFT) | (1U << LCD_SEG13_SHIFT))

#endif  //  SCREEN_CONF_H
