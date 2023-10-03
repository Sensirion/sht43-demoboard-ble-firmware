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

/// @file DeviceSettingsService.h
#ifndef DEVICE_SETTINGS_SERVICE_H
#define DEVICE_SETTINGS_SERVICE_H

#include "app_service/item_store/ItemStore.h"

#include <stdbool.h>
#include <stdint.h>

/// Setup the device settings service
/// The required fields are specified in
/// https://github.com/Sensirion/ble-services/blob/main/ble-services.yml
void DeviceSettingsService_Create();

/// Update the service version
/// @param version actual version of the device settings
void DeviceSettingsService_UpdateVersion(uint8_t version);

/// Update isLogEnabled
/// @param isLogEnabled actual value of the isLogEnabled flag
void DeviceSettingsService_UpdateIsLogEnabled(bool isLogEnabled);

/// Update is advertise data enabled
/// @param isAdvertiseDataEnabled value of the alternative device name
void DeviceSettingsService_UpdateIsAdvertiseDataEnabled(
    bool isAdvertiseDataEnabled);

/// Update alternative device name
/// @param alternativeName value of the alternative device name
void DeviceSettingsService_UpdateAlternativeDeviceName(
    const char* alternativeName);

#endif  // DEVICE_SETTINGS_SERVICE_H
