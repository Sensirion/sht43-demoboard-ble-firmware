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
/// @file ListTest.c
///
/// Implementation of test case

#include "ListTest.h"

#include "utility/ErrorHandler.h"
#include "utility/collection/LinkedList.h"
#include "utility/log/Log.h"

/// The node type used in this list test
typedef struct _tListTest_TestNode {
  LinkedList_Node_t listNode;  ///< link information
  uint8_t value;               ///< a value to identify the node
} ListTest_TestNode_t;

/// test queue
static LinkedList_List_t gList;

/// buffer held by test queue
static ListTest_TestNode_t gNode[16];

/// Print the actual value of the node
/// @param node element that is processed.
/// @return returns always true
static bool PrintNodeValue(LinkedList_Node_t* node);

void ListTest_InsertRemoveElements() {
  LinkedList_Create(&gList);
  for (uint8_t i = 0; i < 8; i++) {
    gNode[i].value = i;
    LinkedList_Append(&gList, (LinkedList_Node_t*)&gNode[i]);
  }
  for (uint8_t i = 8; i < 16; i++) {
    gNode[i].value = i;
    LinkedList_Insert(&gList, (LinkedList_Node_t*)&gNode[i]);
  }
  for (uint8_t i = 3; i < 7; i++) {
    LinkedList_Remove(&gList, (LinkedList_Node_t*)&gNode[i]);
  }
  LOG_INFO("Nr of elements in queue %i", gList.nrOfElements);
  LinkedList_ForEach(&gList, PrintNodeValue);

  LinkedList_Iterator_t iterator;
  LinkedList_IteratorInit(&gList, &iterator);
  while (iterator.hasMoreElements) {
    LinkedList_Iterate(&gList, &iterator);
    PrintNodeValue(iterator.node);
  }
  LinkedList_Empty(&gList);
}

static bool PrintNodeValue(LinkedList_Node_t* node) {
  ListTest_TestNode_t* testNode = (ListTest_TestNode_t*)node;
  LOG_INFO("processing node with value %i", testNode->value);
  return true;
}
