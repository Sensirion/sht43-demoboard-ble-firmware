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

/// @file Reboot.c
/// Implements the Reboot service

#include "Reboot.h"

#include "app_service/networking/ble/BleGatt.h"
#include "app_service/networking/ble/BleTypes.h"
#include "ble.h"
#include "svc/Inc/dis.h"
#include "tl.h"
#include "utility/ErrorHandler.h"

/// structure to hold the handles for gatt service and its characteristics
PLACE_IN_SECTION("BLE_DRIVER_CONTEXT") static struct _tService {
  uint16_t serviceHandle;               ///< service handle
  uint16_t rebootCharacteristicHandle;  ///< reboot request characteristic
} _rebootService;                       ///< reboot service instance

/// Uuid of device information service
BleTypes_Uuid_t rebootServiceServiceId = {
    .uuidType = BLE_TYPES_UUID128,
    .uuid.Char_UUID_128 = {0x25, 0x54, 0xae, 0xa1, 0xd1, 0x89, 0x27, 0xaa, 0x7f,
                           0x41, 0xb0, 0xf5, 0x21, 0x68, 0x68, 0xe6}};

/// Handle service notification events
/// @param event received event
/// @return event handler status
static SVCCTL_EvtAckStatus_t EventHandler(void* event);

/// Add reboot request characteristic
/// @param service the service that holds the characteristic
static void AddRebootCharacteristic(struct _tService* service);

void Reboot_Create() {
  // register service handle; needed for reboot request
  SVCCTL_RegisterSvcHandler(EventHandler);

  _rebootService.serviceHandle =
      BleGatt_AddPrimaryService(rebootServiceServiceId, 1);
  ASSERT(_rebootService.serviceHandle != 0);

  AddRebootCharacteristic(&_rebootService);
}

static SVCCTL_EvtAckStatus_t EventHandler(void* event) {
  hci_event_pckt* event_pckt = (hci_event_pckt*)(((hci_uart_pckt*)event)->data);
  switch (event_pckt->evt) {
    case HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE: {
      evt_blecore_aci* blecore_evt = (evt_blecore_aci*)event_pckt->data;
      switch (blecore_evt->ecode) {
        case ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE: {
          aci_gatt_attribute_modified_event_rp0* attribute_modified =
              (aci_gatt_attribute_modified_event_rp0*)blecore_evt->data;
          if (attribute_modified->Attr_Handle ==
              _rebootService.rebootCharacteristicHandle + 1) {
            /// write the data to SRAM and reset the device
            memcpy((uint8_t*)SRAM1_BASE, attribute_modified->Attr_Data,
                   attribute_modified->Attr_Data_Length);
            NVIC_SystemReset();
          }
          break;
        }
        default:
          break;
      };
      break;
    }
    default:
      break;
  }
  return SVCCTL_EvtNotAck;
}

static void AddRebootCharacteristic(struct _tService* service) {
  BleTypes_Characteristic_t characteristic = {
      .uuid.uuidType = BLE_TYPES_UUID128,
      .uuid.uuid.Char_UUID_128 = {0x19, 0xed, 0x82, 0xae, 0xed, 0x21, 0x4c,
                                  0x9d, 0x41, 0x45, 0x22, 0x8e, 0x11, 0xFE,
                                  0x00, 0x00},
      .maxValueLength = 3,
      .characteristicPropertyFlags = CHAR_PROP_WRITE_WITHOUT_RESP,
      .securityFlags = SECURE_ACCESS,
      .eventFlags = GATT_NOTIFY_ATTRIBUTE_WRITE,
      .encryptionKeySize = 10,
      .isVariableLengthValue = false};
  uint8_t buffer[3] = {0, 0, 0};
  service->rebootCharacteristicHandle = BleGatt_AddCharacteristic(
      service->serviceHandle, &characteristic, buffer, 3);
  ASSERT(service->rebootCharacteristicHandle != 0);
}
