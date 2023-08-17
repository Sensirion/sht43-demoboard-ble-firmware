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

/// @file MessageListener.h
///
/// The MessageListener describes the structure of a client of the
/// MessageBroker that listens to messages

#ifndef MESSAGE_LISTENER_H
#define MESSAGE_LISTENER_H

#include "Message.h"
#include "assert.h"
#include "utility/collection/LinkedList.h"

/// Callback function to handle received information
typedef bool (*MessageListener_HandleReceivedMessageCb_t)(
    Message_Message_t* message);

/// Definition of a listener.
///
/// A listener may register in the message broker (only in one at a time).
/// By setting its receive mask it manifests its interest in a specific
/// category of information @see MessageBroker_Category_t
typedef struct _tMessageListener_Listener {
  LinkedList_Node_t listNode;  ///< list node. We have an intrusive list.
  uint16_t receiveMask;        ///< Bitmask with the interested categories set
  MessageListener_HandleReceivedMessageCb_t
      currentMessageHandlerCb;  ///< Current message handler
} MessageListener_Listener_t;

#endif  // MESSAGE_LISTENER_H
