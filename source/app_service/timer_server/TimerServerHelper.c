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

/// @file TimerServerHelper.c
///
/// Implementation of the helper functions to link the timer

#include "TimerServerHelper.h"

/// Insert a Timer in the list after the Timer ID specified
///
/// @param  timerId The ID of the Timer
/// @param  refTimerId The ID of the Timer to be linked after
static void LinkTimerAfter(uint8_t timerId, uint8_t refTimerId);

/// Insert a Timer in the list before the ID specified
///
/// @param  timerId The ID of the Timer
/// @param  refTimerId The ID of the Timer to be linked before
static void LinkTimerBefore(uint8_t timerId, uint8_t refTimerId);

/// Pointer to gTimerContext from TimerServer.c
static volatile TimerServerHelper_TimerContext_t* gTimerContext;
/// Pointer to gCurrentRunningTimerId from TimerServer.c
static volatile uint8_t* gCurrentRunningTimerId;
/// Pointer to gPreviousRunningTimerId from TimerServer.c
static volatile uint8_t* gPreviousRunningTimerId;

void TimerServerHelper_Init(
    volatile TimerServerHelper_TimerContext_t* timerContext,
    volatile uint8_t* currentRunningTimerId,
    volatile uint8_t* previousRunningTimerId) {
  gTimerContext = timerContext;
  gCurrentRunningTimerId = currentRunningTimerId;
  gPreviousRunningTimerId = previousRunningTimerId;
}

void TimerServerHelper_LinkTimer(uint8_t timerId, uint16_t timeElapsed) {
  uint32_t timeLeft;
  uint8_t timerIdLookup;
  uint8_t nextId;

  if (*gCurrentRunningTimerId == MAX_NBR_CONCURRENT_TIMER) {
    // No timer in the list
    *gPreviousRunningTimerId = *gCurrentRunningTimerId;
    *gCurrentRunningTimerId = timerId;
    (gTimerContext + timerId)->nextId = MAX_NBR_CONCURRENT_TIMER;
  } else {
    // update count of the timer to be linked
    (gTimerContext + timerId)->countLeft += timeElapsed;
    timeLeft = (gTimerContext + timerId)->countLeft;

    // Search for index where the new timer shall be linked
    if ((gTimerContext + *gCurrentRunningTimerId)->countLeft <= timeLeft) {
      // Search for the ID after the first one
      timerIdLookup = *gCurrentRunningTimerId;
      nextId = (gTimerContext + timerIdLookup)->nextId;
      while ((nextId != MAX_NBR_CONCURRENT_TIMER) &&
             ((gTimerContext + nextId)->countLeft <= timeLeft)) {
        timerIdLookup = (gTimerContext + timerIdLookup)->nextId;
        nextId = (gTimerContext + timerIdLookup)->nextId;
      }

      // Link after the ID
      LinkTimerAfter(timerId, timerIdLookup);
    } else {
      // Link before the first ID
      LinkTimerBefore(timerId, *gCurrentRunningTimerId);
      *gPreviousRunningTimerId = *gCurrentRunningTimerId;
      *gCurrentRunningTimerId = timerId;
    }
  }
}

void TimerServerHelper_UnlinkTimer(uint8_t timerId) {
  uint8_t previousId;
  uint8_t nextId;

  if (timerId == *gCurrentRunningTimerId) {
    *gPreviousRunningTimerId = *gCurrentRunningTimerId;
    *gCurrentRunningTimerId = (gTimerContext + timerId)->nextId;
  } else {
    previousId = (gTimerContext + timerId)->previousId;
    nextId = (gTimerContext + timerId)->nextId;

    (gTimerContext + previousId)->nextId = (gTimerContext + timerId)->nextId;
    if (nextId != MAX_NBR_CONCURRENT_TIMER) {
      (gTimerContext + nextId)->previousId =
          (gTimerContext + timerId)->previousId;
    }
  }

  // Timer is out of the list
  gTimerContext[timerId].timerIdStatus = TIMER_ID_CREATED;
}

void TimerServerHelper_SetAllTimersFree(void) {
  uint8_t i = 0U;
  for (i = 0U; i < MAX_NBR_CONCURRENT_TIMER; i++) {
    (gTimerContext + i)->timerIdStatus = TIMER_ID_FREE;
  }
}

static void LinkTimerAfter(uint8_t timerId, uint8_t refTimerId) {
  uint8_t nextId;

  nextId = (gTimerContext + refTimerId)->nextId;

  if (nextId != MAX_NBR_CONCURRENT_TIMER) {
    (gTimerContext + nextId)->previousId = timerId;
  }
  (gTimerContext + timerId)->nextId = nextId;
  (gTimerContext + timerId)->previousId = refTimerId;
  (gTimerContext + refTimerId)->nextId = timerId;
}

static void LinkTimerBefore(uint8_t timerId, uint8_t refTimerId) {
  uint8_t previousId;

  if (refTimerId != *gCurrentRunningTimerId) {
    previousId = (gTimerContext + timerId)->previousId;

    (gTimerContext + previousId)->nextId = timerId;
    (gTimerContext + timerId)->nextId = refTimerId;
    (gTimerContext + timerId)->previousId = previousId;
    (gTimerContext + refTimerId)->previousId = timerId;
  } else {
    (gTimerContext + timerId)->nextId = refTimerId;
    (gTimerContext + refTimerId)->previousId = timerId;
  }
}
