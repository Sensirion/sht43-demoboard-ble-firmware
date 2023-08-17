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

/// @file LinkedList.h
///
/// The module LinkedList provides a simple list that can be used as an
/// ObserverList. Adding and removing nodes can be done within interrupt or
/// task context.
///
/// Differences to the stm_list:
/// - Not double linked list
/// - List type (not only node types) with allocated head node.
/// - Counts nr of elements in list.
/// - Minimal interface

#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stdbool.h>
#include <stdint.h>

/// A node of the linked list
typedef struct _tLinkedList_Node {
  struct _tLinkedList_Node* next;  ///< pointer to next element in the list
} LinkedList_Node_t;

/// A function that can process a node in the list
typedef bool (*ProcessNodeCb_t)(LinkedList_Node_t* node);

/// Minimalistic linked list.
typedef struct _tLinkedList_List {
  LinkedList_Node_t head;  ///< head of the list
  uint16_t nrOfElements;   ///< actual length of the list
} LinkedList_List_t;

/// Data structure to be used to iterate over the list
/// Using the iterator allows to iterate over the list without knowing its
/// internal implementation.
typedef struct _tLinkedList_Iterator {
  bool hasMoreElements;     ///<Tells if more elements are in the list
  LinkedList_Node_t* node;  ///<After each call to LinkedList_Iterate, a valid
                            ///<node is stored in this member.
} LinkedList_Iterator_t;

/// Create a new linked list by initializing its fields
/// @param list The list to be initialized.
void LinkedList_Create(LinkedList_List_t* list);

/// Append a node to the end of the list
/// @param list The list that will hold the new element
/// @param node The node to be inserted
void LinkedList_Append(LinkedList_List_t* list, LinkedList_Node_t* node);

/// Insert a new element in the beginning of the list
///
/// This operation is faster than appending.
/// @param list The list that will hold the new element
/// @param node The node to be inserted
void LinkedList_Insert(LinkedList_List_t* list, LinkedList_Node_t* node);

/// Remove a node from the list
/// @param list The list of elements.
/// @param node The node to be removed
/// @return True if the node was removed from the list, false otherwise.
bool LinkedList_Remove(LinkedList_List_t* list, LinkedList_Node_t* node);

/// Clear all entries from the list
///
/// @param list List to be cleared.
void LinkedList_Empty(LinkedList_List_t* list);

/// Apply a function to every element in the list
/// @param list The list with elements
/// @param processCb the function that shall be applied to each element
void LinkedList_ForEach(LinkedList_List_t* list, ProcessNodeCb_t processCb);

/// Initialize an iterator that can be used to access all elements in the
/// list.
/// After this call the value of iterator->node is not a valid member of the
/// list.
/// @param list list to iterate over
/// @param iterator holds the current state of iteration process
void LinkedList_IteratorInit(LinkedList_List_t* list,
                             LinkedList_Iterator_t* iterator);

/// Fetch the next element into the iterator
/// This function must only be called if the iterator->hasMoreElements member
/// was true.
/// @param list list to iterate over
/// @param iterator holds the current state of iteration process
void LinkedList_Iterate(LinkedList_List_t* list,
                        LinkedList_Iterator_t* iterator);

#endif  // LINKED_LIST_H
