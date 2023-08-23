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

/// @file BleContext.c
///
/// Source file for all advertising (GAP) services

#include "BleContext.h"

#include "app_service/networking/HciTransport.h"
#include "app_service/networking/ble/BleGap.h"
#include "app_service/networking/ble/BleHelper.h"
#include "app_service/networking/ble/BleInterface.h"
#include "app_service/networking/ble/gatt_service/HumidityService.h"
#include "app_service/networking/ble/gatt_service/ShtService.h"
#include "app_service/networking/ble/gatt_service/TemperatureService.h"
#include "app_service/nvm/ProductionParameters.h"
#include "app_service/power_manager/BatteryMonitor.h"
#include "app_service/sensor/Sht4x.h"
#include "app_service/user_button/Button.h"
#include "shci.h"
#include "stm32_lpm.h"
#include "stm32_seq.h"
#include "svc/Inc/svc_ctl.h"
#include "utility/AppDefines.h"
#include "utility/ErrorHandler.h"
#include "utility/log/Trace.h"
#include "utility/scheduler/MessageId.h"
#include "utility/scheduler/Scheduler.h"

/// These are the two tags used to manage a power failure during OTA
/// The MagicKeywordAdress shall be mapped @0x140 from start of the binary image
/// The MagicKeywordvalue is checked in the sht43_ota application

/// value of the magic keyword that is looked up by the OTA loader
#define MAGIC_OTA_KEYWORD 0x94448A29

/// Variable holding the MAGIC_OTA_KEYWORD
PLACE_IN_SECTION("TAG_OTA_END")
const uint32_t MagicKeywordValue = MAGIC_OTA_KEYWORD;

/// Address of variable holding the MAGIC_OTA_KEYWORD
PLACE_IN_SECTION("TAG_OTA_START")
const uint32_t MagicKeywordAddress = (uint32_t)&MagicKeywordValue;

/// Placeholder to advertise a dummy SHT value.
static BleTypes_AdvertisementData_t gAdvData = {
    .adTypeSize = 2,
    .adTypeFlag = AD_TYPE_FLAGS,
    .adTypeValue = 0x06,
    .adTypeManufacturerSize = 11,
    .adTypeManufacturerFlag = AD_TYPE_MANUFACTURER_SPECIFIC_DATA,
    .companyIdentifier = BLE_TYPES_SENSIRION_VENDOR_ID,
    .sAdvT = 0x00,
    .sampleType = 0x06,
    .deviceId = 0xFFFF,
    .temperatureTicks = 0xFFFF,
    .humidityTicks = 0xFFFF,
    .adTypeNameSize = 9,
    .adTypeNameFlag = AD_TYPE_COMPLETE_LOCAL_NAME,
    .name = "",  // will be initialized later on
};

/// Ble subsystem state handler
///
/// @param message
/// @return true if the message was handled, false otherwise
static bool BleDefaultStateCb(Message_Message_t* message);

/// State handler of the ble application when radio is disabled
///
/// @param message
/// @return true if the message was handled, false otherwise
static bool BleDisabledStateCb(Message_Message_t* message);

/// Message handler of the bridge
///
/// just forward the messages of interest to the ble task
/// @param message received message
/// @return true if the message was handled, false otherwise
static bool ForwardToBleAppCb(Message_Message_t* message);

/// Handle change of readout interval
///
/// @param readoutIntervalS new readout interval
static void HandleReadoutIntervalChange(uint8_t readoutIntervalS);

/// Struct to store all user defined information for BLE operations
static BleTypes_ApplicationContext_t gBleApplicationContext = {
    .advertisementData = &gAdvData,
    .advertisementDataSize = sizeof(gAdvData),
    .currentAdvertisementMode.advertiseModeSpecification = {
        .connectable = true,
        .interval = ADVERTISEMENT_INTERVAL_SHORT}};

/// Listener that is executed in the ble task
static MessageListener_Listener_t _bleAppListener = {
    .currentMessageHandlerCb = BleDefaultStateCb,
    .receiveMask = MESSAGE_BROKER_CATEGORY_SENSOR_VALUE |
                   MESSAGE_BROKER_CATEGORY_BLE_EVENT |
                   MESSAGE_BROKER_CATEGORY_BATTERY_EVENT |
                   MESSAGE_BROKER_CATEGORY_BUTTON_EVENT |
                   MESSAGE_BROKER_CATEGORY_TIME_INFORMATION};

/// Listener that is executed in the ble task
static MessageListener_Listener_t _bleBridge = {
    .currentMessageHandlerCb = ForwardToBleAppCb,
    .receiveMask = MESSAGE_BROKER_CATEGORY_SENSOR_VALUE |
                   MESSAGE_BROKER_CATEGORY_BATTERY_EVENT |
                   MESSAGE_BROKER_CATEGORY_BUTTON_EVENT |
                   MESSAGE_BROKER_CATEGORY_SYSTEM_STATE_CHANGE |
                   MESSAGE_BROKER_CATEGORY_TIME_INFORMATION};

MessageListener_Listener_t* BleContext_Instance() {
  return &_bleAppListener;
}

MessageListener_Listener_t* BleContext_BridgeInstance() {
  return &_bleBridge;
}

void BleContext_StartBluetoothApp() {
  gAdvData.deviceId = ProductionParameters_GetUniqueDeviceId() & 0xFFFF;
  // initialize device name
  memcpy(gAdvData.name, (uint8_t*)ProductionParameters_GetDeviceName(),
         sizeof(gAdvData.name));
  BleInterface_Start(&gBleApplicationContext);
  gBleApplicationContext.deviceConnectionStatus = BLE_INTERFACE_IDLE;
  gBleApplicationContext.bleApplicationContextLegacy.connectionHandle = 0xFFFF;
  _bleAppListener.currentMessageHandlerCb = BleDefaultStateCb;
  Message_Message_t msg = {
      .header.category = MESSAGE_BROKER_CATEGORY_SYSTEM_STATE_CHANGE,
      .header.id = MESSAGE_ID_BLE_SUBSYSTEM_READY};
  Message_PublishAppMessage(&msg);
}

/// BLE Application notification handler
///
/// see svc_ctl.h
/// @param notificationPacket The notification from the bluetooth core device
/// @return None
SVCCTL_UserEvtFlowStatus_t SVCCTL_App_Notification(
    void* notificationPacket) {  //NOLINT

  evt_le_meta_event* metaEvent;
  evt_blecore_aci* bleCoreEvent;
  tBleStatus ret = BLE_STATUS_INVALID_PARAMS;
  hci_le_connection_complete_event_rp0* connectionCompleteEvent;
  hci_disconnection_complete_event_rp0* disconnectedCompleteEvent;
  hci_le_connection_update_complete_event_rp0* connectionUpdateCompleteEvent;

  // PAIRING
  aci_gap_pairing_complete_event_rp0* pairingComplete;

  hci_event_pckt* eventPckt =
      (hci_event_pckt*)((hci_uart_pckt*)notificationPacket)->data;

  switch (eventPckt->evt) {
    case HCI_DISCONNECTION_COMPLETE_EVT_CODE: {
      disconnectedCompleteEvent =
          (hci_disconnection_complete_event_rp0*)eventPckt->data;

      if (disconnectedCompleteEvent->Connection_Handle ==
          gBleApplicationContext.bleApplicationContextLegacy.connectionHandle) {
        gBleApplicationContext.deviceConnectionStatus = BLE_INTERFACE_IDLE;
        gBleApplicationContext.bleApplicationContextLegacy.connectionHandle = 0;
        LOG_DEBUG_CALLSTATUS("Disconnected complete event",
                             disconnectedCompleteEvent->Reason);
      }
      // restart advertising
      BleGap_AdvertiseRequest(&gBleApplicationContext,
                              gBleApplicationContext.currentAdvertisementMode);

      break;
    }

    case HCI_LE_META_EVT_CODE: {
      metaEvent = (evt_le_meta_event*)eventPckt->data;
      switch (metaEvent->subevent) {
        case HCI_LE_CONNECTION_UPDATE_COMPLETE_SUBEVT_CODE:
          LOG_DEBUG_CASE(HCI_LE_CONNECTION_UPDATE_COMPLETE_SUBEVT_CODE);
          connectionUpdateCompleteEvent =
              (hci_le_connection_update_complete_event_rp0*)metaEvent->data;

          LOG_DEBUG_CONNECTION_PARAMS(
              connectionUpdateCompleteEvent->Conn_Interval,
              connectionUpdateCompleteEvent->Conn_Latency,
              connectionUpdateCompleteEvent->Supervision_Timeout);

          break;

        case HCI_LE_CONNECTION_COMPLETE_SUBEVT_CODE: {
          connectionCompleteEvent =
              (hci_le_connection_complete_event_rp0*)metaEvent->data;
          // The connection is done, there is no need anymore to schedule the LP ADV
          LOG_DEBUG_CALLSTATUS("connection complete handle: ",
                               connectionCompleteEvent->Connection_Handle);
          LOG_DEBUG_BLUETOOTH_ADDR(connectionCompleteEvent->Peer_Address);
          LOG_DEBUG_CONNECTION_PARAMS(
              connectionCompleteEvent->Conn_Interval,
              connectionCompleteEvent->Conn_Latency,
              connectionCompleteEvent->Supervision_Timeout);

          if (gBleApplicationContext.deviceConnectionStatus ==
              BLE_INTERFACE_LP_CONNECTING) {
            // Connection as client
            gBleApplicationContext.deviceConnectionStatus =
                BLE_INTERFACE_CONNECTED_CLIENT;
          } else {
            // Connection as server
            gBleApplicationContext.deviceConnectionStatus =
                BLE_INTERFACE_CONNECTED_SERVER;
          }
          gBleApplicationContext.bleApplicationContextLegacy.connectionHandle =
              connectionCompleteEvent->Connection_Handle;

          break;  // HCI_LE_CONNECTION_COMPLETE_SUBEVT_CODE
        }

        default:
          break;
      }
      break;  // HCI_LE_META_EVT_CODE
    }

    case HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE:
      bleCoreEvent = (evt_blecore_aci*)eventPckt->data;
      switch (bleCoreEvent->ecode) {
        // SPECIFIC to Custom Template APP
        case ACI_L2CAP_CONNECTION_UPDATE_RESP_VSEVT_CODE:
          LOG_DEBUG_CASE(ACI_L2CAP_CONNECTION_UPDATE_RESP_VSEVT_CODE);
          break;

        case ACI_GAP_PROC_COMPLETE_VSEVT_CODE:
          LOG_DEBUG_CASE(ACI_GAP_PROC_COMPLETE_VSEVT_CODE);
          break;  // ACI_GAP_PROC_COMPLETE_VSEVT_CODE

        case ACI_HAL_END_OF_RADIO_ACTIVITY_VSEVT_CODE:
          LOG_DEBUG_CASE(ACI_L2CAP_CONNECTION_UPDATE_RESP_VSEVT_CODE);
          break;  // ACI_HAL_END_OF_RADIO_ACTIVITY_VSEVT_CODE

        // PAIRING
        case (ACI_GAP_KEYPRESS_NOTIFICATION_VSEVT_CODE):
          LOG_DEBUG_CASE(ACI_GAP_KEYPRESS_NOTIFICATION_VSEVT_CODE);
          break;

        case ACI_GAP_PASS_KEY_REQ_VSEVT_CODE:
          LOG_DEBUG_CASE(ACI_GAP_PASS_KEY_REQ_VSEVT_CODE);

          ret = aci_gap_pass_key_resp(
              gBleApplicationContext.bleApplicationContextLegacy
                  .connectionHandle,
              CFG_FIXED_PIN);
          LOG_DEBUG_CALLSTATUS("aci_gap_pass_key_resp()", ret);

          break;

        case ACI_GAP_NUMERIC_COMPARISON_VALUE_VSEVT_CODE:
          LOG_DEBUG_CASE(ACI_GAP_NUMERIC_COMPARISON_VALUE_VSEVT_CODE);

          ret = aci_gap_numeric_comparison_value_confirm_yesno(
              gBleApplicationContext.bleApplicationContextLegacy
                  .connectionHandle,
              YES);
          LOG_DEBUG_CALLSTATUS("aci_gap_numeric_comparison_value_confirm_yesno",
                               ret);
          break;

        case ACI_GAP_PAIRING_COMPLETE_VSEVT_CODE:
          LOG_DEBUG_CASE(ACI_GAP_PAIRING_COMPLETE_VSEVT_CODE);
          pairingComplete =
              (aci_gap_pairing_complete_event_rp0*)bleCoreEvent->data;

          LOG_DEBUG_CALLSTATUS("pairing()", pairingComplete->Status);
          break;
          // PAIRING
      }
      break;  // HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE

    default:
      break;
  }

  return (SVCCTL_UserEvtFlowEnable);
}

static bool BleDefaultStateCb(Message_Message_t* message) {
  // notify new sensor values
  if (message->header.category == MESSAGE_BROKER_CATEGORY_SENSOR_VALUE &&
      message->header.id == SHT4X_MESSAGE_ID_SENSOR_DATA) {
    Sht4x_SensorMessage_t* sensorMsg = (Sht4x_SensorMessage_t*)message;
    if (message->header.parameter1 == SHT4X_COMMAND_READ_SERIAL_NUMBER) {
      ShtService_SetSerialNumber(sensorMsg->data.serialNumer);
    } else {
      gAdvData.temperatureTicks = sensorMsg->data.measurement.temperatureTicks;
      gAdvData.humidityTicks = sensorMsg->data.measurement.humidityTicks;
      /// this does not change the advertisement mode
      BleGap_AdvertiseRequest(&gBleApplicationContext,
                              gBleApplicationContext.currentAdvertisementMode);
      TemperatureService_SetTemperature(Sht4x_TicksToTemperatureCelsius(
          sensorMsg->data.measurement.temperatureTicks));
      HumidityService_SetHumidity(
          Sht4x_TicksToHumidity(sensorMsg->data.measurement.humidityTicks));
    }
    return true;
  }

  // react on ble events (start stop advertizing)
  if (message->header.category == MESSAGE_BROKER_CATEGORY_BLE_EVENT) {
    if (message->header.id == BLE_INTERFACE_MSG_ID_STOP_ADVERTISE) {
      BleGap_AdvertiseCancel(&gBleApplicationContext);
      return true;
    }
    if (message->header.id == BLE_INTERFACE_MSG_ID_START_ADVERTISE) {
      BleInterface_Message_t* bleMsg = (BleInterface_Message_t*)message;
      BleGap_AdvertiseRequest(&gBleApplicationContext,
                              bleMsg->parameter.advertisementMode);
      return true;
    }
  }

  // react on ble events (start stop advertizing)
  if (message->header.category == MESSAGE_BROKER_CATEGORY_BUTTON_EVENT &&
      message->header.id == BUTTON_EVENT_LONG_PRESS) {
    BleGap_AdvertiseCancel(&gBleApplicationContext);
    _bleBridge.receiveMask = MESSAGE_BROKER_CATEGORY_BUTTON_EVENT;
    _bleAppListener.currentMessageHandlerCb = BleDisabledStateCb;
    return true;
  }

  return false;
}

static bool BleDisabledStateCb(Message_Message_t* message) {
  if (message->header.category == MESSAGE_BROKER_CATEGORY_BUTTON_EVENT &&
      message->header.id == BUTTON_EVENT_LONG_PRESS) {
    gBleApplicationContext.timeRunningTick = 0;
    BleTypes_AdvertisementMode_t advSpec = {
        .advertiseModeSpecification.connectable = true,
        .advertiseModeSpecification.interval = ADVERTISEMENT_INTERVAL_SHORT};
    BleGap_AdvertiseRequest(&gBleApplicationContext, advSpec);
    _bleAppListener.currentMessageHandlerCb = BleDefaultStateCb;
    _bleBridge.receiveMask = MESSAGE_BROKER_CATEGORY_SENSOR_VALUE |
                             MESSAGE_BROKER_CATEGORY_BATTERY_EVENT |
                             MESSAGE_BROKER_CATEGORY_BUTTON_EVENT |
                             MESSAGE_BROKER_CATEGORY_SYSTEM_STATE_CHANGE |
                             MESSAGE_BROKER_CATEGORY_TIME_INFORMATION;

    return true;
  }
  return false;
}

static bool ForwardToBleAppCb(Message_Message_t* message) {
  if (message->header.category == MESSAGE_BROKER_CATEGORY_TIME_INFORMATION &&
      message->header.id == MESSAGE_ID_TIME_INFO_TIME_ELAPSED) {
    BleInterface_PublishBleMessage(message);
    return true;
  }

  if (message->header.category == MESSAGE_BROKER_CATEGORY_SENSOR_VALUE &&
      message->header.id == SHT4X_MESSAGE_ID_SENSOR_DATA) {
    BleInterface_PublishBleMessage(message);
    return true;
  }

  if (message->header.category == MESSAGE_BROKER_CATEGORY_BUTTON_EVENT &&
      message->header.id == BUTTON_EVENT_LONG_PRESS) {
    // do not react on long press during bootup
    if (gBleApplicationContext.timeRunningTick < 5) {
      return false;
    }
    BleInterface_PublishBleMessage(message);
    return true;
  }

  if (message->header.category == MESSAGE_BROKER_CATEGORY_SYSTEM_STATE_CHANGE &&
      message->header.id == MESSAGE_ID_READOUT_INTERVAL_CHANGE) {
    HandleReadoutIntervalChange(message->parameter2);
    return true;
  }

  if (message->header.category == MESSAGE_BROKER_CATEGORY_BATTERY_EVENT) {
    if (message->header.id == BATTERY_MONITOR_MESSAGE_ID_STATE_CHANGE) {
      BatteryMonitor_Message_t* batteryMsg = (BatteryMonitor_Message_t*)message;
      BleTypes_AdvertisementMode_t newMode =
          gBleApplicationContext.currentAdvertisementMode;
      newMode.advertiseModeSpecification.connectable =
          (batteryMsg->currentState ==
           BATTERY_MONITOR_APP_STATE_NO_RESTRICTION);
      uint8_t newMessageId =
          batteryMsg->currentState !=
                  BATTERY_MONITOR_APP_STATE_CRITICAL_BATTERY_LEVEL
              ? BLE_INTERFACE_MSG_ID_START_ADVERTISE
              : BLE_INTERFACE_MSG_ID_STOP_ADVERTISE;
      BleInterface_Message_t newMsg = {
          .head = {.id = newMessageId,
                   .category = MESSAGE_BROKER_CATEGORY_BLE_EVENT},
          .parameter.advertisementMode = newMode};
      BleInterface_PublishBleMessage((Message_Message_t*)&newMsg);
      return true;
    }
    if (message->header.id == BATTERY_MONITOR_MESSAGE_ID_CAPACITY_CHANGE) {
      BleInterface_PublishBleMessage(message);
      return true;
    }
  }
  return false;
}

static void HandleReadoutIntervalChange(uint8_t readoutIntervalS) {
  BleTypes_AdvertisementMode_t newMode =
      gBleApplicationContext.currentAdvertisementMode;
  if (readoutIntervalS == SHORT_READOUT_INTERVAL_S) {
    newMode.advertiseModeSpecification.interval = ADVERTISEMENT_INTERVAL_SHORT;
  } else if (readoutIntervalS == MEDIUM_READOUT_INTERVAL_S) {
    newMode.advertiseModeSpecification.interval = ADVERTISEMENT_INTERVAL_MEDIUM;
  } else {
    newMode.advertiseModeSpecification.interval = ADVERTISEMENT_INTERVAL_LONG;
  }

  BleInterface_Message_t newMsg = {
      .head = {.id = BLE_INTERFACE_MSG_ID_START_ADVERTISE,
               .category = MESSAGE_BROKER_CATEGORY_BLE_EVENT},
      .parameter.advertisementMode = newMode};
  BleInterface_PublishBleMessage((Message_Message_t*)&newMsg);
}
