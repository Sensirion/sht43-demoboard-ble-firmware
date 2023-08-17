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

/// @file BleTypes.h
///
/// Type definitions used by BleService and application

#ifndef BLE_TYPES_H
#define BLE_TYPES_H

#include "app_conf.h"
// clang-format off
#include "core/ble_core.h"
#include "core/ble_bufsize.h"
#include "core/ble_defs.h"
#include "core/ble_legacy.h"
#include "core/ble_std.h"
// clang-format on
#include "hci_tl.h"
#include "svc/Inc/svc_ctl.h"

#include <stdbool.h>

/// Defines the vendor id;
#define BLE_TYPES_SENSIRION_VENDOR_ID 0x06D5

/// security parameters structure
typedef struct _tSecurityParams {
  /// IO capability of the device
  uint8_t ioCapability;
  /// Authentication requirement of the device
  /// Man In the Middle protection required?
  uint8_t mitmMode;
  /// bonding mode of the device
  uint8_t bondingMode;
  /// this variable indicates whether to use a fixed pin
  /// during the pairing process or a passkey has to be
  /// requested to the application during the pairing process
  /// 0 implies use fixed pin and 1 implies request for passkey
  uint8_t useFixedPin;
  /// minimum encryption key size requirement
  uint8_t encryptionKeySizeMin;
  /// maximum encryption key size requirement
  uint8_t encryptionKeySizeMax;
  /// fixed pin to be used in the pairing process if
  /// Use_Fixed_Pin is set to 1
  uint32_t fixedPin;
  /// this flag indicates whether the host has to initiate
  /// the security, wait for pairing or does not have any security
  /// requirements.
  /// 0x00 : no security required
  /// 0x01 : host should initiate security by sending the slave security
  ///        request command
  /// 0x02 : host need not send the clave security request but it
  /// has to wait for paiirng to complete before doing any other
  /// processing
  uint8_t initiateSecurity;
} BleTypes_SecurityParams_t;

/// Identifier for the current state of the Ble Interface
typedef enum {
  BLE_INTERFACE_IDLE,
  BLE_INTERFACE_ADVERTISING,
  BLE_INTERFACE_LP_CONNECTING,
  BLE_INTERFACE_CONNECTED_SERVER,
  BLE_INTERFACE_CONNECTED_CLIENT
} BleTypes_ConnStatus_t;

/// Defines the interval that is used to advertise
typedef enum {
  ADVERTISEMENT_INTERVAL_LONG = 0,  ///< ~5s
  ADVERTISEMENT_INTERVAL_MEDIUM,    ///< ~2s
  ADVERTISEMENT_INTERVAL_SHORT,     ///< ~80 ms
} BleTypes_AdvertisementInterval_t;

/// Specifies how to advertise
typedef union _tBleTypes_AdvertisementMode {
  struct {
    bool connectable;  ///< If true make the device connectable
    /// specifies the used advertisement interval
    BleTypes_AdvertisementInterval_t interval;
    ///< else slow advertisement.
  } advertiseModeSpecification;  ///< define how to advertise
  uint16_t compare;              ///< for easy comparison of two modes
} BleTypes_AdvertisementMode_t;

/// Defines the advertisement data of the SHT4x Demo Board
typedef struct __attribute__((__packed__)) {
  uint8_t adTypeSize;              ///< size type
  uint8_t adTypeFlag;              ///< flag type
  uint8_t adTypeValue;             ///< value type
  uint8_t adTypeManufacturerSize;  ///<size manufacturer
  uint8_t adTypeManufacturerFlag;  ///<flag manufacturer
  uint16_t companyIdentifier;      ///<value manufacturer
  uint8_t sAdvT;                   ///< advertisement type
  uint8_t sampleType;              ///< format tag of subsequent data
  uint16_t deviceId;               ///< device id         (custom data)
  uint16_t temperatureTicks;       ///< temperature ticks (custom data)
  uint16_t humidityTicks;          ///< humidity ticks    (custom data)
  uint8_t adTypeNameSize;          ///< size device name
  uint8_t adTypeNameFlag;          ///< type device name
  uint8_t name[8];                 ///< value device name
} BleTypes_AdvertisementData_t;

/// global context containing the variables common to all services
typedef struct _tBLEProfileGlobalContext {
  /// security requirements of the host
  BleTypes_SecurityParams_t bleSecurityParam;
  /// gap service handle
  uint16_t gapServiceHandle;
  /// device name characteristic handle
  uint16_t devNameCharHandle;
  /// appearance characteristic handle
  uint16_t appearanceCharHandle;
  /// connection handle of the current active connection
  /// When not in connection, the handle is set to 0xFFFF
  uint16_t connectionHandle;
} BleTypes_GlobalContext_t;

/// Defines the possible types for gatt and service UUIDs.
typedef enum {
  BLE_TYPES_UUID16 = UUID_TYPE_16,
  BLE_TYPES_UUID128 = UUID_TYPE_128,
} BleTypes_UuidType;

/// Allows to specify the Uuid of a characteristic or service
typedef struct _tBleTypes_Uuid {
  BleTypes_UuidType uuidType;  ///< Specify if the uuid is 16 or 128 bits
  Char_UUID_t uuid;            ///< uuid value
} BleTypes_Uuid_t;

/// Allow to specify a characteristic of a service
///
typedef struct _tBleTypes_Characteristic {
  BleTypes_Uuid_t uuid;                 ///< uuid of the characteristic
  uint16_t maxValueLength;              ///< maximum length of a characteristic
  uint8_t characteristicPropertyFlags;  ///< property flags of a characteristic
  uint8_t securityFlags;                ///< security flags of a characteristic
  uint8_t eventFlags;                   ///< event flags of a characteristic
  uint8_t
      encryptionKeySize;  ///< size of key used to encrypt the characteristic
  bool isVariableLengthValue;  ///< true if value has variable size;
                               ///< false otherwise
} BleTypes_Characteristic_t;

/// Context required to run the BLE application
typedef struct {
  /// BLE application context
  BleTypes_GlobalContext_t bleApplicationContextLegacy;
  /// Connection Status
  BleTypes_ConnStatus_t deviceConnectionStatus;
  uint64_t timeRunningTick;  ///< the time the ble app is running
  BleTypes_AdvertisementData_t* advertisementData;  ///< advertisement data
  uint8_t advertisementDataSize;                    ///< size of advertisement
  BleTypes_AdvertisementMode_t currentAdvertisementMode;  ///< current mode
} BleTypes_ApplicationContext_t;

/// Helper type to ease the creation of a bluetooth device address
typedef union {
  uint8_t addressBytes[6];   ///<byte representation
  uint32_t addressWords[2];  ///<word representation
} BleTypes_BleDeviceAddress_t;

#endif  // BLE_TYPES_H
