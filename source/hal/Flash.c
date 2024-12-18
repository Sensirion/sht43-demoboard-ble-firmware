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

/// @file Flash.c
#include "Flash.h"

#include "hal/IrqPrio.h"
#include "hw_conf.h"
#include "shci.h"
#include "stm32_lpm.h"
#include "stm32_seq.h"
#include "stm32wbxx_hal.h"
#include "stm32wbxx_ll_hsem.h"
#include "utility/AppDefines.h"
#include "utility/ErrorHandler.h"
#include "utility/concurrency/Concurrency.h"
#include "utility/scheduler/MessageBroker.h"
#include "utility/scheduler/Scheduler.h"

#include <stdint.h>
#include <string.h>

/// helper macro to compute minimum; only used internally!
#ifndef MIN
#define MIN(x, y) (x) > (y) ? (y) : (x)
#endif

/// the one and only category of this message domain
#define FLASH_MSG_CATEGORY 0x1
/// Number of messages in the message queue of the FlashTask message dispatcher.
#define NR_OF_FLASH_MESSAGES 0x4

/// Messages that may occur while erasing multiple pages.
typedef enum {
  FLASH_MESSAGE_PROCESS_NEXT_PAGE,
} FlashMessageId_t;

/// message handler of the flash task
static MessageBroker_Broker_t _flashMessageDispatcher;

/// queue to hold the messages
static uint64_t _flash_task_messages[NR_OF_FLASH_MESSAGES];

/// Function that handles the incoming flash messages in the FlashTask
/// @param msg Message to be handled
/// @return True if the message is handled, false otherwise.
static bool FlashMessageHandlerCb(Message_Message_t* msg);

/// handler for flash messages
static MessageListener_Listener_t _flashMessageHandler = {
    .receiveMask = FLASH_MSG_CATEGORY,
    .currentMessageHandlerCb = FlashMessageHandlerCb};

/// Process the messages of the _flashMessageDispatcher
static void FlashTask();

/// Trigger the erase of a specific page
/// @param pageNr Page that needs to be erased
static void TriggerNextStart(uint8_t pageNr);

/// Start the erase procedure
/// The erase will be executed in the flash task. It will wait until the flash
/// is accessible.
/// Erasing a single page takes about 20ms.
/// @param pageNr Start the erase of  a single page
static void StartErase(uint8_t pageNr);

/// Wait until the flash is ready for another program or erase operation
static void awaitFlashAccessible();

/// Function pointer that is used hold the callback to notify the
/// client that the flash operation is complete.
static Flash_OperationComplete _flashOperationComplete;

/// Memory structure that holds the erase parameters
static FLASH_EraseInitTypeDef _eraseStruct;

/// Number of pages to erase
static uint16_t _pagesToErase;

void Flash_Init() {
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);
  HAL_NVIC_SetPriority(FLASH_IRQn, IRQ_PRIO_APP, 0);

  // initialize and register flash FSM
  MessageBroker_Create(&_flashMessageDispatcher, _flash_task_messages,
                       NR_OF_FLASH_MESSAGES,
                       SCHEDULER_TASK_HANDLE_FLASH_OPERATION, SCHEDULER_PRIO_2);
  MessageBroker_RegisterListener(&_flashMessageDispatcher,
                                 &_flashMessageHandler);
  UTIL_SEQ_RegTask(_flashMessageDispatcher.taskBitmap, UTIL_SEQ_RFU, FlashTask);
}

bool Flash_Read(uint32_t address, uint8_t* buffer, uint16_t nrOfBytes) {
  if (address % 8 != 0) {
    return false;
  }
  uint32_t readAddress = address;
  uint16_t bytesRead = 0;
  do {
    uint64_t data64 = *(volatile uint64_t*)readAddress;
    memcpy(buffer, &data64, MIN(8, (nrOfBytes - bytesRead)));
    bytesRead += sizeof data64;
    readAddress += sizeof data64;
    buffer += sizeof data64;
  } while (bytesRead < nrOfBytes);
  return true;
}

bool Flash_Write(uint32_t address, const uint8_t* buffer, uint16_t nrOfBytes) {
  ASSERT(address >= MIN_WRITABLE_ADDRESS &&
         (address + nrOfBytes - 1) <= MAX_WRITABLE_ADDRESS);
  ASSERT(_flashOperationComplete == 0);  // do not write while erasing
  if (address % 8 != 0) {
    return false;
  }
  uint32_t writeAddress = address;
  uint16_t bytesWritten = 0;

  // reserve flash accesses for CPU1
  while (LL_HSEM_1StepLock(HSEM, CFG_HW_FLASH_SEMID))
    ;
  do {
    uint64_t data = *((uint64_t*)buffer);
    bytesWritten += sizeof(uint64_t);
    // we write always a multiple of 64 bits. If not all is used by data,
    // the reset is padded with 0!
    if (bytesWritten > nrOfBytes) {
      // note: expression needs to be of type uint64_t
      uint64_t mask = (0x1ULL << (8 * (bytesWritten - nrOfBytes))) - 1;
      data &= ~mask;  // clear the data that is not in buffer;
    }
    awaitFlashAccessible();
    HAL_FLASH_Unlock();
    HAL_StatusTypeDef status =
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, writeAddress, data);
    HAL_FLASH_Lock();
    if (status != HAL_OK) {
      LL_HSEM_ReleaseLock(HSEM, CFG_HW_FLASH_SEMID, 0);
      return false;
    }
    writeAddress += sizeof(uint64_t);
    buffer += sizeof(uint64_t);
  } while (bytesWritten < nrOfBytes);
  LL_HSEM_ReleaseLock(HSEM, CFG_HW_FLASH_SEMID, 0);
  return true;
}

// start the erase procedure
//
// It it not allowed to erase more than one page in a row since this
// would take to long and it would interfere with the radio timing.
// Furthermore it is required to give CPU2 the chance to allow erase operation
// only when at least 25ms no radio activity is done.
// This timing protection is done using the program-erase suspend bits.
void Flash_Erase(uint16_t startPageNr,
                 uint8_t nrOfPages,
                 Flash_OperationComplete callback) {
  ASSERT(startPageNr >= FIRST_WRITABLE_FLASH_PAGE);
  ASSERT((startPageNr + nrOfPages - 1) <= LAST_WRITABLE_FLASH_PAGE);
  ASSERT(_flashOperationComplete == 0);
  _pagesToErase = nrOfPages;
  _flashOperationComplete = callback;

  // reserve flash accesses for CPU1
  while (LL_HSEM_1StepLock(HSEM, CFG_HW_FLASH_SEMID))
    ;

  // prevent enter stop mode
  UTIL_LPM_SetStopMode(1 << APP_DEFINE_LPM_CLIENT_FLASH, UTIL_LPM_DISABLE);

  // enable the irq before starting the erase operation
  NVIC_EnableIRQ(FLASH_IRQn);

  // notify cpu2 about erase activity that may start
  SHCI_C2_FLASH_EraseActivity(ERASE_ACTIVITY_ON);
  TriggerNextStart(startPageNr);
}

// this operation is always called from within the FlashTask
static void StartErase(uint8_t pageNr) {
  _eraseStruct.TypeErase = FLASH_TYPEERASE_PAGES;
  _eraseStruct.Page = pageNr;
  _eraseStruct.NbPages = 1;

  // make sure that no operation is suspended
  awaitFlashAccessible();

  HAL_FLASH_Unlock();  // without unlocking, nothing happens, even no error!
  // trigger the erase operation and wait for interrupt
  HAL_FLASHEx_Erase_IT(&_eraseStruct);
}

static bool FlashMessageHandlerCb(Message_Message_t* msg) {
  if (msg->header.id == FLASH_MESSAGE_PROCESS_NEXT_PAGE) {
    // start the erase of a next flash page
    StartErase(msg->header.parameter1);
    return true;
  }
  return false;
}

static void awaitFlashAccessible() {
  while (LL_FLASH_IsActiveFlag_OperationSuspended())
    ;
}

static void FlashTask() {
  MessageBroker_Run(&_flashMessageDispatcher);
}

static void TriggerNextStart(uint8_t pageNr) {
  Message_Message_t msg = {.header.category = FLASH_MSG_CATEGORY,
                           msg.header.id = FLASH_MESSAGE_PROCESS_NEXT_PAGE};
  msg.header.parameter1 = pageNr;
  MessageBroker_PublishMessage(&_flashMessageDispatcher, &msg);
}

/// Called by interrupt handler when operation is finished.
/// This function is called when an erase operation completes.
/// @param parameter write address or page number;
void HAL_FLASH_EndOfOperationCallback(uint32_t parameter) {
  _pagesToErase--;
  HAL_FLASH_Lock();
  if (_pagesToErase == 0) {
    UTIL_LPM_SetStopMode(1 << APP_DEFINE_LPM_CLIENT_FLASH, UTIL_LPM_ENABLE);
    _flashOperationComplete(parameter, _pagesToErase);
    _flashOperationComplete = 0;

    // erase activity done
    SHCI_C2_FLASH_EraseActivity(ERASE_ACTIVITY_OFF);

    // Disable the interrupt after page erase
    NVIC_DisableIRQ(FLASH_IRQn);

    // release global flash semaphore
    LL_HSEM_ReleaseLock(HSEM, CFG_HW_FLASH_SEMID, 0);
    return;
  }
  TriggerNextStart(parameter + 1);
}

/// Called by interrupt handler when an error occurs.
/// This function is called when an error occurs during an erase operation.
/// @param parameter
void HAL_FLASH_OperationErrorCallback(uint32_t parameter) {
  UTIL_LPM_SetStopMode(1 << APP_DEFINE_LPM_CLIENT_FLASH, UTIL_LPM_ENABLE);
  _flashOperationComplete(parameter, _pagesToErase);
  _flashOperationComplete = 0;

  // erase activity done
  SHCI_C2_FLASH_EraseActivity(ERASE_ACTIVITY_OFF);

  HAL_FLASH_Lock();

  // release semaphore
  LL_HSEM_ReleaseLock(HSEM, CFG_HW_FLASH_SEMID, 0);
}

/// Flash Irq handler
void FLASH_IRQHandler() {
  HAL_FLASH_IRQHandler();
}
