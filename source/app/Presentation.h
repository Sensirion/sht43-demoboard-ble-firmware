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

/// @file Presentation.h
///
/// The presentation controller is responsible for presenting
/// the application to the user via lCD and logging.
/// The PresentationController exposes the following state behavior:
///
/// @startuml
/// top to bottom direction
///
///  state AppNormalOperation {
///
///  state NormalOperation{
///
///  [*] -> DisplayPowerState
///
///  DisplayPowerState -> DisplayPowerState: evPowerStateChanged
///
///  --
///
///  [*] -> DisplayRelHumidity
///
///  DisplayRelHumidity -d-> DisplayAbsHumidity: evShortPress
///
///  DisplayAbsHumidity -u-> DisplayRelHumidity: evShortPress
///
///  --
///
///  [*] -> DisplayBleOn
///
///  DisplayBleOn  -d-> DisplayBleOff: evRadioOff
///
///  DisplayBleOff -u-> DisplayBleOn: evRadioOn
///
///  }
///
///  [*] -> NormalOperation
///
///  NormalOperation -d-> ErrorState: evError
///
///  NormalOperation -d-> TestState: evTestEnter
///
///  TestState -u-> NormalOperation: evTestLeave
///
///  ||
///
///  state BleRadio{
///
///  state cR <<choice>>
///
///  [*] -> FastAdvertising
///
///  FastAdvertising -d-> cR: evTimeTick
///
///  cR -d-> FastAdvertising: [timeElapsed < 30] / updateData()
///
///  cR -d-> LowPowerAdvertising: [timeElapsed >= 30] /updateData()
///
///  LowPowerAdvertising->LowPowerAdvertising: evTimeTick/ updateData()
///
///  --
///
///  state AdvertisementMode{
///
///  state which <<choice>>
///
///  [*] -d-> which : PowerState
///
///  which-d->connectable: PowerState(NO_RESTRICTION)
///
///  which-d->not_connectable: PowerState(REDUCED_OP)
///
///  }
///
///  }
///
///  [*] -> BleRadio
///
///  BleRadio -d-> BleRadioDisabled: PowerState(CRITICAL)
///
///  BleRadio -d-> BleRadioDisabled: evButtonLongPress
///
///  BleRadioDisabled -d-> BleRadio: evButtonLongPress
///
///  ||
///
///  [*] -> PowerCheck
///
///  PowerCheck --> NO_RESTRICTION: PowerState(NO_RESTRICTION)
///
///  PowerCheck --> REDUCED_OP: PowerState(REDUCED_OP)
///
///  PowerCheck --> CRITICAL: PowerState(CRITICAL)
///
///  NO_RESTRICTION--> CRITICAL: PowerState(CRITICAL)
///
///  REDUCED_OP--> CRITICAL: PowerState(CRITICAL)
///
///  }
///
///  state c <<choice>>
///
///  [*] -d-> AppBoot
///
///  AppBoot -d-> AppShowBleAddress: evPeripheralInitialized
///
///  AppShowBleAddress -> c: evTimeTick
///
///  AppShowBleAddress -> AppShowBleAddress: evButtonPress/changeUnits()
///
///  c --> AppShowBleAddress: [timeElapsed < 2]
///
///  c --> AppNormalOperation: [timeElapsed >= 2]
///
///  AppNormalOperation --> [*]: evReset
/// @enduml
///
/// The states are represented by a function that handles all occurring
/// events

#ifndef PRESENTATION_H
#define PRESENTATION_H

#include "utility/scheduler/MessageListener.h"

/// Initialize and get an instance of the presentation controller
/// @return An initialized pointer of the PresentationController
MessageListener_Listener_t* Presentation_ControllerInstance();

/// Allow to change the time step from external.
///
/// This function is used for testing and power consumption evaluation.
/// @param timeStepSeconds new time step value
void Presentation_setTimeStep(uint8_t timeStepSeconds);

#endif  // PRESENTATION_H
