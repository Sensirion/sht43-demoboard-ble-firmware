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

/// @file CyclicBuffer.h
///
/// The module CyclicBuffer provides a simple queue that can be used
/// from within interrupt context. The current use case is to send messages
/// from a producer to one or many listeners.
///
/// Differences to the stm_queue:
/// - Can be used from within interrupt context
/// - Fixed size of elements (8 bytes). In case the usage of this
///   data structure is extended we may decide to change this.
/// - Reduced interface

#ifndef CYCLIC_BUFFER_H
#define CYCLIC_BUFFER_H

#include <stdbool.h>
#include <stdint.h>

/// Minimalistic cyclic buffer data structure.
typedef struct _tCyclicBuffer_Buffer {
  uint16_t capacity;         ///< The capacity of the queue!
  uint64_t* elementStorage;  ///< The buffer that holds the elements in the
                             ///< queue
  ///< All elements are copied and contained by value.
  uint16_t indexIn;   ///< index in the buffer to insert the next element
  uint16_t indexOut;  ///< index in the buffer to take out the next element
} CyclicBuffer_Buffer_t;

/// Create a queue by initializing the queue parameter with the appropriate
/// values
/// @param queue Queue object to be initialized
/// @param storage  Memory to store the elements in the buffer.
/// @param capacity Capacity of the queue. The queue will be able to store at
///                 most capacity - 1 values.
///                 The allowed value range is restricted to [2, 256]
///
void CyclicBuffer_Create(CyclicBuffer_Buffer_t* queue,
                         uint64_t* storage,
                         uint16_t capacity);

/// Put a new element into the queue.
/// @param queue The queue that shall contain the new element
/// @param element The element to be put (it will be copied)
/// @return True if the element was added, false if the queue was full
bool CyclicBuffer_Enqueue(CyclicBuffer_Buffer_t* queue,
                          const uint64_t* element);

/// Get a value from the queue and remove it
/// @param queue The queue with elements
/// @param element A pointer where the element can be copied.
/// @return True if the element was copied successfully, false if the queue
///         was empty.
bool CyclicBuffer_Dequeue(CyclicBuffer_Buffer_t* queue, uint64_t* element);

/// Remove all elements from the queue
/// @param queue The queue to be emptied
void CyclicBuffer_Empty(CyclicBuffer_Buffer_t* queue);

/// Query if the queue is empty
/// @param queue Queue to be queried
/// @return True if the queue is empty, false otherwise.
bool CyclicBuffer_IsEmpty(CyclicBuffer_Buffer_t* queue);

/// Query if the queue is full
/// @param queue Queue to be queried
/// @return True if the queue is full, false otherwise.
bool CyclicBuffer_IsFull(CyclicBuffer_Buffer_t* queue);

#endif  // CYCLIC_BUFFER_H
