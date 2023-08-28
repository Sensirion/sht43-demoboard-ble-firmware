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
/// @file SysTest.c
///

#include "SysTest.h"

#include "hal/Qspi.h"
#include "test/AdcTest.h"
#include "test/CyclicBufferTest.h"
#include "test/FlashTest.h"
#include "test/ListTest.h"
#include "test/PresentationTest.h"
#include "test/QspiTest.h"
#include "test/ScreenTest.h"
#include "test/TraceTest.h"
#include "utility/ErrorHandler.h"
#include "utility/log/Log.h"

#include <memory.h>
#include <stdint.h>

/// Count the number of elements in an array
#define COUNT_OF(x) (sizeof(x) / sizeof(x)[0])

/// Offset of the parameter within the 6 bytes received from the uart receiver
#define MSG_PARAM_OFFSET 2

/// Size of the parameters within the received data
#define MSG_PARAM_SIZE 4

/// DefaultHandler of message
/// @param message the message to be handled
/// @return true if the message was handled, false otherwise.
static bool TestControllerIdle(Message_Message_t* message);

/// Default uart receive handler of the SysTest controller
/// @param actualLength received length
static void HandleReceivedCb(uint16_t actualLength);

/// Execute and assert to check non recoverable errors
/// this test must not be included in a ci test sequence.
static void AssertTest();

/// test controller
/// this test controller may react (in the future) on messages coming
/// from the PC.
MessageListener_Listener_t _SysTestController = {
    .currentMessageHandlerCb = TestControllerIdle,
    .receiveMask = MESSAGE_BROKER_CATEGORY_TEST};

/// receive buffer for uart data
static uint8_t _uartRxBuffer[8];

/// Uart receiver structure
Uart_Receiver_t _UartReceiver = {.receiveBuffer = _uartRxBuffer,
                                 .rxLength = 6,
                                 .ReceiveDataCb = HandleReceivedCb};

/// Test functions to test the LCD screen
static SysTest_TestFunctionCb_t _screenTestFunctions[] = {
    ScreenTest_TestDisplaySymbol, ScreenTest_TestSegmentBitmaps};

/// Test functions to test and modify the Presentation controller behavior
static SysTest_TestFunctionCb_t _presentationTestFunctions[] = {
    PresentationTest_SetTimeStep, PresentationTest_TriggerPowerStateChange};

/// Test functions to test the LCD screen
static SysTest_TestFunctionCb_t _flashTestFunctions[] = {
    FlashTest_Erase, FlashTest_Read, FlashTest_Write};

/// Array with test function pointers
static SysTest_TestFunctionCb_t* _allTests[] = {
    _flashTestFunctions, _screenTestFunctions, _presentationTestFunctions};

/// Have a list with the sizes of all test tables in order to check
/// the access to the test functions
static uint8_t _allTestsSizes[] = {
    COUNT_OF(_flashTestFunctions),
    COUNT_OF(_screenTestFunctions),
    COUNT_OF(_presentationTestFunctions),
};

MessageListener_Listener_t* SysTest_TestControllerInstance() {
  return &_SysTestController;
}

// return the receiver object
Uart_Receiver_t* SysTest_GetUartReceiver() {
  return &_UartReceiver;
}

static void HandleReceivedCb(uint16_t actualLength) {
  if (actualLength == 0xFFFF) {  // an error occurred
    memset(_uartRxBuffer, 0xFF, sizeof(_uartRxBuffer));
    return;
  }

  SysTest_Message_t msg = {.head.category = MESSAGE_BROKER_CATEGORY_TEST,
                           .head.id = _uartRxBuffer[0],
                           .head.parameter1 = _uartRxBuffer[1]};

  memccpy(&msg.data, &_uartRxBuffer[MSG_PARAM_OFFSET], MSG_PARAM_SIZE,
          MSG_PARAM_SIZE);
  Message_PublishAppMessage((Message_Message_t*)&msg);
  memset(_uartRxBuffer, 0xFF, sizeof(_uartRxBuffer));
}

static bool TestControllerIdle(Message_Message_t* msg) {
  SysTest_Message_t message = *(SysTest_Message_t*)msg;
  if (message.head.id >= COUNT_OF(_allTestsSizes)) {
    LOG_INFO("Invalid test group %i", message.head.id);
    return false;
  }
  if (message.head.parameter1 >= _allTestsSizes[message.head.id]) {
    LOG_INFO("Invalid test id %i", message.head.parameter1);
    return false;
  }
  SysTest_TestFunctionCb_t* functionTable =
      (SysTest_TestFunctionCb_t*)_allTests[message.head.id];
  functionTable[message.head.parameter1](message.data);
  LOG_INFO("Test with id %i:%i dispatched", message.head.id,
           message.head.parameter1);
  return true;
}

// this test is strictly manual
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void AssertTest() {
  ASSERT(false);
}
#pragma GCC diagnostic pop
