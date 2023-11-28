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

/// @file Log.h
///
/// The module Log provides simple macros to print log messages.
///
#ifndef LOG_H
#define LOG_H

#include "Trace.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#if defined(DEBUG) && !defined(STATIC_CODE_ANALYSIS)
/// write a debug message to trace output
#define LOG_DEBUG(message, ...)                                            \
  snprintf(Trace_GetMessageFormatBuffer(), TRACE_FMT_BUFFER_SIZE, message, \
           __VA_ARGS__);                                                   \
  Trace_Message("Debug: %s\n", Trace_GetMessageFormatBuffer())

#else
/// LOG_DEBUG is eliminated in release builds
#define LOG_DEBUG(message, ...)

#endif

#if defined(STATIC_CODE_ANALYSIS)

/// clang tidy is not aware of the proper compiler architecture and will
/// produce warnings
#define LOG_ERROR(message, ...) Trace_Message(message, __VA_ARGS__)

/// clang tidy is not aware of the proper compiler architecture and will
/// produce warnings
#define LOG_INFO(message, ...) Trace_Message(message, __VA_ARGS__)

#else  // no static code analysis

/// write a error message to trace output
#define LOG_ERROR(message, ...)                                            \
  snprintf(Trace_GetMessageFormatBuffer(), TRACE_FMT_BUFFER_SIZE, message, \
           __VA_ARGS__);                                                   \
  Trace_Message("Error: %s\n", Trace_GetMessageFormatBuffer())

/// write a info message to trace output
#define LOG_INFO(message, ...)                                             \
  snprintf(Trace_GetMessageFormatBuffer(), TRACE_FMT_BUFFER_SIZE, message, \
           __VA_ARGS__);                                                   \
  Trace_Message("Info: %s\n", Trace_GetMessageFormatBuffer())
#endif  // STATIC_CODE_ANALYSIS

#endif  // LOG_H
