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

/// @file Screen.h
///
/// Functions to write to the screen of the gadget.
///
/// The screen has different symbols that can be written individually.
/// On one hand there are eight seven segment symbols.
/// These eight segment symbols are used to write digits or letters. The
/// segments are ordered as described below.
///
/// LCD segment mapping:
///
/// @code
///  --A--
///  |   |
///  F   B
///  |   |
///  --G--
///  |   |
///  E   C
///  |   |
///  --D--
/// @endcode
///
/// Each of this segmented symbols is written by a function
/// *Screen_DisplaySymbol<x>* where x is the number or the position of the
/// symbol. The symbol parameter of this function is a bitmap that is used
/// to switch on the required segment on or off.
///
/// The other symbols consist only of one single segment. They show the units
/// (Fahrenheit, Celsius, Relative humidity) or decimal points (Point1 to
/// Point6) or other high level elements eg. a bluetooth symbol.

#ifndef SCREEN_H
#define SCREEN_H

#include <stdbool.h>
#include <stdint.h>

/// Function pointer to a function that displays/hides a single segment of the
/// Lcd.
typedef void (*Screen_DisplaySegmentCb_t)(bool);

/// Enumeration describing the bitmaps of the 7-segment symbol
///
/// Each segment is represented by one bit
/// @code
/// MSB         LSB
/// 7 6 5 4 3 2 1 0
/// A B F G E C D -
/// @endcode
typedef enum {
  SCREEN_0 = 0xEE,      ///< 0
  SCREEN_1 = 0x44,      ///< 1
  SCREEN_2 = 0xDA,      ///< 2
  SCREEN_3 = 0xD6,      ///< 3
  SCREEN_4 = 0x74,      ///< 4
  SCREEN_5 = 0xB6,      ///< 5
  SCREEN_6 = 0xBE,      ///< 6
  SCREEN_7 = 0xC4,      ///< 7
  SCREEN_8 = 0xFE,      ///< 8
  SCREEN_9 = 0xF6,      ///< 9
  SCREEN_BLANK = 0x00,  ///< empty
  SCREEN_MINUS = 0x10,  ///< -
  SCREEN_A = 0xFC,      ///< A (same as R)
  SCREEN_b = 0x3E,      ///< b
  SCREEN_C = 0xAA,      ///< C
  SCREEN_d = 0x5E,      ///< d
  SCREEN_I = 0x44,      ///< I (same as 1)
  SCREEN_R = 0xFC,      ///< R
  SCREEN_E = 0xBA,      ///< E
  SCREEN_F = 0xB8,      ///< F
  SCREEN_L = 0x2A,      ///< L
  SCREEN_S = 0xB6,      ///< S
  SCREEN_r = 0x18,      ///< r
  SCREEN_t = 0x3A,      ///< t
  SCREEN_n = 0x1C,      ///< n
  SCREEN_o = 0x1E,      ///< o
} Screen_SegmentBitmap_t;

/// Function pointer to a display symbol function.
typedef void (*Screen_DisplaySymbolCb_t)(Screen_SegmentBitmap_t);

/// Initialize the LCD peripheral and the screen.
void Screen_Init();

/// Return the segment bitmap for a digit (decimal or hexadecimal)
/// @param digit Number in the range [0-15].
///              For bigger numbers only the lower four bits will be represented
/// @return The corresponding bitmap that can be used to display the digit on
///         the LCD.
Screen_SegmentBitmap_t Screen_DigitToBitmap(uint8_t digit);

/// Display the 7-segment symbol 1.
///
/// The segments of the symbol are selected by the bits in the bitmap
/// parameter.
/// @param bitmap the symbol to be drawn on 7-segment symbol position 1
void Screen_DisplaySymbol1(Screen_SegmentBitmap_t bitmap);

/// Display the 7-segment symbol 2.
///
/// The segments of the symbol are selected by the bits in the bitmap
/// parameter.
/// @param bitmap the symbol to be drawn on 7-segment symbol position 2
void Screen_DisplaySymbol2(Screen_SegmentBitmap_t bitmap);

/// Display the 7-segment symbol 3.
///
/// The segments of the symbol are selected by the bits in the bitmap
/// parameter.
/// @param bitmap the symbol to be drawn on 7-segment symbol position 3
void Screen_DisplaySymbol3(Screen_SegmentBitmap_t bitmap);

/// Display the 7-segment symbol 4.
///
/// The segments of the symbol are selected by the bits in the bitmap
/// parameter.
/// @param bitmap the symbol to be drawn on 7-segment symbol position 4
void Screen_DisplaySymbol4(Screen_SegmentBitmap_t bitmap);

/// Display the 7-segment symbol 5.
///
/// The segments of the symbol are selected by the bits in the bitmap
/// parameter.
/// @param bitmap the symbol to be drawn on 7-segment symbol position 5
void Screen_DisplaySymbol5(Screen_SegmentBitmap_t bitmap);

/// Display the 7-segment symbol 6.
///
/// The segments of the symbol are selected by the bits in the symbol
/// parameter.
/// @param bitmap the symbol to be drawn on 7-segment symbol position 6
void Screen_DisplaySymbol6(Screen_SegmentBitmap_t bitmap);

/// Display the 7-segment symbol 7.
///
/// The segments of the symbol are selected by the bits in the bitmap
/// parameter.
/// @param bitmap the symbol to be drawn on 7-segment symbol position 7
void Screen_DisplaySymbol7(Screen_SegmentBitmap_t bitmap);

/// Display the 7-segment symbol 8.
///
/// The segments of the symbol are selected by the bits in the bitmap
/// parameter.
/// @param bitmap the symbol to be drawn on 7-segment symbol position 8
void Screen_DisplaySymbol8(Screen_SegmentBitmap_t bitmap);

/// Display the Celsius 1 symbol.
///
/// @param on switch the symbol on if on is true else hide it.
void Screen_DisplayCelsius1(bool on);

/// Display the Fahrenheit 1 symbol.
///
/// @param on switch the symbol on if on is true else hide it.
void Screen_DisplayFahrenheit1(bool on);

/// Display the Celsius 2 symbol.
///
/// @param on switch the symbol on if on is true else hide it.
void Screen_DisplayCelsius2(bool on);

/// Display the Fahrenheit 2 symbol.
///
/// @param on switch the symbol on if on is true else hide it.
void Screen_DisplayFahrenheit2(bool on);

/// Display the relative humidity symbol.
///
/// @param on switch the symbol on if on is true else hide it.
void Screen_DisplayRh(bool on);

/// Display the Bluethooth symbol.
///
/// @param on switch the symbol on if on is true else hide it.
void Screen_DisplayBluetoothSymbol(bool on);

/// Display the Dew Point symbol.
///
/// @param on switch the symbol on if on is true else hide it.
void Screen_DisplayDewPointSymbol(bool on);

/// Display sign S2 from screen specification
///
/// @param on flag to indicate if the segment s2 shall be switched on or off
void Screen_DisplayMinusTop(bool on);

/// Display sign S3 from screen specification
///
/// @param on flag to indicate if the segment s3 shall be switched on or off
void Screen_DisplayMinusBottom(bool on);

/// Display the point P1 from screen specification
///
/// @param on flag to indicate if the point P1 shall be switched on or off
void Screen_DisplayPoint1(bool on);

/// Display the point P2 from screen specification
///
/// @param on flag to indicate if the point P2 shall be switched on or off
void Screen_DisplayPoint2(bool on);

/// Display the point P3 from screen specification
///
/// @param on flag to indicate if the point P3 shall be switched on or off
void Screen_DisplayPoint3(bool on);

/// Display the point P5 from screen specification
///
/// @param on flag to indicate if the point P5 shall be switched on or off
void Screen_DisplayPoint5(bool on);

/// Display the point P6 from screen specification
///
/// @param on flag to indicate if the point P6 shall be switched on or off
void Screen_DisplayPoint6(bool on);

/// Display the point P7 from screen specification.
///
/// @param on flag to indicate if the point P7 shall be switched on or off
void Screen_DisplayPoint7(bool on);

/// Display the low battery symbol on the screen.
///
/// @param on Flag to indicate if the low battery symbol
///           shall be switched on or off.
void Screen_DisplayLowBattery(bool on);

/// Display the CMOSens symbol on the screen.
///
/// @param on Flag to indicate if the CMOSens symbol
///           shall be switched on or off.
void Screen_DisplayCmoSens(bool on);

/// Displays at most four digits from the supplied integer value.
///
/// @param value An integer value that shall be displayed.
/// @param row An array containing function pointer to draw each digit.
/// @param sign The function pointer to draw the segment with the sign.
void Screen_DisplayFourDigits(int32_t value,
                              Screen_DisplaySymbolCb_t* row,
                              Screen_DisplaySegmentCb_t sign);

/// Test function to show if all segments of the screen are operational.
void Screen_TurnAllSegmentsOn();

/// Update all pending request on the screen.
void Screen_UpdatePendingRequests();

/// Clear screen
void Screen_ClearAll();

#endif  // SCREEN_H
