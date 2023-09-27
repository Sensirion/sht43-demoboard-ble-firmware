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

/// @file DataLoggerService.h
#ifndef DATA_LOGGER_SERVICE_H
#define DATA_LOGGER_SERVICE_H

#include "app_service/networking/ble/BleInterface.h"

#include <stdbool.h>
#include <stdint.h>

/// Size of data logger frame
#define TX_FRAME_SIZE 20

/// Setup the data logger service
/// The required fields are specified in
/// https://github.com/Sensirion/arduino-ble-gadget/blob/master/documents/Sensirion_BLE_communication_protocol.pdf
void DataLoggerService_Create();

/// Write the new value of the dataLogging interval to the characteristic.
///
/// The BLE stack will only reply after getting this confirmation.
/// @param dataLoggingInterval New data logging interval
void DataLoggerService_UpdateDataLoggingIntervalCharacteristic(
    uint32_t dataLoggingInterval);

/// Write the available samples to the characteristic.
///
/// The BLE stack will only reply after getting this confirmation.
/// @param samples Number of available samples
void DataLoggerService_UpdateAvailableSamplesCharacteristic(uint32_t samples);

/// Write the next frame to the sample data characteristic.
///
/// @param frame data frame to be notified to the client
/// @return true if the characteristic was updated successfully;
///         false otherwise.
/// @note:  In case the function returns false, the update shall be retried as
///         soon as a ACI_GATT_TX_POOL_AVAILABLE_EVENT has been received.
bool DataLoggerService_UpdateSampleDataCharacteristic(
    uint8_t frame[TX_FRAME_SIZE]);

/// Build the first notification frame containing the metadata of the
/// MeasurementSampleData.
/// @param txFrameBuffer Storage for the first frame
/// @param metadata Metadata to be embedded in the frame
void DataLoggerService_BuildHeaderFrame(uint8_t txFrameBuffer[TX_FRAME_SIZE],
                                        BleTypes_SamplesMetaData_t* metadata);

/// Build a data frame containing logged data.
/// @param txFrameBuffer Storage for the data frame
/// @param frameIndex Index of the frame
/// @param data Data bytes to be sent
/// @param dataLength Number of valid data bytes
void DataLoggerService_BuildDataFrame(uint8_t txFrameBuffer[TX_FRAME_SIZE],
                                      uint16_t frameIndex,
                                      uint8_t* data,
                                      uint8_t dataLength);

#endif  // DATA_LOGGER_SERVICE_H
