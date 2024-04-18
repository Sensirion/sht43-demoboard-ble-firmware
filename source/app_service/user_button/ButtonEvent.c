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

/// @file ButtonEvent.c
///
/// Implementation of user button

#include "Button.h"
#include "app_service/timer_server/TimerServer.h"
#include "stm32wbxx_ll_gpio.h"
#include "utility/ErrorHandler.h"
#include "utility/log/Trace.h"

#include <stdbool.h>
#include <stdint.h>

// Send a button event with message id = BUTTON_EVENT_LONG_PRESS
void ButtonEvent_PublishLongPressEvent() {
  Button_Message_t msg = {.head.category = MESSAGE_BROKER_CATEGORY_BUTTON_EVENT,
                          .head.id = BUTTON_EVENT_LONG_PRESS};
  Message_PublishAppMessage((Message_Message_t*)&msg);
}

// Send a button event with message id = BUTTON_EVENT_SHORT_PRESS
void ButtonEvent_PublishShortPressEvent() {
  Button_Message_t msg = {.head.category = MESSAGE_BROKER_CATEGORY_BUTTON_EVENT,
                          .head.id = BUTTON_EVENT_SHORT_PRESS};
  Message_PublishAppMessage((Message_Message_t*)&msg);
}

// Send a button event with message id = BUTTON_EVENT_DOUBLE_CLICK
void ButtonEvent_PublishDoubleClickEvent() {
  Button_Message_t msg = {.head.category = MESSAGE_BROKER_CATEGORY_BUTTON_EVENT,
                          .head.id = BUTTON_EVENT_DOUBLE_CLICK};
  Message_PublishAppMessage((Message_Message_t*)&msg);
}

void ButtonEvent_TestLongPressHandler() {
  Trace_Message("received button long press\n");
}

void ButtonEvent_TestButtonPressHandler() {
  Trace_Message("received button press\n");
}
