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

/// @file Gpio.h
///
/// Functions to configure the clocks of the system according to the needs of
/// the application.

#ifndef GPIO_H
#define GPIO_H

#include "stm32wbxx_ll_gpio.h"

#include <stdbool.h>

/// Alias for GPIO PC10
#define UserButton_Pin GPIO_PIN_10

/// Enumeration to refer to the debug pins of the demo board.
/// These pins are externally available and can be attached to a logic analyzer.
typedef enum {
  DEBUG_PIN_PB11 = GPIO_PIN_11,
  DEBUG_PIN_PA12 = GPIO_PIN_12,
} DEBUG_PIN;

/// type definition of signal handler callback
typedef void (*Gpio_HandleGpioExtiSignalCb_t)();

/// Initialize the gpio clocks
void Gpio_InitClocks();

/// Configure Gpio PC10 to receive falling edge interrupts and
/// to notify this signal to the registered listener.
///
/// @param handler A listener that handles the occurring interrupt signal.
void Gpio_RegisterOnExtiSignalPc10(Gpio_HandleGpioExtiSignalCb_t handler);

/// Discards the registered listener and disables the interrupts.
void Gpio_UnregisterOnExtiSignalPc10();

/// Query the status of Gpio pin 10 of port c
/// @return true if the pin is high; false otherwise.
bool Gpio_IsPc10Set();

/// Enable debug pins
///
/// The pins are enabled and configured as output.
void Gpio_EnableDebugPins();

/// Set the value of the selected debug pin port B-11.
/// @param value The value to be set
void Gpio_SetDebugPinB11(bool value);

/// Set the value of the selected debug pin port A-12.
/// @param value The value to be set
void Gpio_SetDebugPinA12(bool value);

#endif  // GPIO_H
