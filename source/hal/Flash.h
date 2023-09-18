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

/// @file Flash.h
///
/// Defines high level routines to read and write from  the internal flash.
///
/// The flash requires it's own task in order to support erasing multiple pages.
/// Even though the HAL supports erasing multiple pages in one step, this is not
/// allowed within an application that has ongoing BLE activity.
///
/// Therefore an erase of multiple pages is processed as a
/// sequence of single page erases.
/// Before each erase it needs to be checked, if the access to the flash is
/// allowed.
/// In case access to flash is blocked, the erase operation has to be stopped
/// and can only be resumed upon receiving the notification that the
/// corresponding semaphore has become free.
///
#ifndef FLASH_H
#define FLASH_H

#include <stdbool.h>
#include <stdint.h>

/// Callback to receive the status of the flash erase operation
/// The parameter pageNr holds the first Page that is erased or -1 if an error
/// occurred. The parameter remaining returns the nrOfPages that where NOT
/// erased. In case of a successful execution this parameter should be 0.
typedef void (*Flash_OperationComplete)(uint32_t pageNr, uint8_t remaining);

/// Initialize the flash
///
/// This enables the NVIC interrupt
void Flash_Init();

/// Read a memory block from flash starting at a given address.
/// @param address Start address of read operation; The address needs to be a
///                multiple of 8!
/// @param buffer Buffer to contain the read data
/// @param nrOfBytes Nr of bytes to read
/// @return true if operation successful; false otherwise
bool Flash_Read(uint32_t address, uint8_t* buffer, uint16_t nrOfBytes);

/// Write a memory block to the flash starting at a given address.
/// A write of a single double word takes 85us. This is the default case
/// in our application where we write two samples at a time.
/// Handling this asynchronously will not save a lot of power!
/// @param address Start address of the write operation; The address needs to be
///                a multiple of 8!
/// @param buffer The buffer containing the data to be written.
/// @param nrOfBytes Nr of bytes to be written; if the number of bytes is not a
///                  multiple of 8, the data will be padded with 0!
/// @return true if the write was successful, false otherwise
bool Flash_Write(uint32_t address, const uint8_t* buffer, uint16_t nrOfBytes);

/// Erase one or several pages staring from a specific page number
/// The pages are erased one by one. The callback is invoked only after
/// all pages are erased.
/// @param startPageNr First page to erase
/// @param nrOfPages Number of pages to erase; must be bigger than 0!
/// @param callback The callback indicates the end of the erase operation.
void Flash_Erase(uint16_t startPageNr,
                 uint8_t nrOfPages,
                 Flash_OperationComplete callback);

#endif  // FLASH_H
