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

/// @file TimerServerHelper.h
///
/// Helper functions used only by the TimerServer. Mostly the functions in this
/// file help to link timer in the list of timers according to structure as
/// documented here: https://confluence/x/ZrZaEQ

#ifndef TIMERSERVERHELPER_H
#define TIMERSERVERHELPER_H

#include "TimerServer.h"

/// Maximum number of virtual timers supported
#define MAX_NBR_CONCURRENT_TIMER 6

/// Typedef for the status of the timer context entry with a certain ID
typedef enum {
  TIMER_ID_FREE,
  TIMER_ID_CREATED,
  TIMER_ID_RUNNING
} TimerServerHelper_TimerIdStatus_t;

/// Definitions to set whether the RTC SSR is requested or not
typedef enum {
  SSR_READ_REQUESTED,
  SSR_READ_NOT_REQUESTED
} TimerServerHelper_RequestReadSsr_t;

/// Typedef for the timer context entry
///
/// The timer context is the central point of the TimerServer. It stores
/// information such as the callback as well as the counts when it elapses.
/// However the central aspect is the nextId and previousId. With these
/// variables it is assured that a list can be constructed in which the timer
/// with the least countLeft is always in the first position, referred to as
/// gCurrentRunningTimer, and all subsequent nextId timers subsequently
/// increase the countLeft number.
typedef struct {
  TimerServer_ElapsedCallback_t callback;  ///< Timer elapsed callback
  uint32_t counterInit;                    ///< Timer counter reload value
  uint32_t countLeft;                      ///< Counts left until re-scheduling
  TimerServerHelper_TimerIdStatus_t
      timerIdStatus;        ///< Current status of the entry
  TimerServer_Mode_t mode;  ///< The mode of the timer
  uint8_t previousId;       ///< ID of the previous timer in the linked list
  uint8_t nextId;           ///< ID of the next timer in the linked list
} TimerServerHelper_TimerContext_t;

/// Initialize the TimerServer helper by parsing all required variables as ptr
///
/// @param timerContext Pointer to gTimerContext
/// @param currentRunningTimerId Pointer to gCurrentRunningTimerId
/// @param previousRunningTimerId Pointer to gPreviousRunningTimerId
void TimerServerHelper_Init(
    volatile TimerServerHelper_TimerContext_t* timerContext,
    volatile uint8_t* currentRunningTimerId,
    volatile uint8_t* previousRunningTimerId);

/// Insert a Timer in the list
///
/// Links the timer with the ID specified to the list. For that it checks when
/// the timer to be linked will expire and inserts it in the gTimerContext list
/// such that the gCurrentRunningTimer has the least countLeft and that the
/// countLeft increases with each mNext timer
///
/// @param timerId The ID of the Timer
/// @param timeElapsed ticks counted since the wakeuptimer was started
void TimerServerHelper_LinkTimer(uint8_t timerId, uint16_t timeElapsed);

/// Remove a Timer from the list
///
/// Removing a timer from list and assures that the structure of gTimerContext
/// remains such that gCurrentRunningTimer is the timer with the least
/// countLeft and it increases with each mNext timer.
///
/// @param  timerId The ID of the Timer
void TimerServerHelper_UnlinkTimer(uint8_t timerId);

/// Set all timer to ID status TIMER_ID_FREE
void TimerServerHelper_SetAllTimersFree(void);

#endif  // TIMERSERVERHELPER_H
