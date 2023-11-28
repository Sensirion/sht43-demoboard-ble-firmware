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

/// @file Scheduler.h
///
/// Header file for definitions to identify tasks, events and
/// priorities that are used with the scheduler.
///
/// All id's have to be in the range [0:31]
/// This code is derived from the type definitions in lib/Core/Inc/app_conf.h

#ifndef SCHEDULER_H
#define SCHEDULER_H

/// This enum describes all tasks that may send a ACI/HCI commands
typedef enum {
  SCHEDULER_TASK_HANDLE_HCI_EVENT,  // this task must not be removed
  SCHEDULER_TASK_HANDLE_BLE_MESSAGE,
  SCHEDULER_LAST_HCICMD_TASK,  // this is the last id of the enum
} Scheduler_HciCmdTaskId_t;

/// This enum describes all tasks that may not send a ACI/HCI commands
typedef enum {
  SCHEDULER_FIRST_NO_HCICMD_TASK =
      SCHEDULER_LAST_HCICMD_TASK - 1,  //first enum item
  SCHEDULER_TASK_HANDLE_SYSTEM_HCI_EVENT,
  SCHEDULER_TASK_HANDLE_FLASH_OPERATION,
  SCHEDULER_TASK_HANDLE_APP_MESSAGES,
  SCHEDULER_LAST_NO_HCI_CMD_TASK  // this is the last id of the enum
} Scheduler_NoHciCmdTaskId_t;

/// This enum describes all the available priorities
/// SCHEDULER_PRIO_0 is the highest priority;
/// Only three priorities are allowed!
typedef enum {
  SCHEDULER_PRIO_0,
  SCHEDULER_PRIO_1,
  SCHEDULER_PRIO_2,
} Scheduler_SchedulerPriority_t;

/// This enum describes the list sequencer events
///
/// A task in the sequencer may wait on such an event and be resumed when
/// the event is signaled.
typedef enum {
  SCHEDULER_EVENT_HCI_CMD_RESPONSE,
  SCHEDULER_EVENT_SYSTEM_HCI_CMD_RESPONSE,
  SCHEDULER_EVENT_FLASH_OP_COMPLETE
} Scheduler_SequencerEvent_t;

#endif  // SCHEDULER_H
