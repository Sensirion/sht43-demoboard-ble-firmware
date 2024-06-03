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

/// @file BleInterface.c
///
/// Implementation for the bluetooth low energy interface
#include "BleInterface.h"

#include "BleGap.h"
#include "BleHelper.h"
#include "BleTypes.h"
#include "app_service/networking/ble/gatt_service/BatteryService.h"
#include "app_service/networking/ble/gatt_service/DataLoggerService.h"
#include "app_service/networking/ble/gatt_service/DeviceInfo.h"
#include "app_service/networking/ble/gatt_service/DeviceSettingsService.h"
#include "app_service/networking/ble/gatt_service/HumidityService.h"
#include "app_service/networking/ble/gatt_service/Reboot.h"
#include "app_service/networking/ble/gatt_service/ShtService.h"
#include "app_service/networking/ble/gatt_service/TemperatureService.h"
#include "app_service/nvm/ProductionParameters.h"
#include "shci.h"
#include "stm32_lpm.h"
#include "stm32_seq.h"
#include "svc/Inc/svc_ctl.h"
#include "tl.h"
#include "utility/AppDefines.h"
#include "utility/ErrorHandler.h"
#include "utility/log/Log.h"
#include "utility/scheduler/Scheduler.h"

#define FAST_ADV_TIMEOUT (30 * 1000 * 1000 / CFG_TS_TICK_VAL)     ///< 30s
#define INITIAL_ADV_TIMEOUT (60 * 1000 * 1000 / CFG_TS_TICK_VAL)  ///< 60s

/// Handler for a data received event from the Ble Interface
///
/// @param payload received data
static void UserEventReceivedCb(void* payload);

///  Notifier of the current HCI status
///
/// @param status current HCI status
static void StatusNotificationReceivedCb(HCI_TL_CmdStatus_t status);

/// Initialize the TransportLayer BLE application
static void HciInit();

/// Initialize the GAP and GATT
///
/// @param appContext application context
static void HciGapGattInit(BleTypes_ApplicationContext_t* appContext);

/// Allocate the command buffer to be used by CPU1 and CPU2
PLACE_IN_SECTION("MB_MEM1") ALIGN(4) static TL_CmdPacket_t gBleCmdBuffer;

/// Identity root key used to derive LTK and CSRK
static const uint8_t BLE_CFG_IR_VALUE[16] = CFG_BLE_IRK;

/// Encryption root key used to derive LTK and CSRK
static const uint8_t BLE_CFG_ER_VALUE[16] = CFG_BLE_ERK;

void BleInterface_Start(BleTypes_ApplicationContext_t* appContext) {
  SHCI_CmdStatus_t status;
  SHCI_C2_Ble_Init_Cmd_Packet_t bleInitCmdPacker = {
      {{0, 0, 0}},  // Header unused
      {0,           // pBleBufferAddress not used
       0,           // BleBufferSize not used
       CFG_BLE_NUM_GATT_ATTRIBUTES,
       CFG_BLE_NUM_GATT_SERVICES,
       CFG_BLE_ATT_VALUE_ARRAY_SIZE,
       CFG_BLE_NUM_LINK,
       CFG_BLE_DATA_LENGTH_EXTENSION,
       CFG_BLE_PREPARE_WRITE_LIST_SIZE,
       CFG_BLE_MBLOCK_COUNT,
       CFG_BLE_MAX_ATT_MTU,
       CFG_BLE_SLAVE_SCA,
       CFG_BLE_MASTER_SCA,
       CFG_BLE_LS_SOURCE,
       CFG_BLE_MAX_CONN_EVENT_LENGTH,
       CFG_BLE_HSE_STARTUP_TIME,
       CFG_BLE_VITERBI_MODE,
       CFG_BLE_OPTIONS,
       0,
       CFG_BLE_MAX_COC_INITIATOR_NBR,
       CFG_BLE_MIN_TX_POWER,
       CFG_BLE_MAX_TX_POWER,
       CFG_BLE_RX_MODEL_CONFIG,
       CFG_BLE_MAX_ADV_SET_NBR,
       CFG_BLE_MAX_ADV_DATA_LEN,
       CFG_BLE_TX_PATH_COMPENS,
       CFG_BLE_RX_PATH_COMPENS,
       CFG_BLE_CORE_VERSION,
       CFG_BLE_OPTIONS_EXT}};

  // Initialize host control interface
  HciInit();

  // Do not allow standby
  UTIL_LPM_SetOffMode(1 << APP_DEFINE_LPM_CLIENT_BLE, UTIL_LPM_DISABLE);
  SHCI_C2_RADIO_AllowLowPower(BLE_IP, TRUE);

  // Register the hci user event handler in the sequencer
  UTIL_SEQ_RegTask(1 << SCHEDULER_TASK_HANDLE_HCI_EVENT, UTIL_SEQ_RFU,
                   hci_user_evt_proc);

  // Starts the BLE Stack on CPU2
  status = SHCI_C2_BLE_Init(&bleInitCmdPacker);
  LOG_DEBUG_CALLSTATUS("BLE Init()", status);
  if (status != SHCI_Success) {
    // if you are here, maybe CPU2 doesn't contain STM32WB_Copro_Wireless FW
    ErrorHandler_UnrecoverableError(ERROR_CODE_BLE);
  }

  // Initialization of HCI & GATT & GAP layer
  HciGapGattInit(appContext);

  // Initialization of the BLE Services
  DeviceInfo_Create();
  ShtService_Create();
  TemperatureService_Create();
  HumidityService_Create();
  BatteryService_Create();
  Reboot_Create();
  DataLoggerService_Create();
  DeviceSettingsService_Create();
}

void __attribute__((weak))
BleInterface_PublishBleMessage(Message_Message_t* msg) {
}

static void UserEventReceivedCb(void* payload) {
  SVCCTL_UserEvtFlowStatus_t svctlReturnStatus;
  tHCI_UserEvtRxParam* param;

  param = (tHCI_UserEvtRxParam*)payload;

  svctlReturnStatus = SVCCTL_UserEvtRx((void*)&(param->pckt->evtserial));
  if (svctlReturnStatus != SVCCTL_UserEvtFlowDisable) {
    param->status = HCI_TL_UserEventFlow_Enable;
  } else {
    param->status = HCI_TL_UserEventFlow_Disable;
  }
}

static void StatusNotificationReceivedCb(HCI_TL_CmdStatus_t status) {
  uint32_t taskIdList = (1 << SCHEDULER_LAST_HCICMD_TASK) - 1;
  switch (status) {
    case HCI_TL_CmdBusy:
      // All tasks that may send an commands shall be paused in order
      // to prevent the sending of new commands
      UTIL_SEQ_PauseTask(taskIdList);
      break;

    case HCI_TL_CmdAvailable:
      // All tasks that may send an aci/hci commands shall be listed here
      // This is to prevent a new command is sent while one is already pending
      UTIL_SEQ_ResumeTask(taskIdList);
      break;

    default:
      break;
  }
}

static void HciInit() {
  HCI_TL_HciInitConf_t hciInitConf = {
      .p_cmdbuffer = (uint8_t*)&gBleCmdBuffer,
      .StatusNotCallBack = StatusNotificationReceivedCb};
  hci_init(UserEventReceivedCb, &hciInitConf);
}

static void HciGapGattInit(BleTypes_ApplicationContext_t* appContext) {
  LOG_DEBUG("HCI GAP init begin");
  BleTypes_BleDeviceAddress_t srdBdAddr = {.addressWords = {0}};
  tBleStatus ret = BLE_STATUS_INVALID_PARAMS;

  // HCI Reset to synchronize BLE stack
  ret = hci_reset();
  LOG_DEBUG_CALLSTATUS("Hci reset", ret);

  // We use currently a static random address 48 bit
  // The two upper bits shall be set to 1
  // The lowest 32bits are read from the UDN and the device id
  // to differentiate between devices.
  // The upper 16 bit are the company id and a mask to set the upper most two
  // bits to one!

  srdBdAddr.addressWords[0] = ProductionParameters_GetUniqueDeviceId();
  srdBdAddr.addressWords[1] =
      BLE_TYPES_SENSIRION_VENDOR_ID | 0xC000;  // set highest bits to 11

#if (CFG_BLE_ADDRESS_TYPE != GAP_PUBLIC_ADDR)
  ret = aci_hal_write_config_data(CONFIG_DATA_RANDOM_ADDRESS_OFFSET,
                                  CONFIG_DATA_RANDOM_ADDRESS_LEN,
                                  srdBdAddr.addressBytes);
  LOG_DEBUG_CALLSTATUS("aci_hal_write_config_data(RA)", ret);
  if (ret == BLE_STATUS_SUCCESS) {
    LOG_DEBUG_BLUETOOTH_ADDR(srdBdAddr.addressBytes);
  }
#endif  // CFG_BLE_ADDRESS_TYPE != PUBLIC_ADDR

  // Write Identity root key used to derive LTK and CSRK
  ret = aci_hal_write_config_data(CONFIG_DATA_IR_OFFSET, CONFIG_DATA_IR_LEN,
                                  (uint8_t*)BLE_CFG_IR_VALUE);
  LOG_DEBUG_CALLSTATUS("aci_hal_write_config_data( IR )", ret);

  // Write Encryption root key used to derive LTK and CSRK
  ret = aci_hal_write_config_data(CONFIG_DATA_ER_OFFSET, CONFIG_DATA_ER_LEN,
                                  (uint8_t*)BLE_CFG_ER_VALUE);
  LOG_DEBUG_CALLSTATUS("aci_hal_write_config_data( ER )", ret);

  // Set TX Power.
  ret = aci_hal_set_tx_power_level(1, BLE_TX_POWER);
  LOG_DEBUG_CALLSTATUS("set_tx_power()", ret);

  // Initialize GATT interface
  ret = aci_gatt_init();
  LOG_DEBUG_CALLSTATUS("gatt_init()", ret);

  // Initialize the GAP interface
  BleGap_Init(appContext);

  // Initialize Default PHY
  ret = hci_le_set_default_phy(ALL_PHYS_PREFERENCE, TX_1M, RX_1M);
  LOG_DEBUG_CALLSTATUS("set_default_pyh()", ret);

  LOG_DEBUG("HCI GAP init end");
}
