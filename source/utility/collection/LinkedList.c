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

/// @file LinkedList.c
///
/// Implementation of the Linked List

#include "LinkedList.h"

#include "utility/ErrorHandler.h"
#include "utility/concurrency/Concurrency.h"

#include <string.h>

void LinkedList_Create(LinkedList_List_t* list) {
  list->head.next = &list->head;
  list->nrOfElements = 0;
}

void LinkedList_Append(LinkedList_List_t* list, LinkedList_Node_t* node) {
  // in case the node is already in a list, we have a problem
  ASSERT(list != 0);
  ASSERT(node->next == 0);
  LinkedList_Node_t* current = &list->head;
  uint32_t priorityMask = Concurrency_EnterCriticalSection();
  while (current->next != &list->head) {
    current = current->next;
  }
  node->next = current->next;
  current->next = node;
  list->nrOfElements += 1;
  Concurrency_LeaveCriticalSection(priorityMask);
}

void LinkedList_Insert(LinkedList_List_t* list, LinkedList_Node_t* node) {
  // in case the node is already in a list, we have a problem
  ASSERT(list != 0);
  ASSERT(node->next == 0);
  LinkedList_Node_t* current = &list->head;
  uint32_t priorityMask = Concurrency_EnterCriticalSection();
  node->next = current->next;
  current->next = node;
  list->nrOfElements += 1;
  Concurrency_LeaveCriticalSection(priorityMask);
}

bool LinkedList_Remove(LinkedList_List_t* list, LinkedList_Node_t* node) {
  // the head of the list cannot be removed
  ASSERT(list != 0);
  ASSERT(node != &list->head);
  bool elementIsInList = false;
  LinkedList_Node_t* current = &list->head;
  LinkedList_Node_t* previous = current;
  uint32_t priorityMask = Concurrency_EnterCriticalSection();
  while (current->next != &list->head) {
    if (current == node) {
      elementIsInList = true;
      break;
    }
    previous = current;
    current = current->next;
  }
  if (!elementIsInList) {
    Concurrency_LeaveCriticalSection(priorityMask);
    return false;
  }
  previous->next = current->next;
  node->next = 0;
  list->nrOfElements -= 1;
  Concurrency_LeaveCriticalSection(priorityMask);
  return true;
}

void LinkedList_Empty(LinkedList_List_t* list) {
  LinkedList_Node_t* current = &list->head;
  uint32_t priorityMask = Concurrency_EnterCriticalSection();

  // reset all link information
  while (current->next != &list->head) {
    LinkedList_Node_t* previous = current;
    current = current->next;
    previous->next = 0;
  }
  current->next = 0;  // the last element needs to be reset as well.
  list->nrOfElements = 0;
  list->head.next = &list->head;
  Concurrency_LeaveCriticalSection(priorityMask);
}

void LinkedList_ForEach(LinkedList_List_t* list, ProcessNodeCb_t processCb) {
  LinkedList_Node_t* current = &list->head;
  while (current->next != &list->head) {
    processCb(current->next);
    current = current->next;
  }
}

void LinkedList_IteratorInit(LinkedList_List_t* list,
                             LinkedList_Iterator_t* iterator) {
  iterator->node = &list->head;
  iterator->hasMoreElements = iterator->node->next != &list->head;
}

void LinkedList_Iterate(LinkedList_List_t* list,
                        LinkedList_Iterator_t* iterator) {
  iterator->node = iterator->node->next;
  iterator->hasMoreElements = iterator->node->next != &list->head;
}
