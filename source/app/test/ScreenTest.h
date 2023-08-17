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
/// @file ScreenTest.h

#ifndef SCREEN_TEST_H
#define SCREEN_TEST_H

#include "app/SysTest.h"

/// Defines the functions of the test group SYS_TEST_TEST_GROUP_SCREEN
/// This enum serves the documentation!
typedef enum {
  FUNCTION_ID_TEST_DISPLAY_SYMBOL = 0,
  FUNCTION_ID_TEST_SEGMENT_BITMAPS
} ScreenTest_FunctionId_t;

/// Test the seven segment symbols 1 to 8 on the screen
/// The id iof this function
/// @param param byte 0: is the symbol to be tested [0..7]
///              byte 1: is the number to be displayed (in hex)
///              byte 2: if 0, the screen is not cleared, else it is cleared.
void ScreenTest_TestDisplaySymbol(SysTest_TestMessageParameter_t param);

/// Combined test of screen
/// @param param The parameter is not used
void ScreenTest_TestSegmentBitmaps(SysTest_TestMessageParameter_t param);

#endif  // SCREEN_TEST_H
