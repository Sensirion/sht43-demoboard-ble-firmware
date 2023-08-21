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

/// @file BleOverrides.c
///
/// Implementation of functions that are used within the ST BLE framework.
/// Function names must not be changed.
///

#include "BleHelper.h"
#include "BleTypes.h"
#include "shci.h"
#include "stm32_seq.h"
#include "stm32wbxx_hal.h"
#include "tl.h"
#include "utility/scheduler/Scheduler.h"

#include <stdint.h>

/// Activates the task that handles asynchronous events from CPU1
///
/// This function may be called in different contexts:
///  - IPCC RX interrupt
///  - hci_user_evt_proc()
///  - hci_resume_flow()
///
/// @param data Packet or event pointer
void hci_notify_asynch_evt(void* data) {
  UTIL_SEQ_SetTask(1 << SCHEDULER_TASK_HANDLE_HCI_EVENT, SCHEDULER_PRIO_0);
}

/// Signals an event to the task that handles asynchronous events from CPU1.
///
/// @param flag: Release flag
void hci_cmd_resp_release(uint32_t flag) {
  UTIL_SEQ_SetEvt(1 << SCHEDULER_EVENT_HCI_CMD_RESPONSE);
}

/// Forces the event handling task to wait until reception of an event
/// (triggered by hci_cmd_resp_release)
///
/// @param timeout: Waiting timeout
void hci_cmd_resp_wait(uint32_t timeout) {
  UTIL_SEQ_WaitEvt(1 << SCHEDULER_EVENT_HCI_CMD_RESPONSE);
}
