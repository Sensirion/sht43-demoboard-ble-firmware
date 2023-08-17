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

/// @file Trace.c
///
/// Implementation to support tracing in the application to the UART peripheral

#include "Trace.h"

#include "stm32wbxx_hal.h"
#include "utility/ErrorHandler.h"
#include "utility/concurrency/Concurrency.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

/// Trace function
static Trace_TraceFunctionCb_t _traceFunction = 0;

/// Buffer that can be used to prepare messages before sending to trace
static char _MessageFormatBuffer[TRACE_FMT_BUFFER_SIZE];

/// Trace buffer that is passed to the trace function
static char _TraceOutputBuffer[256];

// Register trace target function
void Trace_Init(Trace_TraceFunctionCb_t traceFunction) {
  _traceFunction = traceFunction;
}

// Overwrite registered trace function callback
Trace_TraceFunctionCb_t Trace_RegisterTraceFunction(
    Trace_TraceFunctionCb_t newTraceFunctionCb) {
  Trace_TraceFunctionCb_t oldCb = _traceFunction;
  uint32_t prioMask = Concurrency_EnterCriticalSection();
  _traceFunction = newTraceFunctionCb;
  Concurrency_LeaveCriticalSection(prioMask);
  return oldCb;
}

char* Trace_GetMessageFormatBuffer() {
  return _MessageFormatBuffer;
}

void Trace_Message(const char* message, ...) {
  va_list args;
  va_start(args, message);
  Trace_VaMessage(message, args);
  va_end(args);
}

void Trace_VaMessage(const char* message, va_list args) {
  vsnprintf(_TraceOutputBuffer, sizeof(_TraceOutputBuffer), message, args);
  uint16_t stringLength = strlen(_TraceOutputBuffer);
  ASSERT(stringLength < sizeof(_TraceOutputBuffer));
  _traceFunction((uint8_t*)_TraceOutputBuffer, stringLength);
}

// just do nothing with the data!
void Trace_DevNull(const uint8_t* data, uint16_t length) {
}
