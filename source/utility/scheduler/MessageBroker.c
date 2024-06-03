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

#include "MessageBroker.h"

#include "stm32_seq.h"
#include "utility/ErrorHandler.h"
#include "utility/log/Log.h"

void MessageBroker_Create(MessageBroker_Broker_t* broker,
                          uint64_t* messageBuffer,
                          uint16_t capacity,
                          uint8_t id,
                          Scheduler_SchedulerPriority_t priority) {
  LinkedList_Create(&broker->listeners);
  CyclicBuffer_Create(&broker->messageQueue, messageBuffer, capacity);
  broker->priority = priority;
  ASSERT(id < 32);
  broker->taskBitmap = 1 << id;
}

void MessageBroker_RegisterListener(MessageBroker_Broker_t* broker,
                                    MessageListener_Listener_t* listener) {
  LinkedList_Insert(&broker->listeners, (LinkedList_Node_t*)listener);
}

void MessageBroker_UnregisterListener(MessageBroker_Broker_t* broker,
                                      MessageListener_Listener_t* listener) {
  LinkedList_Remove(&broker->listeners, (LinkedList_Node_t*)listener);
}

void MessageBroker_PublishMessage(MessageBroker_Broker_t* broker,
                                  Message_Message_t* message) {
  bool scheduleNeeded = CyclicBuffer_IsEmpty(&broker->messageQueue);
  CyclicBuffer_Enqueue(&broker->messageQueue, (uint64_t*)message);
  if (scheduleNeeded) {
    UTIL_SEQ_SetTask(broker->taskBitmap, broker->priority);
  }
}

void MessageBroker_Run(MessageBroker_Broker_t* broker) {
  CyclicBuffer_Buffer_t* queue = &broker->messageQueue;

  if (CyclicBuffer_Dequeue(queue, (uint64_t*)&broker->currentMessage)) {
    LinkedList_Iterator_t iterator = {0};
    LinkedList_IteratorInit(&broker->listeners, &iterator);
    bool messageConsumed = false;
    while (iterator.hasMoreElements) {
      LinkedList_Iterate(&broker->listeners, &iterator);
      MessageListener_Listener_t* listener =
          (MessageListener_Listener_t*)iterator.node;
      if (0 !=
          (listener->receiveMask & broker->currentMessage.header.category)) {
        bool localConsumed =
            listener->currentMessageHandlerCb(&broker->currentMessage);
        messageConsumed = messageConsumed || localConsumed;
      }
    }
    if (!messageConsumed) {
      LOG_DEBUG("Message with id %i was not consumed",
                broker->currentMessage.header.id);
    }
  }
  if (!CyclicBuffer_IsEmpty(queue)) {
    UTIL_SEQ_SetTask(broker->taskBitmap, broker->priority);
  }
}
