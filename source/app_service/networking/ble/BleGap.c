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

/// @file BleGap.c
///
/// Source file for all advertising (GAP) services

#include "BleGap.h"

#include "BleHelper.h"
#include "utility/AppDefines.h"
#include "utility/ErrorHandler.h"
#include "utility/log/Log.h"

/// GAP_APPEARANCE configuration value
#define BLE_APPEARANCE_MULTISENSOR 0x0552

/// Actually used GAP appearance value
#define BLE_GAP_APPEARANCE (BLE_APPEARANCE_MULTISENSOR)

/// Helper structure to define the different advertisement intervals
typedef struct {
  uint32_t min;  ///< minimal interval time in ticks
  uint32_t max;  ///< maximal interval time in ticks
} AdvertiseInterval_t;

/// Mapping from advertisement interval type to the range that belongs to it.
static AdvertiseInterval_t _advertiseIntervalSpec[] = {
    [ADVERTISEMENT_INTERVAL_LONG] = {.min = LONG_LONG_ADVERTISE_INTERVAL_MIN,
                                     .max = LONG_LONG_ADVERTISE_INTERVAL_MAX},
    [ADVERTISEMENT_INTERVAL_MEDIUM] = {.min = LONG_ADVERTISE_INTERVAL_MIN,
                                       .max = LONG_ADVERTISE_INTERVAL_MAX},
    [ADVERTISEMENT_INTERVAL_SHORT] = {.min = SHORT_ADVERTISE_INTERVAL_MIN,
                                      .max = SHORT_ADVERTISE_INTERVAL_MAX}};

void BleGap_Init(BleTypes_ApplicationContext_t* bleApplicationContext) {
  uint8_t role = GAP_PERIPHERAL_ROLE;
  tBleStatus ret = BLE_STATUS_INVALID_PARAMS;
  uint16_t gapServiceHandle;
  uint16_t gapDevNameCharHandle;
  uint16_t gapAppearanceCharHandle;
  uint16_t appearance[1] = {BLE_GAP_APPEARANCE};

  // Initialize GAP interface
  uint8_t nameLength = BLE_TYPES_LOCAL_NAME_LENGTH;
  ret = aci_gap_init(role, CFG_PRIVACY, nameLength, &gapServiceHandle,
                     &gapDevNameCharHandle, &gapAppearanceCharHandle);
  LOG_DEBUG_CALLSTATUS("aci_gap_init()", ret);

  ret = aci_gatt_update_char_value(gapServiceHandle, gapDevNameCharHandle, 0,
                                   nameLength,
                                   (uint8_t*)bleApplicationContext->localName);
  if (ret != BLE_STATUS_SUCCESS) {
    ErrorHandler_UnrecoverableError(ERROR_CODE_BLE);
  }

  ret = aci_gatt_update_char_value(gapServiceHandle, gapAppearanceCharHandle, 0,
                                   2, (uint8_t*)&appearance);
  if (ret != BLE_STATUS_SUCCESS) {
    ErrorHandler_UnrecoverableError(ERROR_CODE_BLE);
  }

  // Initialize authentication
  bleApplicationContext->bleApplicationContextLegacy.bleSecurityParam.mitmMode =
      CFG_MITM_PROTECTION;
  bleApplicationContext->bleApplicationContextLegacy.bleSecurityParam
      .encryptionKeySizeMin = CFG_ENCRYPTION_KEY_SIZE_MIN;
  bleApplicationContext->bleApplicationContextLegacy.bleSecurityParam
      .encryptionKeySizeMax = CFG_ENCRYPTION_KEY_SIZE_MAX;
  bleApplicationContext->bleApplicationContextLegacy.bleSecurityParam
      .useFixedPin = CFG_USED_FIXED_PIN;
  bleApplicationContext->bleApplicationContextLegacy.bleSecurityParam.fixedPin =
      CFG_FIXED_PIN;
  bleApplicationContext->bleApplicationContextLegacy.bleSecurityParam
      .bondingMode = CFG_BONDING_MODE;

  ret = aci_gap_set_authentication_requirement(
      bleApplicationContext->bleApplicationContextLegacy.bleSecurityParam
          .bondingMode,
      bleApplicationContext->bleApplicationContextLegacy.bleSecurityParam
          .mitmMode,
      CFG_SC_SUPPORT, CFG_KEYPRESS_NOTIFICATION_SUPPORT,
      bleApplicationContext->bleApplicationContextLegacy.bleSecurityParam
          .encryptionKeySizeMin,
      bleApplicationContext->bleApplicationContextLegacy.bleSecurityParam
          .encryptionKeySizeMax,
      bleApplicationContext->bleApplicationContextLegacy.bleSecurityParam
          .useFixedPin,
      bleApplicationContext->bleApplicationContextLegacy.bleSecurityParam
          .fixedPin,
      CFG_IDENTITY_ADDRESS);

  LOG_DEBUG_CALLSTATUS("aci_gap_set_authentication_requirement()", ret);

  // Initialize whitelist
  if (bleApplicationContext->bleApplicationContextLegacy.bleSecurityParam
          .bondingMode) {
    ret = aci_gap_configure_whitelist();
    LOG_DEBUG_CALLSTATUS("aci_gap_configure_whitelist()", ret);
  }

  // Initialize IO capability
  bleApplicationContext->bleApplicationContextLegacy.bleSecurityParam
      .ioCapability = CFG_IO_CAPABILITY;
  ret = aci_gap_set_io_capability(
      bleApplicationContext->bleApplicationContextLegacy.bleSecurityParam
          .ioCapability);
  LOG_DEBUG_CALLSTATUS("aci_gap_set_io_capability()", ret);
}

void BleGap_AdvertiseRequest(BleTypes_ApplicationContext_t* applicationContext,
                             BleTypes_AdvertisementMode_t mode) {
  tBleStatus ret = BLE_STATUS_INVALID_PARAMS;

  if ((applicationContext->deviceConnectionStatus ==
       BLE_INTERFACE_ADVERTISING) &&
      (applicationContext->currentAdvertisementMode.compare != mode.compare)) {
    aci_gap_set_non_discoverable();
    applicationContext->deviceConnectionStatus = BLE_INTERFACE_IDLE;
  }

  if (applicationContext->deviceConnectionStatus == BLE_INTERFACE_IDLE) {
    applicationContext->deviceConnectionStatus = BLE_INTERFACE_ADVERTISING;

    // evaluate advertisement mode
    uint8_t advertisementType =
        mode.advertiseModeSpecification.connectable ? ADV_IND : ADV_NONCONN_IND;
    uint16_t minInterval =
        _advertiseIntervalSpec[mode.advertiseModeSpecification.interval].min;
    uint16_t maxInterval =
        _advertiseIntervalSpec[mode.advertiseModeSpecification.interval].max;

    // Start Fast or Low Power Advertising
    ret = aci_gap_set_discoverable(advertisementType, minInterval, maxInterval,
                                   CFG_BLE_ADDRESS_TYPE, ADV_FILTER, 0, 0, 0, 0,
                                   0, 0);
    if (ret == BLE_STATUS_SUCCESS) {
      applicationContext->currentAdvertisementMode = mode;
    }
  }

  if (applicationContext->deviceConnectionStatus == BLE_INTERFACE_ADVERTISING) {
    // Update Advertising data
    ret = aci_gap_update_adv_data(
        applicationContext->advertisementDataSize,
        (uint8_t*)applicationContext->advertisementData);
    LOG_DEBUG_CALLSTATUS("aci_gap_update_adv_data()", ret);
  }
}

void BleGap_AdvertiseCancel(BleTypes_ApplicationContext_t* applicationContext) {
  if (applicationContext->deviceConnectionStatus !=
      BLE_INTERFACE_CONNECTED_SERVER) {
    [[maybe_unused]] tBleStatus ret = BLE_STATUS_INVALID_PARAMS;
    ret = aci_gap_set_non_discoverable();
    applicationContext->deviceConnectionStatus = BLE_INTERFACE_IDLE;
    LOG_DEBUG_CALLSTATUS("aci_gap_set_non_discoverable()", ret);
  }
}
