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
#include "stm32_lpm.h"
#include "stm32wbxx_hal.h"
#include "utility/AppDefines.h"
#include "utility/ErrorHandler.h"

#include <stdint.h>
#include <string.h>

/// helper macro to compute minimum; only used internally!
#define MIN(x, y) (x) > (y) ? (y) : (x)

/// Function pointer that is used hold the callback to notify the
/// client that the flash operation is complete.
static Flash_OperationComplete _flashOperationComplete;

/// Memory structure that holds the erase parameters
static FLASH_EraseInitTypeDef _eraseStruct;

void Flash_Init() {
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);
  HAL_NVIC_SetPriority(FLASH_IRQn, IRQ_PRIO_APP, 0);
  HAL_NVIC_EnableIRQ(FLASH_IRQn);
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
  ASSERT(_flashOperationComplete == 0);  // do not write while erasing
  if (address % 8 != 0) {
    return false;
  }
  uint32_t writeAddress = address;
  uint16_t bytesWritten = 0;
  HAL_FLASH_Unlock();
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
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, writeAddress, data) !=
        HAL_OK) {
      HAL_FLASH_Lock();
      return false;
    }
    writeAddress += sizeof(uint64_t);
    buffer += sizeof(uint64_t);
  } while (bytesWritten < nrOfBytes);
  HAL_FLASH_Lock();
  return true;
}

// start the erase procedure
void Flash_Erase(uint16_t startPageNr,
                 uint8_t nrOfPages,
                 Flash_OperationComplete callback) {
  ASSERT(_flashOperationComplete == 0);
  UTIL_LPM_SetStopMode(1 << APP_DEFINE_LPM_CLIENT_FLASH, UTIL_LPM_DISABLE);
  _flashOperationComplete = callback;
  _eraseStruct.TypeErase = FLASH_TYPEERASE_PAGES;
  _eraseStruct.Page = startPageNr;
  _eraseStruct.NbPages = nrOfPages;
  HAL_FLASH_Unlock();  // without unlocking, nothing happens, even no error!
  HAL_FLASHEx_Erase_IT(&_eraseStruct);
  HAL_FLASH_Lock();
}

/// Called by interrupt handler when operation is finished.
/// This function is called when an erase operation completes.
/// @param parameter write address or page number;
void HAL_FLASH_EndOfOperationCallback(uint32_t parameter) {
  UTIL_LPM_SetStopMode(1 << APP_DEFINE_LPM_CLIENT_FLASH, UTIL_LPM_ENABLE);
  _flashOperationComplete(parameter);
  _flashOperationComplete = 0;
}

/// Called by interrupt handler when an error occurs.
/// This function is called when an error occurs during an erase operation.
/// @param parameter
void HAL_FLASH_OperationErrorCallback(uint32_t parameter) {
  UTIL_LPM_SetStopMode(1 << APP_DEFINE_LPM_CLIENT_FLASH, UTIL_LPM_ENABLE);
  _flashOperationComplete(-1);
  _flashOperationComplete = 0;
}

/// Flash Irq handler
void FLASH_IRQHandler() {
  HAL_FLASH_IRQHandler();
}
