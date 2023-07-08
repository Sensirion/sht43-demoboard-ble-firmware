////////////////////////////////////////////////////////////////////////////////
//  S E N S I R I O N   AG,  Laubisruetistr. 50, CH-8712 Staefa, Switzerland
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023, Sensirion AG
// All rights reserved.
// The content of this code is confidential. Redistribution and use in source
// and binary forms, with or without modification, are not permitted.
//
// THIS CODE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////

/// @file TimerServer.h
///
/// Public interface provided by the Real Time Clock (RTC) and more precise
/// the wake-up timer (WUT). All timers are stored in a list. The length of the
/// list can be adjusted in TimerServer.c

#ifndef TIMERSERVER_H
#define TIMERSERVER_H

#include "stm32wbxx_hal.h"

/// Asynchronous prescaler value of the RTC
///
/// It should as high as possible (to output clock as low as possible) to
/// minimize the power consumption (RM0434 p. 914), but the output clock should
/// be equal or higher frequency compare to the clock feeding the wakeup timer.
/// A lower clock speed would impact the accuracy of the TimerServer.
#define CFG_RTC_ASYNCH_PRESCALER (0x0F)

/// Synchronous prescaler value of the RTC
///
/// This sets the synchronous prescaler of the RTC.
/// When the 1Hz calendar clock is required, it shall be sets according to other
/// settings. When the 1Hz calendar clock is not needed, CFG_RTC_SYNCH_PRESCALER
///  should be set to 0x7FFF (MAX VALUE)
#define CFG_RTC_SYNCH_PRESCALER (0x7FFF)

/// Typedef for the callback to be called when a timer elapsed
typedef void (*TimerServer_ElapsedCallback_t)(void);

/// Typedef for the mode of a timer
typedef enum {
  /// Single-shot timer mode
  ///
  /// The timer it is not automatically restarted when the timeout occurs.
  /// However, the timer is kept reserved in the list and could be restarted
  /// anytime with #TimerServer_Start.
  TIMER_SERVER_MODE_SINGLE_SHOT,
  /// Repeated timer mode
  ///
  /// The timer is automatically restarted when the timeout occurs.
  TIMER_SERVER_MODE_REPEATED,
} TimerServer_Mode_t;

/// Initialize the timer server
///
/// Reads the user configuration of the RTC and adjusts the TimerServer
/// accordingly. Additional all possible timers in the list are set to state
/// FREE and the wakeup timer interrupt is set up.
///
/// @param rtc RTC Handle
void TimerServer_Init(RTC_HandleTypeDef* rtc);

/// Create a virtual timer
///
/// Creates a virtual timer if at least one timer is free. The id of the timer
/// is distributed automatically.
///
/// @param mode         The mode of the timer
/// @param callback     The callback to be executed once the timer has elapsed
/// @return The identifier of the timer given by the timer server
uint8_t TimerServer_CreateTimer(TimerServer_Mode_t mode,
                                TimerServer_ElapsedCallback_t callback);

/// Function to delete a virtual timer
///
/// Deletes the timer with the given timerId and sets it to state free such that
/// a new timer can be Created  in the place of the deleted one.
///
/// @param timerId The identifier of the timer
void TimerServer_DeleteTimer(uint8_t timerId);

/// Schedule the timer list on the timer interrupt handler
///
/// This interrupt handler shall be called by the application in the RTC
/// interrupt handler. This handler takes care of clearing all status flag
/// required in the RTC and EXTI peripherals
void TimerServer_RtcWakeupHandler(void);

/// Start a virtual timer
///
/// The timeout value is specified and may be different each time. When the
/// timer is in the single shot mode, it will move to the pending state when it
/// expires. The user may restart it at any time with a different timeout value.
/// When the timer is in the repeated mode, it always stays in running state
/// and restarts automatically with the same timeout value.
/// Timeout values are specified in milliseconds. The virtual timers are not
/// well suited to achieve more fine grained timeout values.
///
/// This API shall not be called on a running timer.
///
/// @param timerId The ID of the timer to start
/// @param timeoutMs  Number of milliseconds of the virtual timer
void TimerServer_Start(uint8_t timerId, uint32_t timeoutMs);

/// Stop a virtual timer
///
/// A timer which is stopped is moved to the pending state. A pending timer may
/// be restarted at any time with a different timeout value but the mode cannot
/// be changed. Nothing is done when it is called to stop a timer which has been
/// already stopped
///
/// @param  timerId Id of the timer to stop
void TimerServer_Stop(uint8_t timerId);

#endif  // TIMERSERVER_H
