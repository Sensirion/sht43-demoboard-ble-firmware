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

/// @file SensorController.c
///
/// The sensor controller orchestrates the read and write commands to the SHT
/// sensor.

#ifndef SENSOR_CONTROLLER_H
#define SENSOR_CONTROLLER_H

#include "stm32wbxx_hal.h"
#include "utility/scheduler/MessageListener.h"

#include <stdbool.h>

/// This is the definition of the sensor state machine
/// The controller caches the actual values from the sensor. The representation
/// in units is done in another place.
typedef struct _tSensorController_Controller {
  MessageListener_Listener_t listener;  ///< base class, it listens to messages
  uint8_t consecutiveErrors;            ///< Count errors during successive
                                        ///< sensor requests.
  bool activeReminder;                  ///< flag to indicate that we have an
                                        ///< request that needs to be processed
} SensorController_Controller_t;

/// Initializes the sensor controller upon the first call
/// This instance is kept even when entering low-power
/// @return The initialized Instance of the sensor controller
SensorController_Controller_t* SensorController_Sht4xControllerInstance();

#endif  // SENSOR_CONTROLLER_H
