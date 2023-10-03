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

/// @file System.c
///
/// Peripheral initialization and program start

#include "System.h"

#include "BleContext.h"
#include "SysTest.h"
#include "app/Presentation.h"
#include "app/test/FlashTest.h"
#include "app_service/item_store/ItemStore.h"
#include "app_service/item_store/MeasurementItemController.h"
#include "app_service/item_store/SettingsController.h"
#include "app_service/networking/HciTransport.h"
#include "app_service/networking/ble/BleInterface.h"
#include "app_service/nvm/ProductionParameters.h"
#include "app_service/power_manager/BatteryMonitor.h"
#include "app_service/power_manager/PowerManager.h"
#include "app_service/screen/Screen.h"
#include "app_service/sensor/SensorController.h"
#include "app_service/sensor/Sht4x.h"
#include "app_service/timer_server/TimerServer.h"
#include "app_service/user_button/Button.h"
#include "app_service/user_button/ButtonEvent.h"
#include "hal/Clock.h"
#include "hal/Flash.h"
#include "hal/Gpio.h"
#include "hal/I2c3.h"
#include "hal/Ipcc.h"
#include "hal/Qspi.h"
#include "hal/Rtc.h"
#include "hal/Uart.h"
#include "math.h"
#include "stdarg.h"
#include "stm32_lpm.h"
#include "stm32_seq.h"
#include "utility/AppDefines.h"
#include "utility/ErrorHandler.h"
#include "utility/log/Log.h"
#include "utility/log/Trace.h"
#include "utility/scheduler/MessageBroker.h"
#include "utility/scheduler/MessageId.h"
#include "utility/scheduler/Scheduler.h"

/// Type definition for the message broker initialization
typedef struct _tMessageBus {
  MessageBroker_Broker_t broker;  ///< the message broker to be initialized
  uint64_t messages[32];          ///< the storage for the message queue
  uint8_t taskId;                 ///< the task id of the message bus task
  Scheduler_SchedulerPriority_t priority;  ///< the task priority
  void (*taskFunction)();  ///< the task Function (could be avoided)
} MessageBus_t;

/// Manage peripherals during runtime and preprocessing
static void RunSystem(void);

/// Initialize the application message broker
///
/// The application message broker must never send messages to
/// the ble application directly!
/// @param config data to start the message broker
/// @param nrOfObservers nr of observers
/// @param ... listeners that will register
static void InitMessageBroker(MessageBus_t* config, int nrOfObservers, ...);

/// Task function to run the message broker in the scheduler
static void RunAppMessageDispatch();

/// Task function to run the message broker in the scheduler
/// could be avoided
static void RunBleMessageDispatch();

/// instance of application message broker
static MessageBus_t _appMessageBroker = {
    .taskFunction = RunAppMessageDispatch,
    .taskId = SCHEDULER_TASK_HANDLE_APP_MESSAGES,
    .priority = SCHEDULER_PRIO_1};

/// instance of ble message broker
static MessageBus_t _bleMessageBroker = {
    .taskFunction = RunBleMessageDispatch,
    .taskId = SCHEDULER_TASK_HANDLE_BLE_MESSAGE,
    .priority = SCHEDULER_PRIO_0};

void System_Init(void) {
  // Setup the message passing infrastructure.
  // This is the first thing to do since errors are also propagated as
  // messages
  InitMessageBroker(
      &_appMessageBroker, 8, SysTest_TestControllerInstance(),
      BatteryMonitor_Instance(), SensorController_Sht4xControllerInstance(),
      BleContext_BridgeInstance(), Presentation_ControllerInstance(),
      ItemStore_ListenerInstance(), MeasurementItemController_Instance(),
      SettingsController_Instance());

  InitMessageBroker(&_bleMessageBroker, 1, BleContext_Instance());

  // accesses the flash to read production parameters
  ProductionParameters_Init();
  Clock_ConfigureSystemAndPeripheralClocks(ProductionParameters_GetHseTuning());

  // initialize peripherals
  Trace_Init(Uart_WriteBlocking);
  LOG_DEBUG("%s\n", "Initialize Peripherals {");

  Gpio_InitClocks();

  Flash_Init();

  Screen_Init();

  PowerManger_Init();

  // Initialized with ~2050 Ticks per second
  TimerServer_Init(Rtc_Instance());

  HciTransport_Init(BleContext_StartBluetoothApp);

  Button_Init(ButtonEvent_PublishShortPressEvent,
              ButtonEvent_PublishLongPressEvent);

  LOG_DEBUG("%s\n", "} SUCCESS!\n");

  Sht4x_Init(&_appMessageBroker.broker);

  Uart_RegisterRxHandler(SysTest_GetUartReceiver());

  ItemStore_Init();

  RunSystem();
}

static void RunSystem(void) {
  // trigger the initialization of the application

  Message_Message_t peripheralInitialized = {
      .header.category = MESSAGE_BROKER_CATEGORY_SYSTEM_STATE_CHANGE,
      .header.id = MESSAGE_ID_PERIPHERALS_INITIALIZED};
  Message_PublishAppMessage(&peripheralInitialized);

  while (1) {
    UTIL_SEQ_Run(UTIL_SEQ_DEFAULT);
  }
}

static void InitMessageBroker(MessageBus_t* config, int nrOfObservers, ...) {
  MessageBroker_Create(&config->broker, config->messages,
                       COUNT_OF(config->messages), config->taskId,
                       config->priority);
  va_list args;
  va_start(args, nrOfObservers);
  for (int i = 0; i < nrOfObservers; i++) {
    MessageListener_Listener_t* node =
        va_arg(args, MessageListener_Listener_t*);
    MessageBroker_RegisterListener(&config->broker, node);
  }
  va_end(args);
  UTIL_SEQ_RegTask(config->broker.taskBitmap, UTIL_SEQ_RFU,
                   config->taskFunction);
}

// override ble interface
void BleInterface_PublishBleMessage(Message_Message_t* msg) {
  MessageBroker_PublishMessage(&_bleMessageBroker.broker, msg);
}

static void RunAppMessageDispatch() {
  MessageBroker_Run(&_appMessageBroker.broker);
}

static void RunBleMessageDispatch() {
  MessageBroker_Run(&_bleMessageBroker.broker);
}

void Message_PublishAppMessage(Message_Message_t* message) {
  MessageBroker_PublishMessage(&_appMessageBroker.broker, message);
}

void ErrorHandler_UnrecoverableError(ErrorHandler_ErrorCode_t code) {
  // An unrecoverable error triggers a state change!
  Message_Message_t message = {
      .header.id = MESSAGE_ID_STATE_CHANGE_ERROR,
      .header.category = MESSAGE_BROKER_CATEGORY_SYSTEM_STATE_CHANGE,
      .parameter2 = code};
  // make sure that the message queue is empty
  CyclicBuffer_Empty(&_appMessageBroker.broker.messageQueue);
  MessageBroker_PublishMessage(&_appMessageBroker.broker, &message);
  // immediately dispatch the message
  MessageBroker_Run(&_appMessageBroker.broker);
  // and stop execution afterwards
  __disable_irq();
  while (1)
    ;
}

void ErrorHandler_RecoverableError(ErrorHandler_ErrorCode_t code) {
  ErrorHandler_RecoverableErrorExtended(code, 0);
}

void ErrorHandler_RecoverableErrorExtended(ErrorHandler_ErrorCode_t code,
                                           uint8_t param) {
  // The recoverable error is sent in another category to allow faster filtering
  // and since it does not trigger a state change
  Message_Message_t msg = {
      .header.id = 1,
      .header.category = MESSAGE_BROKER_CATEGORY_RECOVERABLE_ERROR,
      .header.parameter1 = param,
      .parameter2 = code};
  Message_PublishAppMessage((Message_Message_t*)&msg);
}
