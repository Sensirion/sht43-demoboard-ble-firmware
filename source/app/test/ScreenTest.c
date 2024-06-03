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
/// @file ScreenTest.c

#include "ScreenTest.h"

#include "app/SysTest.h"
#include "app_service/screen/Screen.h"
#include "app_service/user_button/Button.h"
#include "utility/log/Log.h"

/// Array with all **DisplaySymbol** functions. This data is used
/// within different tests.
Screen_DisplaySymbolCb_t _displaySymbols[] = {
    Screen_DisplaySymbol1, Screen_DisplaySymbol2, Screen_DisplaySymbol3,
    Screen_DisplaySymbol4, Screen_DisplaySymbol5, Screen_DisplaySymbol6,
    Screen_DisplaySymbol7, Screen_DisplaySymbol8,
};

void ScreenTest_TestDisplaySymbol(SysTest_TestMessageParameter_t param) {
  if (param.byteParameter[0] >= 8) {
    LOG_INFO("Invalid function selection %i", param.byteParameter[0] + 1);
    return;
  }
  uint8_t symbolIndex = param.byteParameter[0];
  if (param.byteParameter[1] > 15) {
    LOG_INFO("Invalid number selection %i", param.byteParameter[1]);
    return;
  }
  if (param.byteParameter[2] > 0) {
    Screen_ClearAll();
  }
  uint8_t displayDigit = param.byteParameter[1];
  LOG_INFO("display number %i on symbol %i", displayDigit, symbolIndex);
  _displaySymbols[param.byteParameter[0]](
      Screen_DigitToBitmap(param.byteParameter[1]));
  Screen_UpdatePendingRequests();
  HAL_Delay(500);
}

void ScreenTest_TestSegmentBitmaps(SysTest_TestMessageParameter_t _) {
  Screen_DisplaySymbol1(SCREEN_t);
  Screen_DisplaySymbol2(SCREEN_E);
  Screen_DisplaySymbol3(SCREEN_S);
  Screen_DisplaySymbol4(SCREEN_t);

  Screen_UpdatePendingRequests();
  for (uint8_t i = 0; i < 16; i++) {
    LOG_INFO(" display symbol %i\n", i);
    Screen_DisplaySymbol5(SCREEN_I);
    Screen_DisplaySymbol6(SCREEN_d);
    Screen_DisplayPoint3(false);
    Screen_DisplayPoint5(false);
    Screen_DisplaySymbol7(SCREEN_MINUS);

    Screen_DisplaySymbol8(Screen_DigitToBitmap(i));
    Screen_DisplayLowBattery(true);
    Screen_UpdatePendingRequests();
    HAL_Delay(1000);
  }
  Screen_DisplayLowBattery(true);
  Screen_UpdatePendingRequests();
  HAL_Delay(1000);
  Screen_DisplayLowBattery(false);
  Screen_UpdatePendingRequests();
}
