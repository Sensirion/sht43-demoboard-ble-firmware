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

/// @file FlashTest.c
#include "FlashTest.h"

#include "hal/Flash.h"
#include "hal/Uart.h"
#include "utility/log/Log.h"

#include <string.h>

/// Start address of the embedded flash
#define FLASH_START 0x08000000
/// Start address of the flash test
#define FLASH_TEST_START (FLASH_START + 0x40000)
/// Flash test page
#define FLASH_TEST_PAGE_0 64

/// A pattern that is written to flash
static uint64_t _testPattern = 0x89ABCDEFFEDCBA98;

/// The buffer that is used in tests for writing and reading data
static uint8_t _testBuffer[32];

/// Callback to indicate the completion of the erase operation
/// @param pageId the page id that was erased.
/// @param remaining number of pages that have not been erased
///                  on a successful operation this should be 0.
static void EraseDoneCallback(uint32_t pageId, uint8_t remaining);

void FlashTest_Erase(SysTest_TestMessageParameter_t param) {
  Flash_Erase(FLASH_TEST_PAGE_0 + param.byteParameter[0],
              param.byteParameter[1], EraseDoneCallback);
}

void FlashTest_Read(SysTest_TestMessageParameter_t param) {
  uint32_t startAddress = param.shortParameter[0] + FLASH_TEST_START;
  uint32_t nrOfBytes = param.shortParameter[1];
  memset(_testBuffer, 0, sizeof(_testBuffer));
  if (!Flash_Read(startAddress, _testBuffer, nrOfBytes)) {
    LOG_INFO("read failed %li", nrOfBytes);
  }
  Uart_WriteBlocking(_testBuffer, nrOfBytes);
}

void FlashTest_Write(SysTest_TestMessageParameter_t param) {
  uint32_t nrOfBytes = param.shortParameter[1];
  for (uint32_t i = 0; i < nrOfBytes; i += sizeof(_testPattern)) {
    memcpy(_testBuffer + i, &_testPattern, sizeof(_testPattern));
  }
  if (!Flash_Write(FLASH_TEST_START + param.shortParameter[0], _testBuffer,
                   param.shortParameter[1])) {
    LOG_INFO("write failed %li", nrOfBytes);
  }
  Uart_WriteBlocking(_testBuffer, nrOfBytes);
}

static void EraseDoneCallback(uint32_t pageId, uint8_t remaining) {
  UNUSED(remaining);
  LOG_INFO("Erase done %li", pageId);
}
