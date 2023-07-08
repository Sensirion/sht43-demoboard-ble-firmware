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

/// @file TimerServerRtcInterface.h
///
/// Interface used only by the timerserver with helping functions to reschedule
/// the RTC wakeup timer.

#ifndef TIMERSERVERRTCINTERFACE_H
#define TIMERSERVERRTCINTERFACE_H

#include "TimerServerHelper.h"
#include "stm32wbxx_hal.h"

/// Forbidden value for SSR (SubSeconds Register)
#define SSR_FORBIDDEN_VALUE 0xFFFFFFFF

/// Definitions for the runtime state of the current timer
typedef enum {
  WAKEUP_TIMER_VALUE_OVERPASSED,
  WAKEUP_TIMER_VALUE_LARGE_ENOUGH
} TimerServerRtcInterface_WakeupTimerLimitationStatus_t;

/// Initialize the TimerServerRtcInterface by parsing all required variables as
/// ptr
///
/// @param rtc Pointer to the RTC_HandleTypeDef
/// @param timerContext Pointer to the Timer Context array
/// @param currentRunningTimerId Pointer to the Id of the current running timer
void TimerServerRtcInterface_Init(
    RTC_HandleTypeDef* rtc,
    volatile TimerServerHelper_TimerContext_t* timerContext,
    volatile uint8_t* currentRunningTimerId);

/// The the sub-second register value
///
/// @param ssrValueOnLastSetup
void TimerServerRtcInterface_SetSsrValueOnLastSetup(
    uint32_t ssrValueOnLastSetup);

/// Get the wakeup timer limitation status (overpassed or large enough)
///
/// This value helps to schedule the RTC WKUP register. In the
/// initialization of the TimerServer, the maximal delay is set. This variable
/// helps to guarantee that this value is kept at all times.
///
/// @retval Current status of the wakeup timer limitation
TimerServerRtcInterface_WakeupTimerLimitationStatus_t
TimerServerRtcInterface_GetWakeupTimerLimitation();

/// Return the number of ticks counted by the wakeuptimer since it was started
///
/// The API is reading the SSR register to get how many ticks have been counted
/// since the time the timer has been started
///
/// @retval Time expired in Ticks
uint16_t TimerServerRtcInterface_ReturnTimeElapsed(void);

/// Reschedule the list of timer
///
/// 1) Update the count left for each timer in the list
/// 2) Setup the wakeuptimer
void TimerServerRtcInterface_RescheduleTimerList(void);

#endif  // TIMERSERVERRTCINTERFACE_H
