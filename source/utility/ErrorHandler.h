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

/// @file ErrorHandler.h
///
/// The module ErrorHandler defines the ErrorHandling functions and macros of the
/// application.
///

#ifndef __ERROR_HANDLER_H
#define __ERROR_HANDLER_H

/// Macro to assert a condition
#define ASSERT(x)                                               \
  do {                                                          \
    if (!(x)) {                                                 \
      ErrorHandler_UnrecoverableError(ERROR_CODE_SW_TEST_FAIL); \
    }                                                           \
  } while (0)

/// Define the different sources of errors.
typedef enum {
  ERROR_CODE_SW_TEST_FAIL,  ///< A software test failed at a specific location
  ERROR_CODE_HARDWARE,
  ERROR_CODE_TIMEOUT,
  ERROR_CODE_SENSOR_READOUT,
  ERROR_CODE_BLE,
  ERROR_CODE_ITEM_STORE,
} ErrorHandler_ErrorCode_t;

/// Signals an unrecoverable error to the application
///
/// Unrecoverable errors will cause the application to stay in
/// a blocked state. The error code will be shown on the screen if the screen
/// is already initialized.
/// To exit this state, the power has to be removed.
/// @param code Error code that is published
/// @note An unrecoverable error is the last action a system executes before
/// getting stuck. The message propagating this error may only be used for
/// cleanup tasks.
void ErrorHandler_UnrecoverableError(ErrorHandler_ErrorCode_t code);

/// Signals a recoverable error
///
/// A recoverable error may be used to exit a wait state of a state machine
/// On application layer it will be ignored.
/// @param code Error code that is published
void ErrorHandler_RecoverableError(ErrorHandler_ErrorCode_t code);

#endif  // __ERROR_HANDLER_H
