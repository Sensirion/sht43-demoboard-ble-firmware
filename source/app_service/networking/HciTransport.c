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

/// @file HciTransport.c
///
/// Implementation of HciTransport.h

#include "HciTransport.h"

#include "hal/Ipcc.h"
#include "utility/scheduler/Scheduler.h"
// clang-format off
#include "app_conf.h"
#include "shci.h"
#include "shci_tl.h"
#include "stm32_lpm.h"
#include "stm32_seq.h"
#include "tl.h"
// clang-format on
#include "utility/ErrorHandler.h"
#include "utility/log/Log.h"

#include <stdbool.h>

/// Size of the event pool on the Memory Manager
#define POOL_SIZE                    \
  (CFG_TLBLE_EVT_QUEUE_LENGTH * 4U * \
   DIVC((sizeof(TL_PacketHeader_t) + TL_BLE_EVENT_FRAME_SIZE), 4U))

/// Allocation of the callback for the Transport Layer availability notification
///
/// @param status Status of the Transport Layer
static void OnSystemStatusNotificationCb(SHCI_TL_CmdStatus_t status);

/// Callback that will receive a user asynch system event
///
/// @param payload Data received in the user asynch system event
static void OnSystemUserEventReceivedCb(void* payload);

/// Execute on user event if the code from CPU2 is "SHCI_SUB_EVT_ERROR_NOTIF"
///
/// @param sysEvent received system event
static void OnSystemEventErrorCb(TL_AsynchEvt_t* sysEvent);

/// Execute on user event if the code from CPU2 is "SHCI_SUB_EVT_CODE_READY"
///
/// @param userEventParam parameters of the received event
static void OnSystemEventReadyProcessingCb(
    tSHCI_UserEvtRxParam* userEventParam);

/// Check the correctness of the wireless firmware info.
/// @param fwInfo Structure containing the information about wireless firmware
///               running on cortex M0 (C2)
/// @return true if the versions match are within expected range expected;
///         false otherwise.
static bool CheckC2FwVersions(WirelessFwInfo_t* fwInfo);

/// Event Pool for the Memory Manager
///
/// The BLE and System channel asynchronous events are taken from that pool and
/// sent to CPU1. When CPU1 pushes back a received asynchronous event to the
/// FreeBufQueue, CPU2 reads them from the FreeBufQueue to push them back to the
/// EvtPool.
PLACE_IN_SECTION("MB_MEM2")
ALIGN(4) static uint8_t gEvtPool[POOL_SIZE];

/// Allocate the command buffer to be used by CPU1 and CPU2
PLACE_IN_SECTION("MB_MEM2")
ALIGN(4) static TL_CmdPacket_t gSystemCmdBuffer;

/// This buffers is currently not used.
///
/// They should be kept available as some next CM0+ release may use them. When
/// this happens, it will be transparent to the user and backward compatible.
/// Link to Forum with this info :
/// https://community.st.com/s/question/0D53W00000uY4ZaSAK/ipcc-memory-manager-channel-tlmm
PLACE_IN_SECTION("MB_MEM2")
ALIGN(4)
static uint8_t
    gSystemSpareEvtBuffer[sizeof(TL_PacketHeader_t) + TL_EVT_HDR_SIZE + 255U];

/// This buffers is currently not used.
///
/// They should be kept available as some next CM0+ release may use them. When
/// this happens, it will be transparent to the user and backward compatible.
/// Link to Forum with this info :
/// https://community.st.com/s/question/0D53W00000uY4ZaSAK/ipcc-memory-manager-channel-tlmm
PLACE_IN_SECTION("MB_MEM2")
ALIGN(4)
static uint8_t
    gBleSpareEvtBuffer[sizeof(TL_PacketHeader_t) + TL_EVT_HDR_SIZE + 255];

/// pointer to peripheral
static IPCC_HandleTypeDef* gIpcc;

/// Callback that is used to start the wireless subsystem
static WirelessAppStarterCb_t gWirelessAppStarterCb;

void HciTransport_Init(WirelessAppStarterCb_t startedCb) {
  gIpcc = Ipcc_Instance();
  gWirelessAppStarterCb = startedCb;
  LOG_DEBUG("Initialize TransportLayer ");

  TL_Init();

  // Register the processing of the received asynch user event as well as the
  // calling of UserEvtRx() to the sequencer.
  UTIL_SEQ_RegTask(1 << SCHEDULER_TASK_HANDLE_SYSTEM_HCI_EVENT, UTIL_SEQ_RFU,
                   shci_user_evt_proc);

  // initialize system host control interface
  SHCI_TL_HciInitConf_t hciTransportInitConf = {
      .p_cmdbuffer = (uint8_t*)&gSystemCmdBuffer,
      .StatusNotCallBack = OnSystemStatusNotificationCb,
  };
  shci_init(OnSystemUserEventReceivedCb, (void*)&hciTransportInitConf);

  // Memory Manager channel initialization
  TL_MM_Config_t memManagerConfig = {
      .p_BleSpareEvtBuffer = gBleSpareEvtBuffer,
      .p_SystemSpareEvtBuffer = gSystemSpareEvtBuffer,
      .p_AsynchEvtPool = gEvtPool,
      .AsynchEvtPoolSize = POOL_SIZE};
  TL_MM_Init(&memManagerConfig);

  TL_Enable();

  LOG_DEBUG("...SUCCESS!\n");
}

static void OnSystemStatusNotificationCb(SHCI_TL_CmdStatus_t status) {
  LOG_DEBUG("Status received %i\n", status);
}

static void OnSystemUserEventReceivedCb(void* payload) {
  TL_AsynchEvt_t* sysEvent;

  sysEvent = (TL_AsynchEvt_t*)(((tSHCI_UserEvtRxParam*)payload)
                                   ->pckt->evtserial.evt.payload);

  // Derive the kind of the received user event
  switch (sysEvent->subevtcode) {
    case SHCI_SUB_EVT_CODE_READY:

      OnSystemEventReadyProcessingCb((tSHCI_UserEvtRxParam*)payload);
      break;

    case SHCI_SUB_EVT_ERROR_NOTIF:
      OnSystemEventErrorCb(sysEvent);
      break;

    case SHCI_SUB_EVT_BLE_NVM_RAM_UPDATE:
      LOG_DEBUG("BLE NVM RAM has been updated by CPU2:\n");
      LOG_DEBUG(
          "     - StartAddress = %lx , Size = %ld\n",
          ((SHCI_C2_BleNvmRamUpdate_Evt_t*)sysEvent->payload)->StartAddress,
          ((SHCI_C2_BleNvmRamUpdate_Evt_t*)sysEvent->payload)->Size);
      break;

    case SHCI_SUB_EVT_NVM_START_WRITE:
      LOG_DEBUG(
          "Start NVM write : NumberOfWords = %ld\n",
          ((SHCI_C2_NvmStartWrite_Evt_t*)sysEvent->payload)->NumberOfWords);
      break;

    case SHCI_SUB_EVT_NVM_END_WRITE:
      LOG_DEBUG("End NVM write\n");
      break;

    case SHCI_SUB_EVT_NVM_START_ERASE:
      LOG_DEBUG(
          "Start NVM erase : NumberOfSectors = %ld\n",
          ((SHCI_C2_NvmStartErase_Evt_t*)sysEvent->payload)->NumberOfSectors);
      break;

    case SHCI_SUB_EVT_NVM_END_ERASE:
      LOG_DEBUG("End NVM erase\n");
      break;

    default:
      break;
  }
}

static void OnSystemEventErrorCb(TL_AsynchEvt_t* sysEvent) {
  SCHI_SystemErrCode_t sysErrorCode =
      *((SCHI_SystemErrCode_t*)sysEvent->payload);

  LOG_ERROR("System error %x received\n", sysErrorCode);
}

static void OnSystemEventReadyProcessingCb(tSHCI_UserEvtRxParam* userEvent) {
  TL_AsynchEvt_t* sysEvent =
      (TL_AsynchEvt_t*)(userEvent->pckt->evtserial.evt.payload);
  SHCI_C2_Ready_Evt_t* sysReadyEvent;
  WirelessFwInfo_t wirelessInfo = {0};
  SHCI_C2_CONFIG_Cmd_Param_t configParam = {0};
  uint32_t revisionId = 0;
  uint32_t deviceId = 0;

  // Read the firmware version of both the wireless firmware and the FUS
  SHCI_GetWirelessFwInfo(&wirelessInfo);
  if (!CheckC2FwVersions(&wirelessInfo)) {
    LOG_INFO("Unsupported Firmware version");
  }

  sysReadyEvent = (SHCI_C2_Ready_Evt_t*)sysEvent->payload;

  if (sysReadyEvent->sysevt_ready_rsp == WIRELESS_FW_RUNNING) {
    // The wireless firmware is running on the CPU2
    LOG_DEBUG("Wireless firmware running\n");

    // Enable all events Notification
    configParam.PayloadCmdSize = SHCI_C2_CONFIG_PAYLOAD_CMD_SIZE;
    configParam.EvtMask1 =
        SHCI_C2_CONFIG_EVTMASK1_BIT0_ERROR_NOTIF_ENABLE +
        SHCI_C2_CONFIG_EVTMASK1_BIT1_BLE_NVM_RAM_UPDATE_ENABLE +
        SHCI_C2_CONFIG_EVTMASK1_BIT2_THREAD_NVM_RAM_UPDATE_ENABLE +
        SHCI_C2_CONFIG_EVTMASK1_BIT3_NVM_START_WRITE_ENABLE +
        SHCI_C2_CONFIG_EVTMASK1_BIT4_NVM_END_WRITE_ENABLE +
        SHCI_C2_CONFIG_EVTMASK1_BIT5_NVM_START_ERASE_ENABLE +
        SHCI_C2_CONFIG_EVTMASK1_BIT6_NVM_END_ERASE_ENABLE;

    // Read revision identifier
    revisionId = LL_DBGMCU_GetRevisionID();

    LOG_DEBUG("Revision ID= %lx \n", revisionId);

    configParam.RevisionID = (uint16_t)revisionId;

    deviceId = LL_DBGMCU_GetDeviceID();
    LOG_DEBUG("Device ID= %lx \n", deviceId);
    configParam.DeviceID = (uint16_t)deviceId;
    (void)SHCI_C2_Config(&configParam);

    if (gWirelessAppStarterCb != 0) {
      gWirelessAppStarterCb();
    }
    UTIL_LPM_SetOffMode(1U << CFG_LPM_APP, UTIL_LPM_ENABLE);
  } else if (sysReadyEvent->sysevt_ready_rsp == FUS_FW_RUNNING) {
    // The FUS firmware is running on the CPU2
    // In the scope of this application, there should be no case when we get here
    LOG_DEBUG("FUS Firmware running\n");

    // The packet shall not be released as this is not supported by the FUS
    userEvent->status = SHCI_TL_UserEventFlow_Disable;
  } else {
    LOG_DEBUG("Unexpected event\n");
    ASSERT(false);
  }
}

static bool CheckC2FwVersions(WirelessFwInfo_t* fwInfo) {
  bool asExpected = true;
  asExpected = asExpected && fwInfo->FusVersionMajor == FUS_VERSION_MAJOR;
  asExpected = asExpected && fwInfo->FusVersionMinor == FUS_VERSION_MINOR;
  asExpected = asExpected && fwInfo->StackType == COPRO_BINARY_TYPE;
  asExpected = asExpected && fwInfo->VersionMajor == COPRO_BINARY_VERSION_MAJOR;
  asExpected = asExpected && fwInfo->VersionMinor == COPRO_BINARY_VERSION_MINOR;

  LOG_INFO("BLE Stack version %d.%d.%d\n", fwInfo->VersionMajor,
           fwInfo->VersionMinor, fwInfo->VersionSub);
  LOG_INFO("BLE Stack build %d\n", fwInfo->VersionReleaseType);
  LOG_INFO("Firmware update service version %d.%d.%d\n",
           fwInfo->FusVersionMajor, fwInfo->FusVersionMinor,
           fwInfo->FusVersionSub);

  return asExpected;
}

/// Triggers process to be called once a shci_notify_asynch_evt notification
/// is received
///
/// @param pdata unused
void shci_notify_asynch_evt(void* pdata) {
  UTIL_SEQ_SetTask(1 << SCHEDULER_TASK_HANDLE_SYSTEM_HCI_EVENT,
                   SCHEDULER_PRIO_0);
  return;
}

/// Signal event to sequencer
///
/// @param flag unused
void shci_cmd_resp_release(uint32_t flag) {
  UTIL_SEQ_SetEvt(1 << SCHEDULER_EVENT_SYSTEM_HCI_CMD_RESPONSE);
  return;
}

/// Force sequencer to wait on event
///
/// @param timeout unused
void shci_cmd_resp_wait(uint32_t timeout) {
  UTIL_SEQ_WaitEvt(1 << SCHEDULER_EVENT_SYSTEM_HCI_CMD_RESPONSE);
  return;
}
