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

/// @file Button.c
///
/// Implementation of user button

#include "Button.h"

#include "app_service/timer_server/TimerServer.h"
#include "hal/Gpio.h"
#include "stm32wbxx_ll_gpio.h"
#include "utility/ErrorHandler.h"
#include "utility/concurrency/Concurrency.h"
#include "utility/log/Log.h"

#include <stdbool.h>
#include <stdint.h>

/// defines threshold when debouncing shall stop
#define BUTTON_UNSTABLE_THRESHOLD_TM 100

/// defines threshold when the button is considered to be stably pressed
#define BUTTON_PRESSED_THRESHOLD_CNT 3
/// defines threshold when the button is considered to be long pressed
#define BUTTON_LONG_PRESSED_THRESHOLD_CNT 1000  // set to ~ 2 seconds
/// defines threshold when the button is considered to stably released
#define BUTTON_RELEASE_THRESHOLD_CNT 3
/// define threshold when a click is considered a single click and not a
/// double click
#define BUTTON_PRESS_DONE_THRESHOLD_CNT 50
/// defines button monitoring interval
#define MONITORING_INTERVAL_2MS 2
/// defines a threshold where the monitoring will stop even when
/// the button remains pressed.
#define STICKY_BUTTON_THRESHOLD_TM 5000

/// State information of the button service
static struct {
  uint16_t timerId;  ///< id of used monitoring timer

  uint16_t buttonLowInRow;  ///< Counts how long the button was down
  uint16_t buttonUpInRow;   ///< Counts how long the button was up
  uint16_t monitoringTime;  ///< Overall observation time; used to prevent
                            ///< buttons that bounce forever.
  void (*monitoringStateHandler)(bool);  ///< Current state handler of the
                                         ///< button monitoring FSM.
  Button_EventHandlerCb_t buttonLongPressHandler;  ///< Handler of long press
  Button_EventHandlerCb_t buttonPressHandler;      ///< Handler of short press
  Button_EventHandlerCb_t buttonDblClickHandler;   ///< Handle double click
} gButtonState;

/// Exit Signal handler callback
///
/// This is the entry stated of the button state. It transitions to
/// the HandleButtonActive state.
static void HandleButtonIdle();

/// TimerServer tick handler
///
/// The button handler state machine runs within the TimerServer callbacks
static void HandleButtonActive();

/// Initial state of the button handler FSM
/// @param buttonPressed state of the button
static void Debouncing(bool buttonPressed);

/// ButtonPressed state of the button handler FSM
///
/// A stable button press has been detected
/// @param buttonPressed state of the button
static void ButtonPressed(bool buttonPressed);

/// ButtonReleased state of the button handler FSM
///
/// After a stable button press has been detected the
/// the button is released again. We may now detect
/// a double click or wait until final release.
///
/// @param buttonPressed state of the button
static void ButtonReleased(bool buttonPressed);

/// ButtonLongPressed state of the button handler FSM
///
/// The button was pressed for more than BUTTON_LONG_PRESSED_THRESHOLD_CNT
/// monitoring steps.
/// @param buttonPressed state of the button
static void ReleaseLongPressed(bool buttonPressed);

/// Terminates the button handler state machine
/// @param handler The handler that shall be called when the monitoring stops.
///     The handler may be 0 in case the listener was already called.
static void StopMonitoringAndSignal(Button_EventHandlerCb_t handler);

void Button_Init(Button_EventHandlerCb_t pressHandler,
                 Button_EventHandlerCb_t longPressHandler,
                 Button_EventHandlerCb_t doubleClickHandler) {
  LOG_DEBUG("Button_Init()");
  uint32_t priMask = Concurrency_EnterCriticalSection();
  gButtonState.buttonLongPressHandler = longPressHandler;
  gButtonState.buttonPressHandler = pressHandler;
  gButtonState.buttonDblClickHandler = doubleClickHandler;

  Gpio_RegisterOnExtiSignalPc10(HandleButtonIdle);
  gButtonState.timerId =
      TimerServer_CreateTimer(TIMER_SERVER_MODE_REPEATED, HandleButtonActive);
  Concurrency_LeaveCriticalSection(priMask);
}

static void HandleButtonIdle() {
  gButtonState.buttonLowInRow = 0;
  gButtonState.buttonUpInRow = 0;
  gButtonState.monitoringTime = 0;
  Gpio_UnregisterOnExtiSignalPc10();

  gButtonState.monitoringStateHandler = Debouncing;
  TimerServer_Start(gButtonState.timerId, MONITORING_INTERVAL_2MS);
  gButtonState.monitoringStateHandler(!Gpio_IsPc10Set());
}

static void HandleButtonActive() {
  gButtonState.monitoringTime++;
  gButtonState.monitoringStateHandler(!Gpio_IsPc10Set());
}

static void Debouncing(bool buttonPressed) {
  // debouncing state: we did not get a stable signal yet.
  // filter the events and wait unit the button state is stable
  //
  if (buttonPressed) {
    gButtonState.buttonLowInRow += 1;
    gButtonState.buttonUpInRow = 0;
  } else {
    gButtonState.buttonUpInRow += 1;
    gButtonState.buttonLowInRow = 0;
  }

  // we got a stable button pressed signal
  if (gButtonState.buttonLowInRow > BUTTON_PRESSED_THRESHOLD_CNT) {
    gButtonState.monitoringStateHandler = ButtonPressed;
    return;
  }

  // if the button state is not stable after a long time,
  // we stop monitoring anyway.
  if (gButtonState.monitoringTime > BUTTON_UNSTABLE_THRESHOLD_TM) {
    StopMonitoringAndSignal(0);
    return;
  }
}

static void ButtonPressed(bool buttonPressed) {
  if (buttonPressed) {
    gButtonState.buttonLowInRow += 1;
  } else {
    gButtonState.buttonUpInRow += 1;
  }
  if (gButtonState.buttonLowInRow > BUTTON_LONG_PRESSED_THRESHOLD_CNT) {
    if (gButtonState.buttonLongPressHandler != 0) {
      gButtonState.buttonLongPressHandler();
    }
    // we want to have a save release procedure
    // the release may bounce as well.
    gButtonState.monitoringStateHandler = ReleaseLongPressed;
    return;
  }
  if (gButtonState.buttonUpInRow > BUTTON_RELEASE_THRESHOLD_CNT) {
    gButtonState.buttonLowInRow = 0;
    gButtonState.buttonUpInRow = 0;
    gButtonState.monitoringStateHandler = ButtonReleased;
    return;
  }
}
static void ButtonReleased(bool buttonPressed) {
  if (buttonPressed) {
    gButtonState.buttonLowInRow += 1;
  } else {
    gButtonState.buttonUpInRow += 1;
  }
  // uint16_t buttonLow = gButtonState.buttonLowInRow;
  if (gButtonState.buttonUpInRow > BUTTON_PRESS_DONE_THRESHOLD_CNT) {
    if (gButtonState.buttonLowInRow == 0) {
      StopMonitoringAndSignal(gButtonState.buttonPressHandler);
    } else {
      StopMonitoringAndSignal(gButtonState.buttonDblClickHandler);
    }
  }
}

static void ReleaseLongPressed(bool buttonPressed) {
  if (buttonPressed) {
    gButtonState.buttonLowInRow += 1;
  } else {
    gButtonState.buttonUpInRow += 1;
  }

  if (gButtonState.buttonUpInRow > BUTTON_RELEASE_THRESHOLD_CNT ||
      gButtonState.monitoringTime > STICKY_BUTTON_THRESHOLD_TM) {
    StopMonitoringAndSignal(0);
    return;
  }
}

static void StopMonitoringAndSignal(Button_EventHandlerCb_t handler) {
  TimerServer_Stop(gButtonState.timerId);
  Gpio_RegisterOnExtiSignalPc10(HandleButtonIdle);
  if (handler != 0) {
    handler();
  }
}
