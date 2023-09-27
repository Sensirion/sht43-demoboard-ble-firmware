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

/// @file DataLoggerService.c
#include "DataLoggerService.h"

#include "app_service/networking/ble/BleGatt.h"
#include "app_service/networking/ble/BleTypes.h"
#include "app_service/nvm/ProductionParameters.h"
#include "app_service/power_manager/BatteryMonitor.h"
#include "ble.h"
#include "string.h"
#include "svc/Inc/dis.h"
#include "tl.h"
#include "utility/ErrorHandler.h"
#include "utility/log/Trace.h"

#include <math.h>

/// Sample type of SHT4x data logger frames
#define SHT4x_SAMPLE_TYPE 0x5
/// Offset of the sample type in data logger frame[0]
#define SAMPLE_TYPE_OFFSET 0x4
/// Offset of metadata in data logger frame[0]
#define METADATA_OFFSET 0x6

/// Copy a uint16_t value into a buffer
#define SET_UINT16(buffer, value, offset) *((uint16_t*)&buffer[offset]) = value

/// Copy the value into buffer at the specified offset
#define SET_MEM(buffer, value, offset, length) \
  memcpy(&buffer[offset], value, length)

/// Handle service notification events
/// @param event received event
/// @return event handler status
static SVCCTL_EvtAckStatus_t EventHandler(void* event);

/// Define an ID for each characteristic of the service
/// The ID is used as index into a function table
typedef enum {
  CHARACTERISTIC_ID_LOGGING_INTERVAL = 0,
  CHARACTERISTIC_ID_AVAILABLE_SAMPLES,
  CHARACTERISTIC_ID_REQUEST_SAMPLES,
  CHARACTERISTIC_ID_SAMPLE_DATA,
  CHARACTERISTIC_ID_NR_OF_CHARS,
} CharacteristicIds_t;

/// structure to hold the handles for gatt service and its characteristics
PLACE_IN_SECTION("BLE_DRIVER_CONTEXT") static struct _tService {
  uint16_t serviceHandle;  ///< Service handle
  /// table with characteristics
  BleGatt_ServiceCharacteristic_t characteristic[CHARACTERISTIC_ID_NR_OF_CHARS];
  uint16_t currentConnection;  ///< Handle to the current connection
} _service;                    ///< service instance

/// Uuid of device data logger service
/// 00008000-B38D-4985-720E-0F993A68EE41
static BleTypes_Uuid_t _serviceId = {
    .uuidType = UUID_TYPE_128,
    .uuid.Char_UUID_128 = {0x41, 0xEE, 0x68, 0x3A, 0x99, 0x0F, 0x0E, 0x72, 0x85,
                           0x49, 0x8D, 0xB3, 0x00, 0x80, 0x00, 0x00}};

/// Add the logging interval characteristic
/// @param service Pointer to the service structure
static void AddLoggingIntervalCharacteristic(struct _tService* service);

/// Add the available samples characteristic
/// @param service Pointer to the service structure
static void AddAvailableSamplesCharacteristic(struct _tService* service);

/// Add the request samples characteristic
/// @param service Pointer to the service structure
static void AddRequestSamplesCharacteristic(struct _tService* service);

/// Add the sample data characteristic
/// @param service Pointer to the service structure
static void AddSampleDataCharacteristic(struct _tService* service);

/// Handle the client request of reading the logging interval
/// @param currentConnection Client connection handle
/// @param data The data of the request
/// @param dataLength The number of bytes in data
/// @return the status of the event handler
SVCCTL_EvtAckStatus_t ReadLoggingInterval(uint16_t currentConnection,
                                          uint8_t* data,
                                          uint8_t dataLength);

/// Handle the client request of setting the logging interval.
/// @param currentConnection Client connection handle
/// @param data The data of the request
/// @param dataLength The number of bytes in data
/// @return the status of the event handler
SVCCTL_EvtAckStatus_t WriteLoggingInterval(uint16_t currentConnection,
                                           uint8_t* data,
                                           uint8_t dataLength);

/// Handle the client request reading the number of available samples.
/// @param currentConnection Client connection handle
/// @param data The data of the request
/// @param dataLength The number of bytes in data
/// @return the status of the event handler
SVCCTL_EvtAckStatus_t ReadAvailableSamples(uint16_t currentConnection,
                                           uint8_t* data,
                                           uint8_t dataLength);

/// Handle the client request reading the number of requested samples that
/// where not yet fetched.
/// @param currentConnection Client connection handle
/// @param data The data of the request
/// @param dataLength The number of bytes in data
/// @return the status of the event handler
SVCCTL_EvtAckStatus_t ReadRequestedSamples(uint16_t currentConnection,
                                           uint8_t* data,
                                           uint8_t dataLength);

/// Handle the client request to write the number of requested samples.
///
/// @param currentConnection Client connection handle
/// @param data The data of the request
/// @param dataLength The number of bytes in data
/// @return the status of the event handler
SVCCTL_EvtAckStatus_t WriteRequestedSamples(uint16_t currentConnection,
                                            uint8_t* data,
                                            uint8_t dataLength);

/// Default handler to be used for read only characteristic
/// @param currentConnection Client connection handle
/// @param data The data of the request
/// @param dataLength The number of bytes in data
/// @return the status of the event handler
SVCCTL_EvtAckStatus_t NopWriteHandler(uint16_t currentConnection,
                                      uint8_t* data,
                                      uint8_t dataLength);

/// Setup the data logger service
void DataLoggerService_Create() {
  // create service
  _service.serviceHandle = BleGatt_AddPrimaryService(_serviceId, 5);
  ASSERT(_service.serviceHandle != 0);

  // register service handle; needed for data logger service
  SVCCTL_RegisterSvcHandler(EventHandler);

  // add characteristics
  AddLoggingIntervalCharacteristic(&_service);
  AddAvailableSamplesCharacteristic(&_service);
  AddRequestSamplesCharacteristic(&_service);
  AddSampleDataCharacteristic(&_service);
}

void DataLoggerService_UpdateDataLoggingIntervalCharacteristic(
    uint32_t dataLoggingInterval) {
  tBleStatus status = BleGatt_UpdateCharacteristic(
      _service.serviceHandle,
      _service.characteristic[CHARACTERISTIC_ID_LOGGING_INTERVAL].handle,
      (uint8_t*)&dataLoggingInterval, sizeof dataLoggingInterval);
  ASSERT(status == BLE_STATUS_SUCCESS);
  aci_gatt_allow_read(_service.currentConnection);
}

void DataLoggerService_UpdateAvailableSamplesCharacteristic(uint32_t samples) {
  tBleStatus status = BleGatt_UpdateCharacteristic(
      _service.serviceHandle,
      _service.characteristic[CHARACTERISTIC_ID_AVAILABLE_SAMPLES].handle,
      (uint8_t*)&samples, sizeof samples);
  ASSERT(status == BLE_STATUS_SUCCESS);
  aci_gatt_allow_read(_service.currentConnection);
}

bool DataLoggerService_UpdateSampleDataCharacteristic(
    uint8_t frame[TX_FRAME_SIZE]) {
  tBleStatus status = BleGatt_UpdateCharacteristic(
      _service.serviceHandle,
      _service.characteristic[CHARACTERISTIC_ID_SAMPLE_DATA].handle, frame,
      TX_FRAME_SIZE);
  return (status == BLE_STATUS_SUCCESS);
}

static void AddLoggingIntervalCharacteristic(struct _tService* service) {
  BleTypes_Characteristic_t loggingIntervalCharacteristic = {
      .uuid.uuid.Char_UUID_16 = 0x8001,
      .maxValueLength = 4,
      .characteristicPropertyFlags = CHAR_PROP_READ | CHAR_PROP_WRITE,
      .securityFlags = ATTR_PERMISSION_NONE,
      .eventFlags = GATT_NOTIFY_ATTRIBUTE_WRITE |
                    GATT_NOTIFY_READ_REQ_AND_WAIT_FOR_APPL_RESP,
      .encryptionKeySize = 10,
      .isVariableLengthValue = false};
  BleGatt_ExtendCharacteristicUuid(&loggingIntervalCharacteristic.uuid,
                                   &_serviceId);

  uint32_t value = 60000;

  uint16_t handle = BleGatt_AddCharacteristic(service->serviceHandle,
                                              &loggingIntervalCharacteristic,
                                              (uint8_t*)&value, sizeof(value));
  ASSERT(handle != 0);
  _service.characteristic[CHARACTERISTIC_ID_LOGGING_INTERVAL].handle = handle;
  _service.characteristic[CHARACTERISTIC_ID_LOGGING_INTERVAL].onRead =
      ReadLoggingInterval;
  _service.characteristic[CHARACTERISTIC_ID_LOGGING_INTERVAL].onWrite =
      WriteLoggingInterval;
}

static void AddAvailableSamplesCharacteristic(struct _tService* service) {
  BleTypes_Characteristic_t availableSamplesCharacteristic = {
      .uuid.uuid.Char_UUID_16 = 0x8002,
      .maxValueLength = 4,
      .characteristicPropertyFlags = CHAR_PROP_READ,
      .securityFlags = ATTR_PERMISSION_NONE,
      .eventFlags = GATT_NOTIFY_READ_REQ_AND_WAIT_FOR_APPL_RESP,
      .encryptionKeySize = 10,
      .isVariableLengthValue = false};
  BleGatt_ExtendCharacteristicUuid(&availableSamplesCharacteristic.uuid,
                                   &_serviceId);

  uint32_t value = 0;

  uint16_t handle = BleGatt_AddCharacteristic(service->serviceHandle,
                                              &availableSamplesCharacteristic,
                                              (uint8_t*)&value, sizeof(value));
  ASSERT(handle != 0);
  _service.characteristic[CHARACTERISTIC_ID_AVAILABLE_SAMPLES].handle = handle;
  _service.characteristic[CHARACTERISTIC_ID_AVAILABLE_SAMPLES].onRead =
      ReadAvailableSamples;
  _service.characteristic[CHARACTERISTIC_ID_AVAILABLE_SAMPLES].onWrite =
      NopWriteHandler;
}

static void AddRequestSamplesCharacteristic(struct _tService* service) {
  BleTypes_Characteristic_t requestedSamplesCharacteristic = {
      .uuid.uuid.Char_UUID_16 = 0x8003,
      .maxValueLength = 4,
      .characteristicPropertyFlags = CHAR_PROP_READ | CHAR_PROP_WRITE,
      .securityFlags = ATTR_PERMISSION_NONE,
      .eventFlags = GATT_NOTIFY_ATTRIBUTE_WRITE,
      .encryptionKeySize = 10,
      .isVariableLengthValue = false};
  BleGatt_ExtendCharacteristicUuid(&requestedSamplesCharacteristic.uuid,
                                   &_serviceId);

  uint32_t value = 0;

  uint16_t handle = BleGatt_AddCharacteristic(service->serviceHandle,
                                              &requestedSamplesCharacteristic,
                                              (uint8_t*)&value, sizeof(value));
  ASSERT(handle != 0);
  _service.characteristic[CHARACTERISTIC_ID_REQUEST_SAMPLES].handle = handle;
  _service.characteristic[CHARACTERISTIC_ID_REQUEST_SAMPLES].onRead =
      ReadRequestedSamples;
  _service.characteristic[CHARACTERISTIC_ID_REQUEST_SAMPLES].onWrite =
      WriteRequestedSamples;
}

static void AddSampleDataCharacteristic(struct _tService* service) {
  BleTypes_Characteristic_t sampleDataCharacteristic = {
      .uuid.uuid.Char_UUID_16 = 0x8004,
      .maxValueLength = TX_FRAME_SIZE,
      .characteristicPropertyFlags = CHAR_PROP_NOTIFY,
      .securityFlags = ATTR_PERMISSION_NONE,
      .eventFlags = GATT_DONT_NOTIFY_EVENTS,
      .encryptionKeySize = 10,
      .isVariableLengthValue = false};
  BleGatt_ExtendCharacteristicUuid(&sampleDataCharacteristic.uuid, &_serviceId);
  uint8_t value[TX_FRAME_SIZE] = {0};

  uint16_t handle = BleGatt_AddCharacteristic(
      service->serviceHandle, &sampleDataCharacteristic, value, TX_FRAME_SIZE);
  ASSERT(handle != 0);
  _service.characteristic[CHARACTERISTIC_ID_SAMPLE_DATA].handle = handle;
}

static SVCCTL_EvtAckStatus_t EventHandler(void* void_event) {
  hci_event_pckt* event_pckt =
      (hci_event_pckt*)(((hci_uart_pckt*)void_event)->data);
  switch (event_pckt->evt) {
    case HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE: {
      evt_blecore_aci* event = (evt_blecore_aci*)event_pckt->data;
      return BleGatt_HandleBleCoreEvent(event, _service.characteristic,
                                        CHARACTERISTIC_ID_NR_OF_CHARS);
    }
    default:
      break;
  }
  return SVCCTL_EvtNotAck;
}

SVCCTL_EvtAckStatus_t ReadLoggingInterval(uint16_t connectionHandle,
                                          uint8_t* data,
                                          uint8_t dataLength) {
  _service.currentConnection = connectionHandle;
  Message_Message_t msg = {
      .header.category = MESSAGE_BROKER_CATEGORY_BLE_SERVICE_REQUEST,
      .header.id = SERVICE_REQUEST_MESSAGE_ID_GET_LOGGING_INTERVAL,
      .parameter2 = connectionHandle};

  Message_PublishAppMessage(&msg);
  return SVCCTL_EvtAckFlowEnable;
}

SVCCTL_EvtAckStatus_t WriteLoggingInterval(uint16_t connectionHandle,
                                           uint8_t* data,
                                           uint8_t dataLength) {
  Message_Message_t msg = {
      .header.category = MESSAGE_BROKER_CATEGORY_BLE_SERVICE_REQUEST,
      .header.id = SERVICE_REQUEST_MESSAGE_ID_SET_LOGGING_INTERVAL,
      .parameter2 = *((uint32_t*)data)};
  Message_PublishAppMessage(&msg);
  return SVCCTL_EvtAckFlowEnable;
}

SVCCTL_EvtAckStatus_t ReadAvailableSamples(uint16_t connectionHandle,
                                           uint8_t* data,
                                           uint8_t dataLength) {
  _service.currentConnection = connectionHandle;
  Message_Message_t msg = {
      .header.category = MESSAGE_BROKER_CATEGORY_BLE_SERVICE_REQUEST,
      .header.id = SERVICE_REQUEST_MESSAGE_ID_GET_AVAILABLE_SAMPLES,
      .parameter2 = connectionHandle};

  Message_PublishAppMessage(&msg);
  return SVCCTL_EvtAckFlowEnable;
}

SVCCTL_EvtAckStatus_t NopWriteHandler(uint16_t connectionHandle,
                                      uint8_t* data,
                                      uint8_t dataLength) {
  return SVCCTL_EvtAckFlowEnable;
}

SVCCTL_EvtAckStatus_t ReadRequestedSamples(uint16_t currentConnection,
                                           uint8_t* data,
                                           uint8_t dataLength) {
  // should not get here as the event flag is not set.
  return SVCCTL_EvtAckFlowEnable;
}

SVCCTL_EvtAckStatus_t WriteRequestedSamples(uint16_t currentConnection,
                                            uint8_t* data,
                                            uint8_t dataLength) {
  Message_Message_t msg = {
      .header.category = MESSAGE_BROKER_CATEGORY_BLE_SERVICE_REQUEST,
      .header.id = SERVICE_REQUEST_MESSAGE_ID_SET_REQUESTED_SAMPLES,
      .parameter2 = *((uint16_t*)data)};
  Message_PublishAppMessage(&msg);
  // just update the characteristic with this value; isn't very meaningful
  // though
  tBleStatus status = BleGatt_UpdateCharacteristic(
      _service.serviceHandle,
      _service.characteristic[CHARACTERISTIC_ID_REQUEST_SAMPLES].handle, data,
      dataLength);
  ASSERT(status == BLE_STATUS_SUCCESS);
  return SVCCTL_EvtAckFlowEnable;
}

void DataLoggerService_BuildHeaderFrame(uint8_t txFrameBuffer[TX_FRAME_SIZE],
                                        BleTypes_SamplesMetaData_t* metadata) {
  memset(txFrameBuffer, 0, TX_FRAME_SIZE);
  SET_UINT16(txFrameBuffer, SHT4x_SAMPLE_TYPE, SAMPLE_TYPE_OFFSET);
  SET_MEM(txFrameBuffer, metadata, METADATA_OFFSET,
          sizeof(BleTypes_SamplesMetaData_t));
}

void DataLoggerService_BuildDataFrame(uint8_t txFrameBuffer[TX_FRAME_SIZE],
                                      uint16_t frameIndex,
                                      uint8_t* data,
                                      uint8_t dataLength) {
  ASSERT(dataLength <= 16);
  memset(txFrameBuffer, 0, TX_FRAME_SIZE);
  SET_UINT16(txFrameBuffer, frameIndex, 0);
  SET_MEM(txFrameBuffer, data, sizeof(frameIndex), dataLength);
}
