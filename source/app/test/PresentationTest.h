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

/// @file PresentationTest.h
#ifndef PRESENTATION_TEST_H
#define PRESENTATION_TEST_H

#include "app/SysTest.h"

/// Defines the functions of the test group SYS_TEST_TEST_GROUP_PRESENTATION
/// This enum serves the documentation!
typedef enum {
  FUNCTION_ID_TEST_SET_TIME_STEP = 0,
  FUNCTION_ID_TEST_POWER_STATE = 1
} PresentationTest_FunctionId_t;

/// Set the time step of the application
/// @param param Byte[0] of the param sets the time step in seconds.
///              all other bytes are ignored.
void PresentationTest_SetTimeStep(SysTest_TestMessageParameter_t param);

/// Trigger power state change
///
/// Test is used to show the proper behavior of the presentation controller with
/// regards to receiving power state changed events
/// Before the message is started, the step time is set to a big value in order
/// to prevent that the system sends events by it's own.
/// @param param Byte[0] The id of the new power state.
///              all other bytes are ignored.
/// @note After setting power level = 3, the system needs to be rebooted for
/// unrestricted operation.
void PresentationTest_TriggerPowerStateChange(
    SysTest_TestMessageParameter_t param);

#endif  // PRESENTATION_TEST_H
