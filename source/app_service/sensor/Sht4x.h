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

/// @file Sht4x.h
///
/// Header file to all functions to read the SHT4x Sensor data and process it

#ifndef SHT4X_H
#define SHT4X_H

#include "assert.h"
#include "utility/scheduler/MessageBroker.h"

/// These are the ids of the message category
/// MESSAGE_BROKER_CATEGORY_SENSOR_VALUE
typedef enum {
  SHT4X_MESSAGE_ID_REQUEST_SENT,  /// The request completed successfully
  SHT4X_MESSAGE_ID_SENSOR_READY,  /// The wait time for the request has elapsed
  SHT4X_MESSAGE_ID_SENSOR_DATA,   /// The message contains the read sensor data
  SHT4X_MESSAGE_ID_ERROR          /// Something went wrong
} Sht4x_MessageId_t;

/// Defines the commands that can be handled by the SHT4x sensor
typedef enum {
  SHT4X_COMMAND_READ_SERIAL_NUMBER,
  SHT4X_COMMAND_LOW_REPEATABILITY_MEASUREMENT,
  SHT4X_COMMAND_HIGH_REPEATABILITY_MEASUREMENT
} Sht4x_Commands_t;

/// This is the data that is received in response to a
/// communication with the sensor
typedef struct _tSht4x_SensorMessage {
  MessageBroker_MsgHead_t head;  ///< Message head:
                                 ///< Id will be set to a value
                                 ///< of @ref Sht4x_MessageId_t.
                                 ///< The category will be set to SENSOR_VALUE
                                 ///< The param1 will contain the sent command
  union {
    struct {
      uint16_t temperatureTicks;  ///< measured temperature
      uint16_t humidityTicks;     ///< measured humidity
    } measurement;                ///< Sensor measurement values
    uint32_t serialNumer;         ///< The read serial number
    uint32_t errorCode;           ///< number of the error if error occurred
  } data;  ///< The data needs to be interpreted depending on the message id in
           ///< head.id.
           ///< - id == SHT4X_MESSAGE_ID_REQUEST_SENT: data is invalid
           ///< - id == SHT4X_SENSOR_READY: data is invalid
           ///< - id == SHT4X_MESSAGE_ID_SENSOR_DATA: depending on param1 the
           ///<      message contains the measurement data or the serial number
} Sht4x_SensorMessage_t;

static_assert(sizeof(Sht4x_SensorMessage_t) == 8);

/// Initialize the SHT4x Sensor
///
/// @param broker The message broker that where all messages are published
void Sht4x_Init(MessageBroker_Broker_t* broker);

/// Trigger a sensor request
/// @param command id of the request to be triggered
void Sht4x_StartRequest(Sht4x_Commands_t command);

/// The sensor shall publish a message when the
/// current executing command has finished.
///
void Sht4x_NotifySensorReady();

/// Get the previously requested data
void Sht4x_ReadRequestData();

/// Convert ticks from the SHT to temperature in [°C]
///
/// @param ticks  Temperature value in ticks
/// @return Temperature measured by the SHT in [°C]
float Sht4x_TicksToTemperatureCelsius(uint16_t ticks);

/// Convert ticks from the SHT to temperature in [°F]
///
/// @param ticks  Temperature value in ticks
/// @return Temperature measured by the SHT in [°F]
float Sht4x_TicksToTemperatureFahrenheit(uint16_t ticks);

/// Convert ticks from the SHT to relative humidity in [%rH]
///
/// @param ticks  Relative humidity value in ticks
/// @return Humidity measured by the SHT in [%rH]
float Sht4x_TicksToHumidity(uint16_t ticks);

#endif  // SHT4X_H
