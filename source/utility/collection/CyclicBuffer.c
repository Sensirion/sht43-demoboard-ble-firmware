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

/// @file CyclicBuffer.c
///
/// Implementation of the CyclicBuffer

#include "CyclicBuffer.h"

#include "utility/ErrorHandler.h"
#include "utility/concurrency/Concurrency.h"

#include <string.h>

/// Helper macro to make wrap around more readable
#define RESTRICT_BOUND(q, index) (q)->index >= (q)->capacity ? 0 : (q)->index

// We assume that the function is not called from different
// execution contexts.
void CyclicBuffer_Create(CyclicBuffer_Buffer_t* queue,
                         uint64_t* storage,
                         uint16_t capacity) {
  ASSERT(storage != 0);
  ASSERT(capacity > 1 && capacity <= 256);

  queue->capacity = capacity;
  queue->elementStorage = storage;
  queue->indexIn = 0;
  queue->indexOut = 0;
}

bool CyclicBuffer_Enqueue(CyclicBuffer_Buffer_t* queue,
                          const uint64_t* element) {
  uint32_t priorityMask = Concurrency_EnterCriticalSection();
  if (CyclicBuffer_IsFull(queue)) {
    Concurrency_LeaveCriticalSection(priorityMask);
    return false;
  }
  queue->elementStorage[queue->indexIn++] = *element;
  queue->indexIn = RESTRICT_BOUND(queue, indexIn);
  Concurrency_LeaveCriticalSection(priorityMask);
  return true;
}

bool CyclicBuffer_Dequeue(CyclicBuffer_Buffer_t* queue, uint64_t* element) {
  uint32_t priorityMask = Concurrency_EnterCriticalSection();
  if (CyclicBuffer_IsEmpty(queue)) {
    Concurrency_LeaveCriticalSection(priorityMask);
    return false;
  }
  *element = queue->elementStorage[queue->indexOut++];
  queue->indexOut = RESTRICT_BOUND(queue, indexOut);
  Concurrency_LeaveCriticalSection(priorityMask);
  return true;
}

void CyclicBuffer_Empty(CyclicBuffer_Buffer_t* queue) {
  uint32_t priorityMask = Concurrency_EnterCriticalSection();
  queue->indexIn = 0;
  queue->indexOut = 0;
  Concurrency_LeaveCriticalSection(priorityMask);
}

bool CyclicBuffer_IsEmpty(CyclicBuffer_Buffer_t* queue) {
  return queue->indexIn == queue->indexOut;
}

bool CyclicBuffer_IsFull(CyclicBuffer_Buffer_t* queue) {
  uint16_t nextIndex = queue->indexIn + 1;
  nextIndex = nextIndex >= queue->capacity ? 0 : nextIndex;
  return nextIndex == queue->indexOut;
}
