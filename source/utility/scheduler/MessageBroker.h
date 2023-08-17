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

/// @file MessageBroker.h
///
/// The MessageBrokers is the central part of a little framework that is used
/// to distribute information within the application and to keep the different
/// components of the application independent from each other.
/// It allows consumers of information to register. Producer of information
/// can publish their messages to the MessageBroker and the MessageBroker
/// will distribute the information to the interested listeners.
/// The listener will declare their interest in specific information with a
/// Bitmask. Each message is associated to a category that is represented by
/// a single bit.

#ifndef MESSAGE_BROKER_H
#define MESSAGE_BROKER_H

#include "Message.h"
#include "MessageListener.h"
#include "assert.h"
#include "utility/collection/CyclicBuffer.h"
#include "utility/scheduler/Scheduler.h"

/// Definition of the message broker
typedef struct _tMessageBroker_Broker {
  LinkedList_List_t listeners;             ///< The collection with listeners
  CyclicBuffer_Buffer_t messageQueue;      ///< The message queue
  Message_Message_t currentMessage;        ///< Storage for the current message
                                           ///< that needs to be dispatched.
  uint32_t taskBitmap;                     ///< The bitmap used in the scheduler
  Scheduler_SchedulerPriority_t priority;  ///< The priority in the scheduler
  ProcessNodeCb_t messageDispatchCb;  ///< Pointer to message dispatch callback

} MessageBroker_Broker_t;

/// Create a message broker by initializing its members
/// @param broker The broker instance
/// @param messageBuffer A pointer to the storage for the message queue
/// @param capacity The number of elements the storage can take. Value needs
///                 to be in range [2,256] @see CyclicBuffer_Buffer_t
/// @param id       The id the message broker will have when running the
///                 scheduler. Value needs to be in range [0,31].
/// @param priority The priority the message broker will have when running
///                 in the scheduler.
void MessageBroker_Create(MessageBroker_Broker_t* broker,
                          uint64_t* messageBuffer,
                          uint16_t capacity,
                          uint8_t id,
                          Scheduler_SchedulerPriority_t priority);

/// Register a listener in the message broker
/// @param broker The instance of the message broker
/// @param listener The instance of the listener to be registered
void MessageBroker_RegisterListener(MessageBroker_Broker_t* broker,
                                    MessageListener_Listener_t* listener);

/// Unregister a listener in the message broker
/// @param broker The instance of the message broker
/// @param listener The instance of the listener to be registered
void MessageBroker_UnregisterListener(MessageBroker_Broker_t* broker,
                                      MessageListener_Listener_t* listener);

/// Put a message in the message queue of the broker such it can be published
/// to all registered listeners
/// @param broker The instance of the message broker
/// @param message The message to he published.
void MessageBroker_PublishMessage(MessageBroker_Broker_t* broker,
                                  Message_Message_t* message);

/// Function to be executed in the context of the scheduler to forward a
/// message to all registered listeners
/// @param broker The instance of the message broker
void MessageBroker_Run(MessageBroker_Broker_t* broker);

#endif  // MESSAGE_BROKER_H
