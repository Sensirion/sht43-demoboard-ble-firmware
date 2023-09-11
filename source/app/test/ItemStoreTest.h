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

/// @file ItemStoreTest.h
#ifndef ITEM_STORE_TEST_H
#define ITEM_STORE_TEST_H

#include "app/SysTest.h"

// Defines the functions of the test group SYS_TEST_TEST_GROUP_PRESENTATION
/// This enum serves the documentation!
typedef enum {
  FUNCTION_ID_ADD_ITEM = 0,
  FUNCTION_ID_ADD_ITEMS_FROM_TIMER = 1,
  FUNCTION_ID_ENUMERATE_ITEMS = 2
} ItemStore_FunctionId_t;

/// Add an item to the item store
/// @param param parameters of the AddItem function
///              byteParameter[0] is the item to be added;
///              the added data are hard coded
///              byteParameter[1] is the number of items that shall be inserted
void ItemStoreTest_AddItem(SysTest_TestMessageParameter_t param);

/// Starts a timer that inserts items into the item store
/// @param param parameters of the TimerAddItem function
///              byteParameter[0] is the item to be added;
///              the added data are hard coded
///              shortParameter[1] is the number of items that shall be inserted
void ItemStoreTest_TimerAddItem(SysTest_TestMessageParameter_t param);

/// Enumerate the
/// @param param parameters of the EnumerateItems function
///              byteParameter[0] is the item to be added;
///              byteParameter[1] selects the semantics of next parameter
///              shortParameter[1] depends on value of byteParameter[1]:
///                 * 0 => number of items to be read from index 0
///                 * 1 => items from start index until end of enumeration
void ItemStoreTest_EnumerateItems(SysTest_TestMessageParameter_t param);

#endif  // ITEM_STORE_TEST_H
