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
/// @file SysTest.h
///
/// Here we can list test cases that need to be executed on application
/// level.

#ifndef SYS_TEST_H
#define SYS_TEST_H

#include "hal/Uart.h"
#include "utility/StaticCodeAnalysisHelper.h"
#include "utility/scheduler/MessageBroker.h"

/// Defines the actually available test groups
/// This enum is meant for documentation purposes and will
/// be extended in the future.
typedef enum {
  SYS_TEST_TEST_GROUP_SCREEN = 0,
  SYS_TEST_TEST_GROUP_PRESENTATION = 1
} SysTest_TestGroups;

/// Generic data structure that is given to test functions as argument
typedef union _uSysTest_TestMessage {
  uint8_t byteParameter[4];    ///< 8bit access to the data
  uint16_t shortParameter[2];  ///< 16bit access to the data
  uint32_t longParameter;      ///< 32bit access to the data
} SysTest_TestMessageParameter_t;

/// Signature of any test function that can be hooked up to the test controller
typedef void (*SysTest_TestFunctionCb_t)(SysTest_TestMessageParameter_t param);

/// A message to hold information about test
/// The contents of the message are not yet well defined.
/// Its a showcase how to extend messages.
typedef struct _tSysTest_Message {
  MessageBroker_MsgHead_t head;         ///< Generic message header
  SysTest_TestMessageParameter_t data;  ///< Data to be passed as arguments to
                                        ///< the specified test function.
} SysTest_Message_t;

ASSERT_SIZE_TYPE1_LESS_THAN_TYPE2(SysTest_Message_t, uint64_t);

/// Return an initialized instance to the system test controller
/// @return Pointer to a message listener
MessageListener_Listener_t* SysTest_TestControllerInstance();

/// Get an object that may receive data from the UART
/// @return a receiver object
Uart_Receiver_t* SysTest_GetUartReceiver();

#endif  // SYS_TEST_H
