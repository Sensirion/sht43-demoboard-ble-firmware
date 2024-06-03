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
/// @file QspiTest.c
///

#include "QspiTest.h"

#include "hal/Qspi.h"
#include "stm32_seq.h"
#include "utility/log/Log.h"
#include "utility/scheduler/Scheduler.h"

#include <stdint.h>
#include <string.h>

/// buffer for receiving data from flash
static uint8_t gTestDataBuffer[256];

/// storage for status register 1; can be assigned in the callback
/// and is needed to enable Quad mode on the Flash.
static uint8_t gStatusReg1;

/// storage for status register 2; can be assigned in the callback
/// and is needed to enable Quad mode on the Flash.
static uint8_t gStatusReg2;

/// Handle instruction written that trigger a read
static void QspiInstructionReadResponseCb();

/// Signal operation complete
static void QspiSignalFlashOperationCompleteCb();

void QspiTest_W25Q80ReadUuid() {
  Qspi_WriteInstruction(0x4b, 0,  // receive uuid
                        QSPI_INSTRUCTION_DATA_SIZE_FOUR_BYTE, 8,
                        QspiInstructionReadResponseCb);
  UTIL_SEQ_WaitEvt(1 << SCHEDULER_EVENT_FLASH_OP_COMPLETE);
  LOG_INFO("Uuid = %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\n",
           gTestDataBuffer[0], gTestDataBuffer[1], gTestDataBuffer[2],
           gTestDataBuffer[3], gTestDataBuffer[4], gTestDataBuffer[5],
           gTestDataBuffer[6], gTestDataBuffer[7]);
}

void QspiTest_W25Q80EnterPowerDown() {
  Qspi_WriteInstruction(0xB9, 0,  // power down command
                        QSPI_INSTRUCTION_DATA_SIZE_NONE, 0,
                        QspiSignalFlashOperationCompleteCb);
  UTIL_SEQ_WaitEvt(1 << SCHEDULER_EVENT_FLASH_OP_COMPLETE);
}

void QspiTest_W25Q80LeavePowerDown() {
  Qspi_WriteInstruction(0xAB, 0,  // power down command
                        QSPI_INSTRUCTION_DATA_SIZE_NONE, 0,
                        QspiSignalFlashOperationCompleteCb);
  UTIL_SEQ_WaitEvt(1 << SCHEDULER_EVENT_FLASH_OP_COMPLETE);
}

void QspiTest_W25Q80WriteEnable() {
  Qspi_WriteInstruction(0x06, 0,  // write enable
                        QSPI_INSTRUCTION_DATA_SIZE_NONE, 0,
                        QspiSignalFlashOperationCompleteCb);
  UTIL_SEQ_WaitEvt(1 << SCHEDULER_EVENT_FLASH_OP_COMPLETE);
  LOG_INFO("Write enable done\n");
}

/// Read status register 1
///
/// Needed in many flash operations
void QspiTest_W25Q80ReadStatusRegister1() {
  Qspi_WriteInstruction(0x05, 0,  // read status reg 1
                        QSPI_INSTRUCTION_DATA_SIZE_NONE, 1,
                        QspiSignalFlashOperationCompleteCb);
  UTIL_SEQ_WaitEvt(1 << SCHEDULER_EVENT_FLASH_OP_COMPLETE);
  gStatusReg1 = gTestDataBuffer[0];
  LOG_INFO("Status register 1 = %i\n", gStatusReg1);
}

/// Read status register 2
///
/// Needed in many flash operations
void QspiTest_W25Q80ReadStatusRegister2() {
  Qspi_WriteInstruction(0x35, 0,  // read status reg 2
                        QSPI_INSTRUCTION_DATA_SIZE_NONE, 1,
                        QspiInstructionReadResponseCb);
  UTIL_SEQ_WaitEvt(1 << SCHEDULER_EVENT_FLASH_OP_COMPLETE);
  gStatusReg2 = gTestDataBuffer[0];
  LOG_INFO("Status register 2 = %i\n", gStatusReg2);
}

/// Write status register 1 and 2
///
/// Needed to enable quad mode in flash
void QspiTest_W25Q80EnableQuadOperation() {
  QspiTest_W25Q80ReadStatusRegister1();
  QspiTest_W25Q80ReadStatusRegister2();
  QspiTest_W25Q80WriteEnable();
  uint32_t newValue = (gStatusReg1) | ((gStatusReg2 | 0x2) << 8);
  Qspi_WriteInstruction(0x01, newValue, QSPI_INSTRUCTION_DATA_SIZE_TWO_BYTE, 0,
                        QspiSignalFlashOperationCompleteCb);
  UTIL_SEQ_WaitEvt(1 << SCHEDULER_EVENT_FLASH_OP_COMPLETE);
}

void QspiTest_W25Q80FastRead() {
  uint32_t startAddress = 0x0;
  Qspi_QuadInitiateBulkTransfer(
      0x6b, (uint8_t*)&startAddress, 3, gTestDataBuffer, sizeof gTestDataBuffer,
      8, QSPI_TRANSFER_DIRECTION_READ, QspiSignalFlashOperationCompleteCb);
  UTIL_SEQ_WaitEvt(1 << SCHEDULER_EVENT_FLASH_OP_COMPLETE);
  for (uint16_t i = 0; i < 256; i++) {
    if (i % 16 == 0 && i > 0) {
      LOG_INFO("%02x\n", gTestDataBuffer[i]);
    } else {
      LOG_INFO("%02x", gTestDataBuffer[i]);
    }
  }
}

/// Program flash page
void QspiTest_W25Q80ProgramPage() {
  QspiTest_W25Q80WriteEnable();
  for (uint16_t i = 0; i < 256; i++) {
    gTestDataBuffer[i] = i;
  }
  uint32_t startAddress = 0x0;
  Qspi_QuadInitiateBulkTransfer(
      0x32, (uint8_t*)&startAddress, 3, gTestDataBuffer, sizeof gTestDataBuffer,
      0, QSPI_TRANSFER_DIRECTION_WRITE, QspiSignalFlashOperationCompleteCb);
  UTIL_SEQ_WaitEvt(1 << SCHEDULER_EVENT_FLASH_OP_COMPLETE);
  QspiTest_W25Q80FastRead();
}

static void QspiInstructionReadResponseCb() {
  Qspi_ReadInstructionData(gTestDataBuffer, QspiSignalFlashOperationCompleteCb);
}

static void QspiSignalFlashOperationCompleteCb() {
  UTIL_SEQ_SetEvt(1 << SCHEDULER_EVENT_FLASH_OP_COMPLETE);
}

void QspiTest_RequestReleaseRequest() {
  LOG_INFO("uuid before release\n\t");
  QspiTest_W25Q80ReadUuid();
  Qspi_Release();
  LOG_INFO("uuid after release\n\t");
  QspiTest_W25Q80ReadUuid();
}
