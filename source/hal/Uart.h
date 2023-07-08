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

/// @file Uart.h
///
/// Functions to configure the Uart according to the needs of
/// the application.

#ifndef UART_H
#define UART_H

#include "stm32wbxx_hal.h"  // including uart only is not enough

#include <stdbool.h>

/// Allow to receive data from the uart
typedef struct _tUart_Receiver {
  void (*ReceiveDataCb)(uint16_t nrOfBytes);  ///< Callback to signal the
                                              ///< availability of data.
                                              ///< After calling this function,
                                              ///< more data can be received
  uint8_t* receiveBuffer;                     ///< Buffer to get the data.
  uint16_t rxLength;                          ///< Expected receive length. An
                                              ///< interrupt will be signalled
                                              ///< after receiving this amount
                                              ///< of data.
} Uart_Receiver_t;

/// Get the initialized pointer of the Uart driver instance
///
/// The driver is initialized upon the first call to this function.
/// Subsequent calls return the initialized instance.
/// The driver will be reinitialized after release as well.
///
/// @return pointer to the initialized driver pointer
UART_HandleTypeDef* Uart_Instance();

/// Release the Uart to save power
///
/// The uart clock is halted and the ios are configured as analog input
/// to optimize current consumption in power down mode.
void Uart_Release();

/// Write data to the Uart
/// @param data data to be written
/// @param length length of data in number of bytes.
void Uart_WriteBlocking(const uint8_t* data, uint16_t length);

/// Write data to the Uart
/// @param data data to be written
/// @param length length of data in number of bytes.
/// @note This function must not be used in interrupt context!
/// A higher prioritized interrupt may prevent the operation from completion
/// and the system may get stuck.
/// The function is meant to be called by the tracer in
/// user mode context!
void Uart_Write(const uint8_t* data, uint16_t length);

/// Register a receive handler
///
/// The receive handler is meant to receive and interpret data from the PC.
/// Only one receive handler can be registered. To unregister, the function is
/// called with a null pointer as receive handler.
/// @param rxHandler receive handler that is registered
void Uart_RegisterRxHandler(Uart_Receiver_t* rxHandler);

#endif  //UART_H
