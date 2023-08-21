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

/// @file Message.h
///
/// Describes the message structure that is used to pass information
/// between different parts of the application.

#ifndef MESSAGE_H
#define MESSAGE_H

#include "utility/StaticCodeAnalysisHelper.h"

#include <stdint.h>

/// Defines the information categories that are exchanged in the application
typedef enum {
  MESSAGE_BROKER_CATEGORY_TIME_INFORMATION = 0x01,
  MESSAGE_BROKER_CATEGORY_BUTTON_EVENT = 0x02,
  MESSAGE_BROKER_CATEGORY_SENSOR_VALUE = 0x04,
  MESSAGE_BROKER_CATEGORY_SYSTEM_STATE_CHANGE = 0x08,
  MESSAGE_BROKER_CATEGORY_BLE_EVENT = 0x10,
  MESSAGE_BROKER_CATEGORY_BATTERY_EVENT = 0x20,
  MESSAGE_BROKER_CATEGORY_RECOVERABLE_ERROR = 0x40,
  MESSAGE_BROKER_CATEGORY_TEST = 0x80,

} MessageBroker_Category_t;

ASSERT_SIZE_TYPE1_LESS_THAN_TYPE2(MessageBroker_Category_t, uint16_t);

/// Head of any message
typedef struct _tMessageBroker_MsgHead {
  uint8_t id;          ///< Identification of the message;
                       ///< Identification and category need to be unique
  uint8_t parameter1;  ///< one byte user payload of a message
  MessageBroker_Category_t category;  ///< category of the message;

} MessageBroker_MsgHead_t;

/// The message is the base type of any message that is published
/// within the system.
/// The first 4 bytes must not be changed. Parameter2 can be customized
/// according the information that needs to be carried.
/// The overall message must never exceed 8 bytes! It shall be asserted with
/// a static assert.
typedef struct _tMessage_Message {
  MessageBroker_MsgHead_t header;  ///< header of the message
  uint32_t parameter2;             ///< user payload of a message that can be
                                   ///< redefined in other messages
} Message_Message_t;

ASSERT_SIZE_TYPE1_LESS_THAN_TYPE2(Message_Message_t, uint64_t);

/// Publish a message to all registered listeners of the
/// Application message broker.
/// Upon sending the message will be copied. No locking is therefore required.
/// @param message the message that will be published.
void Message_PublishAppMessage(Message_Message_t* message);

#endif  // MESSAGE_H
