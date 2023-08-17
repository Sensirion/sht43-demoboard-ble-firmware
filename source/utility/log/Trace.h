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

/// @file Trace.h
///
/// Header file to support tracing in the application to the UART peripheral

#ifndef TRACE_H
#define TRACE_H

#include <stdarg.h>
#include <stdint.h>

/// Size of the trace fmt buffer that can be used externally
#define TRACE_FMT_BUFFER_SIZE 256

/// Function pointer to trace data
typedef void (*Trace_TraceFunctionCb_t)(const uint8_t* data, uint16_t length);

/// Initialize the UART peripheral
/// @param tracer Function to trace data.
void Trace_Init(Trace_TraceFunctionCb_t tracer);

/// Register a new TranceFunctionCb
/// This function is used to change the behavior of the tracer. E.g when
/// disabling the UART, the Trace_Dump will be used to trace.
/// @param newTraceFunctionCb A callback to handle access to the output device.
/// @return returns the previously registered TraceFunctionCb.
Trace_TraceFunctionCb_t Trace_RegisterTraceFunction(
    Trace_TraceFunctionCb_t newTraceFunctionCb);

/// Trace a message by sending the according message via UART
///
/// @param message Message to be sent including format characters
/// @param ... arguments to the format string
void Trace_Message(const char* message, ...);

/// Get access to the message format buffer to format a message before it is
/// written to the output.
/// The message buffer has a size of **MESSAGE_FMT_BUFFER_SIZE**.
/// @return pointer to the message buffer
char* Trace_GetMessageFormatBuffer();

/// Trace a message including the arguments parsed as va_list
///
/// @param message Message to be traced
/// @param args pointer to arguments to be included in the message
void Trace_VaMessage(const char* message, va_list args);

/// Dump trace messages
/// This function can be used used when tracing to UART is disabled.
/// @param data Data to be dumped.
/// @param length length of the data
void Trace_DevNull(const uint8_t* data, uint16_t length);

#endif  // TRACE_H
