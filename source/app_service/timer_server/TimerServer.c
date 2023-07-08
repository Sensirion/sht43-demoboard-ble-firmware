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

/// @file TimerServer.c
///
/// Timer server that provides multiple virtual timers sharing the RTC wake-up
/// timer. Each virtual timer can be defined as either single shot or a
/// repeated timer.
///
/// This implementation was adapted from STs implementation in hw_timerserver.c.

#include "TimerServer.h"

#include "TimerServerHelper.h"
#include "TimerServerRtcInterface.h"
#include "hal/IrqPrio.h"
#include "utility/concurrency/Concurrency.h"

/// Preemption priority of the RTC_WKUP interrupt handler in the NVIC
#define NVIC_RTC_WAKEUP_IT_PREEMPTPRIO 3

/// Preemption subpriority of the RTC_WKUP interrupt handler in the NVIC
#define NVIC_RTC_WAKEUP_IT_SUBPRIO 0

/// Transform milliseconds to ticks of the real time clock
///
/// @param milliseconds milliseconds to be converted to ticks
///
/// @return ticks to count to pass the amount of milliseconds demanded
static uint32_t MillisecondsToTicks(uint32_t milliseconds);

/// Start a virtual timer
///
/// Internally the time server works with ticks. The timer is not only started
/// from external clients but also from within the wakeup handler. Therefore
/// we split the start function into the internal call which works with ticks
/// and an external interface that uses milliseconds.
///
/// @param timerId The ID of the timer to start
/// @param timeoutTicks  Number of milliseconds of the virtual timer
void StartTimer(uint8_t timerId, uint32_t timeoutTicks);

/// Return the first free ID in the timer context list
///
/// @return First free ID in the timer context list
static uint8_t FindFirstFreeId(void);

/// See TimerServer.h for Documentation

/// Global RTC_HandleTypeDef from System.c
static RTC_HandleTypeDef* gRtc;
/// Array storing a timer struct for each time to be configured
static volatile TimerServerHelper_TimerContext_t
    gTimerContext[MAX_NBR_CONCURRENT_TIMER];
/// Variable storing the ID of the current running timer
static volatile uint8_t gCurrentRunningTimerId;
/// Variable storing the ID of the previous running timer
static volatile uint8_t gPreviousRunningTimerId;

void TimerServer_Init(RTC_HandleTypeDef* rtc) {
  // point the static gRtc pointer to the passed rtc variable
  gRtc = rtc;

  TimerServerHelper_Init(gTimerContext, &gCurrentRunningTimerId,
                         &gPreviousRunningTimerId);

  TimerServerRtcInterface_Init(gRtc, gTimerContext, &gCurrentRunningTimerId);

  // Disable the write protection for RTC registers
  __HAL_RTC_WRITEPROTECTION_DISABLE(gRtc);

  // Configure EXTI module
  LL_EXTI_EnableRisingTrig_0_31(RTC_EXTI_LINE_WAKEUPTIMER_EVENT);
  LL_EXTI_EnableIT_0_31(RTC_EXTI_LINE_WAKEUPTIMER_EVENT);

  TimerServerHelper_SetAllTimersFree();

  // Set ID to non valid value
  gCurrentRunningTimerId = MAX_NBR_CONCURRENT_TIMER;

  /// Configure the wakeup timer
  // Disable the wakeup timer
  __HAL_RTC_WAKEUPTIMER_DISABLE(gRtc);
  // Clear pending flags in RTC module
  __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(gRtc, RTC_FLAG_WUTF);
  // Clear flag in EXTI module
  __HAL_RTC_WAKEUPTIMER_EXTI_CLEAR_FLAG();
  // Enable interrupt in RTC module  */
  __HAL_RTC_WAKEUPTIMER_ENABLE_IT(gRtc, RTC_IT_WUT);
  // Enable the write protection for the RTC register
  __HAL_RTC_WRITEPROTECTION_ENABLE(gRtc);

  // Set IRQ settings for RTC wakeup timer
  // Clear pending bit in NVIC
  HAL_NVIC_ClearPendingIRQ(RTC_WKUP_IRQn);
  // Set NVIC priority
  HAL_NVIC_SetPriority(RTC_WKUP_IRQn, IRQ_PRIO_RTC_WAKE_UP,
                       NVIC_RTC_WAKEUP_IT_SUBPRIO);
  // Enable NVIC
  HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);
}

uint8_t TimerServer_CreateTimer(TimerServer_Mode_t timerMode,
                                TimerServer_ElapsedCallback_t timerCallback) {
  uint32_t priMask = Concurrency_EnterCriticalSection();

  uint8_t id = FindFirstFreeId();

  if (id != MAX_NBR_CONCURRENT_TIMER) {
    gTimerContext[id].timerIdStatus = TIMER_ID_CREATED;

    Concurrency_LeaveCriticalSection(priMask);

    gTimerContext[id].mode = timerMode;
    gTimerContext[id].callback = timerCallback;
  } else {
    Concurrency_LeaveCriticalSection(priMask);
  }
  return id;
}

void TimerServer_DeleteTimer(uint8_t timerId) {
  // Release ID
  gTimerContext[timerId].timerIdStatus = TIMER_ID_FREE;
}

void TimerServer_RtcWakeupHandler(void) {
  TimerServer_ElapsedCallback_t callback;
  TimerServerRtcInterface_WakeupTimerLimitationStatus_t wakeupTimerLimitation;
  uint8_t currentRunningTimerId;

  uint32_t priMask = Concurrency_EnterCriticalSection();
  // Disable the write protection for RTC registers
  __HAL_RTC_WRITEPROTECTION_DISABLE(gRtc);

  // Disable the Wakeup Timer
  // This may speed up a bit the processing to wait the timer to be disabled
  // The timer is still counting 2 RTCCLK
  __HAL_RTC_WAKEUPTIMER_DISABLE(gRtc);

  currentRunningTimerId = gCurrentRunningTimerId;

  if (gTimerContext[currentRunningTimerId].timerIdStatus == TIMER_ID_RUNNING) {
    callback = gTimerContext[currentRunningTimerId].callback;

    // It should be good to check whether the TimeElapsed is greater or not than
    // the tick left to be counted. However, due to the inaccuracy of the
    // reading of the time elapsed, it may return there is 1 tick to be left
    // whereas the count is over. A more secure implementation has been done
    // with a flag to state whereas the full count has been written in the
    // wakeup-timer or not
    wakeupTimerLimitation = TimerServerRtcInterface_GetWakeupTimerLimitation();
    if (wakeupTimerLimitation != WAKEUP_TIMER_VALUE_OVERPASSED) {
      if (gTimerContext[currentRunningTimerId].mode ==
          TIMER_SERVER_MODE_REPEATED) {
        TimerServerHelper_UnlinkTimer(currentRunningTimerId);
        Concurrency_LeaveCriticalSection(priMask);

        StartTimer(currentRunningTimerId,
                   gTimerContext[currentRunningTimerId].counterInit);

        // Disable the write protection for RTC registers
        __HAL_RTC_WRITEPROTECTION_DISABLE(gRtc);
      } else {
        Concurrency_LeaveCriticalSection(priMask);
        TimerServer_Stop(currentRunningTimerId);

        // Disable the write protection for RTC registers
        __HAL_RTC_WRITEPROTECTION_DISABLE(gRtc);
      }
      callback();
    } else {
      TimerServerRtcInterface_RescheduleTimerList();
      Concurrency_LeaveCriticalSection(priMask);
    }
  } else {
    // We should never end up in this case
    // However, if due to any bug in the timer server this is the case, the
    // mistake may not impact the user. We could just clean the interrupt flag
    // and get out from this unexpected interrupt
    while (__HAL_RTC_WAKEUPTIMER_GET_FLAG(gRtc, RTC_FLAG_WUTWF) == RESET) {
    }

    // Make sure to clear the pending flags after checking the WUTWF.
    // It takes 2 RTCCLK between the time the WUTE bit is disabled and the
    // time the timer is disabled. The WUTWF bit somehow guarantee the system
    // is stable. Otherwise, when the timer is periodic with 1 Tick, it may
    // generate an extra interrupt in between due to the autoreload feature
    __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(gRtc, RTC_FLAG_WUTF);
    // Clear flag in EXTI module
    __HAL_RTC_WAKEUPTIMER_EXTI_CLEAR_FLAG();

    Concurrency_LeaveCriticalSection(priMask);
  }

  // Enable the write protection for RTC registers
  __HAL_RTC_WRITEPROTECTION_ENABLE(gRtc);
}

void TimerServer_Start(uint8_t timerId, uint32_t timeoutMs) {
  uint32_t timeoutTicks = MillisecondsToTicks(timeoutMs);
  StartTimer(timerId, timeoutTicks);
}

void StartTimer(uint8_t timerId, uint32_t timeoutTicks) {
  uint16_t timeElapsed;
  uint8_t currentRunningTimerId;

  if (gTimerContext[timerId].timerIdStatus == TIMER_ID_RUNNING) {
    TimerServer_Stop(timerId);
  }
  uint32_t priMask = Concurrency_EnterCriticalSection();

  // Disable NVIC
  HAL_NVIC_DisableIRQ(RTC_WKUP_IRQn);

  // Disable the write protection for RTC registers
  __HAL_RTC_WRITEPROTECTION_DISABLE(gRtc);

  gTimerContext[timerId].timerIdStatus = TIMER_ID_RUNNING;

  gTimerContext[timerId].countLeft = timeoutTicks;
  gTimerContext[timerId].counterInit = timeoutTicks;

  if (gCurrentRunningTimerId == MAX_NBR_CONCURRENT_TIMER) {
    timeElapsed = 0;
    TimerServerRtcInterface_SetSsrValueOnLastSetup(SSR_FORBIDDEN_VALUE);
  } else {
    timeElapsed = TimerServerRtcInterface_ReturnTimeElapsed();
  }

  TimerServerHelper_LinkTimer(timerId, timeElapsed);
  currentRunningTimerId = gCurrentRunningTimerId;

  if (gPreviousRunningTimerId != currentRunningTimerId) {
    TimerServerRtcInterface_RescheduleTimerList();
  } else {
    gTimerContext[timerId].countLeft -= timeElapsed;
  }

  // Enable the write protection for RTC registers
  __HAL_RTC_WRITEPROTECTION_ENABLE(gRtc);

  // Enable NVIC
  HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);

  Concurrency_LeaveCriticalSection(priMask);
}

void TimerServer_Stop(uint8_t timerId) {
  uint8_t currentRunningTimerId;

  uint32_t priMask = Concurrency_EnterCriticalSection();

  // Disable NVIC
  HAL_NVIC_DisableIRQ(RTC_WKUP_IRQn);

  // Disable the write protection for RTC registers
  __HAL_RTC_WRITEPROTECTION_DISABLE(gRtc);

  if (gTimerContext[timerId].timerIdStatus == TIMER_ID_RUNNING) {
    TimerServerHelper_UnlinkTimer(timerId);
    if (gCurrentRunningTimerId == MAX_NBR_CONCURRENT_TIMER) {
      TimerServerRtcInterface_SetSsrValueOnLastSetup(SSR_FORBIDDEN_VALUE);
    }
    currentRunningTimerId = gCurrentRunningTimerId;

    if (currentRunningTimerId == MAX_NBR_CONCURRENT_TIMER) {
      //Disable the timer
      if ((READ_BIT(RTC->CR, RTC_CR_WUTE) == (RTC_CR_WUTE)) == SET) {
        // Wait for the flag to be back to 0 when the wakeup timer is enabled
        while (__HAL_RTC_WAKEUPTIMER_GET_FLAG(gRtc, RTC_FLAG_WUTWF) == SET) {
        }
      }
      // Disable the Wakeup Timer
      __HAL_RTC_WAKEUPTIMER_DISABLE(gRtc);

      while (__HAL_RTC_WAKEUPTIMER_GET_FLAG(gRtc, RTC_FLAG_WUTWF) == RESET) {
      }
      // Clear flag in RTC module
      __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(gRtc, RTC_FLAG_WUTF);
      // Clear flag in EXTI module
      __HAL_RTC_WAKEUPTIMER_EXTI_CLEAR_FLAG();
      // Clear pending bit in NVIC
      HAL_NVIC_ClearPendingIRQ(RTC_WKUP_IRQn);
    } else if (gPreviousRunningTimerId != currentRunningTimerId) {
      TimerServerRtcInterface_RescheduleTimerList();
    }
  }

  // Enable the write protection for RTC registers
  __HAL_RTC_WRITEPROTECTION_ENABLE(gRtc);

  //  Enable NVIC
  HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);

  Concurrency_LeaveCriticalSection(priMask);
}

static uint8_t FindFirstFreeId(void) {
  uint8_t id = 0U;
  while ((id < MAX_NBR_CONCURRENT_TIMER) &&
         (gTimerContext[id].timerIdStatus != TIMER_ID_FREE)) {
    id++;
  }
  return id;
}

uint32_t MillisecondsToTicks(uint32_t milliseconds) {
  return (
      uint32_t)((LSE_VALUE / (CFG_RTC_ASYNCH_PRESCALER + 1) * milliseconds) /
                1000);
}
