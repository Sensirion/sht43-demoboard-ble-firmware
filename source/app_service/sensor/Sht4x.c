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

/// @file Sht4x.c
///
/// Implementation of the interface to read the data from the SHT4x

#include "Sht4x.h"

#include "app_service/timer_server/TimerServer.h"
#include "hal/I2c3.h"
#include "utility/scheduler/MessageBroker.h"

#include <stdbool.h>

/// Address of the SHT
///
/// Bitshift is necessary due to the 7-bit addressing mode. According to the
/// ref manual, bit 0 and bit 8 are "don't care" in the 7-bit addressing mode
#define SHT4X_DEVICE_ADDRESS (0x44U << 1U)

/// Generator polynomial for CRC
///
/// P(x) = x^8 + x^5 + x^4 + 1 = 100110001
#define POLYNOMIAL 0x131
/// Timeout value to transceive I2C data
#define I2C_TRANSCEIVE_TIMEOUT_MS 1000

/// Command to read the serial number
#define START_READ_SERIAL_NUMBER_CMD 0x89
/// Size of the response of the SHT when reading the serial number
#define READ_SERIAL_NUMBER_SIZE 6

/// Command to start a high repeatability measurement
#define START_HIGH_REPEATABILITY_MEASUREMENT_CMD 0xFD
/// Command to start a low repeatability measurement
#define START_LOW_REPEATABILITY_MEASUREMENT_CMD 0xE0
/// Size of the response of the SHT when reading a measurement
#define READ_MEASUREMENT_SIZE 6
/// maximum size required for communication buffer
#define MAX_BUFFER_SIZE 6

/// Defines a set of metadata that are used to process
/// any request to the sensor in a generic way
typedef struct _tCommandMetaData {
  uint8_t cmdId;       ///< command id to be sent to the sensor
  uint8_t waitTimeMs;  ///< time the sensor requires to process the request
  uint8_t resultSize;  ///< number of bytes returned by the sensor
  void (*evaluateCb)(const uint8_t* data, Sht4x_SensorMessage_t* message);  ///<
      ///< callback function to evaluate the data extract
      ///< the sensor data from the byte stream returned
      ///< be the sensor.
} CommandMetaData_t;

/// Get the serial number from the received byte stream
/// @param data Data bytes received from the sensor
/// @param message Message to be filled with the extracted serial number
static void ExtractSerialNumber(const uint8_t* data,
                                Sht4x_SensorMessage_t* message);

/// Get the measurement values from the received byte stream
/// @param data Data bytes received from the sensor
/// @param message Message to be filled with the extracted measurement values
static void ExtractMeasurementValues(const uint8_t* data,
                                     Sht4x_SensorMessage_t* message);

/// Calculate the Cyclic Redundancy Check (CRC)
///
/// @param data     Data of which the CRC should be calculated
/// @param nBytes   Number of bytes to be checked
/// @return The CRC of the data parsed
static uint8_t CalculateCrc(uint8_t const data[], uint8_t nBytes);

/// Check the received CRCs within the received ata
///
/// @param nBytes   Number of bytes to be checked
///                 This is supposed to be a multiple of 3!
/// @return true, if the calculated crc matches the one from the SHT
static bool CheckCrc(uint8_t nBytes);

/// Publishes the message that the request is sent
static void RequestCompleted();

/// Publishes the message the the response is received
static void ResposeReceived();

/// Callback to indicate that the wait time has elapsed and that the
/// and the data can be read.
static void SensorReadyCb();

/// The metadata for each request
static const CommandMetaData_t _commandMetaData[] = {
    [SHT4X_COMMAND_READ_SERIAL_NUMBER] = {.cmdId = 0x89,
                                          .resultSize = 6,
                                          .waitTimeMs = 1,
                                          .evaluateCb = ExtractSerialNumber},
    [SHT4X_COMMAND_LOW_REPEATABILITY_MEASUREMENT] =
        {.cmdId = 0xe0,
         .resultSize = 6,
         .waitTimeMs = 2,
         .evaluateCb = ExtractMeasurementValues},
    [SHT4X_COMMAND_HIGH_REPEATABILITY_MEASUREMENT] = {
        .cmdId = 0xFD,
        .resultSize = 6,
        .waitTimeMs = 9,
        .evaluateCb = ExtractMeasurementValues}};

/// Pointer to the I2C type handler
static MessageBroker_Broker_t* _appMessageBroker;

/// Sensor message template
Sht4x_SensorMessage_t _sht4xMessage = {
    .head.category = MESSAGE_BROKER_CATEGORY_SENSOR_VALUE,
    .head.id = 0xFF,                // undefined at this point in time
    .head.parameter1 = 0xFF,        // invalid data
    .data.serialNumer = 0xFFFFFFFF  // not defined at this point in time
};

/// Buffer holding the data for the asynchronous transfer with the sensor
static uint8_t _communicationBuffer[8];

/// currently executing command
/// will be set when the request arrives
static uint8_t _command = 0xFF;

/// Sensor busy timer
/// This timer will be started when asked for a notification
uint8_t _timer;

float Sht4x_TicksToTemperatureCelsius(uint16_t ticks) {
  return (float)(ticks * (175.F / 65535.F)) - 45.F;
}

float Sht4x_TicksToTemperatureFahrenheit(uint16_t ticks) {
  // f = c*9/5 + 32
  return (float)(ticks * (315.F / 65535.F)) - 49.F;
}

float Sht4x_TicksToHumidity(uint16_t ticks) {
  return (float)(ticks * (125.F / 65535.F)) - 6.F;
}

void Sht4x_Init(MessageBroker_Broker_t* broker) {
  _appMessageBroker = broker;
  _timer =
      TimerServer_CreateTimer(TIMER_SERVER_MODE_SINGLE_SHOT, SensorReadyCb);
}

void Sht4x_StartRequest(Sht4x_Commands_t command) {
  _command = command;
  _communicationBuffer[0] = _commandMetaData[command].cmdId;
  I2c3_Write(SHT4X_DEVICE_ADDRESS, _communicationBuffer, 1, RequestCompleted);
}

void Sht4x_NotifySensorReady() {
  TimerServer_Start(_timer, _commandMetaData[_command].waitTimeMs);
}

void Sht4x_ReadRequestData() {
  I2c3_Read(SHT4X_DEVICE_ADDRESS, _communicationBuffer,
            _commandMetaData[_command].resultSize, ResposeReceived);
}

static void RequestCompleted() {
  _sht4xMessage.head.id = SHT4X_MESSAGE_ID_REQUEST_SENT;
  _sht4xMessage.head.parameter1 = _command;
  MessageBroker_PublishMessage(_appMessageBroker,
                               (Message_Message_t*)&_sht4xMessage);
}

static void ResposeReceived() {
  _sht4xMessage.head.id = SHT4X_MESSAGE_ID_SENSOR_DATA;
  if (!CheckCrc(_commandMetaData[_command].resultSize)) {
    _sht4xMessage.head.id = SHT4X_MESSAGE_ID_ERROR;
    _sht4xMessage.data.errorCode = 0x100;
  } else {
    _commandMetaData[_command].evaluateCb(_communicationBuffer, &_sht4xMessage);
  }
  // publish either data or error message
  MessageBroker_PublishMessage(_appMessageBroker,
                               (Message_Message_t*)&_sht4xMessage);
}

static void SensorReadyCb() {
  _sht4xMessage.head.id = SHT4X_MESSAGE_ID_SENSOR_READY;
  MessageBroker_PublishMessage(_appMessageBroker,
                               (Message_Message_t*)&_sht4xMessage);
}

static void ExtractSerialNumber(const uint8_t* data,
                                Sht4x_SensorMessage_t* message) {
  // Deserialize the serial number
  message->data.serialNumer =
      (data[0] << 24) | (data[1] << 16) | (data[3] << 8) | data[4];
}

static void ExtractMeasurementValues(const uint8_t* data,
                                     Sht4x_SensorMessage_t* message) {
  // Deserialize the measurement values
  message->data.measurement.temperatureTicks = (data[0] << 8) | data[1];
  message->data.measurement.humidityTicks = (data[3] << 8) | data[4];
}

// this can be replaced by hw!
static uint8_t CalculateCrc(uint8_t const data[], uint8_t nBytes) {
  uint8_t crc = 0xFF;  // calculated checksum

  // Calculate 8-Bit checksum with given polynomial
  for (uint8_t byteCtr = 0; byteCtr < nBytes; ++byteCtr) {
    crc ^= (data[byteCtr]);
    for (uint8_t bit = 8; bit > 0; --bit) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ POLYNOMIAL;
      } else {
        crc = (crc << 1);
      }
    }
  }
  return crc;
}

static bool CheckCrc(uint8_t nBytes) {
  for (int i = 0; i < nBytes; i += 3) {
    uint8_t crc = CalculateCrc(&_communicationBuffer[i], 3);
    if (crc != 0) {
      return false;
    }
  }
  return true;
}
