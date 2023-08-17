# Software Architecture {#doc_app_architecture}

## Static Decomposition

The software has the layers **utility**, **hal**, **app_service** and **app**. These layers are reflected by the folder structure of the software repository. A detailed description of this structure can be found [here](./application_structure.md)

## Dynamic Decomposition

## Application States

The application behavior can be represented by a state diagram. An overview of this state behavior is shown in the figure below.
![application states](diagrams/app_states.png)

Just after reset the application goes into the **AppBoot** state in which the peripherals are initialized.
After initializing the peripherals, the event **evPeripheralInitialized** triggers the application to enter its main loop.
At first it displays the BleAddress for two seconds. During these two seconds the user may change the display units by pressing the button. After this short time, it enters the state AppNormalOperation. In this state many things happen in parallel:

- *NormalOperation*: Regular readout of sensor data and update the display with the new values.
- *Ble Radio*: If not disabled, the sensor values are published in a proprietary advertizement data package. The rate of publishing data is reduced after some time.
  When the battery degrades, the device cannot be connected anymore.

- *PowerCheck States*: The system is always in one of the power states:
  - NO_RESTRICTION: The battery has enough capacity to support all use cases.
  - REDUCED_OP: The battery capacity does not allow connecting the device.
  - CRITICAL: The remaining battery capacity is critical and thus does not allow any BLE or flash operation.

## Interrupts

In order to allow the Software to work in parallel to the hardware many hardware blocks are operated in interrupt mode or using a DMA channel. In any case this requires that some interrupts are enabled and one interrupt triggered function does not depend on another function that requires an interrupt with lower priority.
This section lists the used interrupts, what they are used for and their priority as they are defined.

|Peripheral|Description| Interrupt prio|
|:---| :---|:---|
|SysTic_IRQ| Priority of the system tick: the system tick is used in timing functions.| TICK_INT_PRIORITY=4|
|RTC_WKUP_IRQ| Wake up interrupt of real time clock; this is the clock that is running during stop mode to wake up the system in regular time intervals| IRQ_PRIO_RTC_WAKE_UP = 3|
|LCD_IRQ| Irq used by LCD screen; the IRQ is enabled but no software function depends on it.| 0|
|ADC1_IRQ| Irq used by ADC1; signals the end of a battery voltage measurement.| IRQ_PRIO_APP=5 |
|EXTI15_10_IRQ| Interrupt of the external interrupt trigger lines 10-15. The user button is attached to this interrupt| IRQ_PRIO_APP=5 |
|I2C3_EV_IRQ|Event to synchronize I2C with DMA| IRQ_PRIO_SYSTEM=0 |
|DMA1_Channel2_IRQ; DMA1_Channel3_IRQ|DMA channels used for I2c transmit and receive| IRQ_PRIO_SYSTEM=0|
|DMA1_Channel1_IRQ|DMA used by Quad-SPI. In the normal application Quad-SPI will not be in use at all.| IRQ_PRIO_SYSTEM=0|
|QUADSPI_IRQ|In the normal application Quad-SPI will not be in use at all.| 0|
|USART1_IRQ| The receive interrupt of the USART is used to receive test commands.| IRQ_PRIO_APP=5 |
|DMA1_Channel4_IRQ| The Uart module exposes a function to write asynchronously using the DMA1-Channel4| 0|
|HSEM_IRQ| Interrupt generated from hardware semaphores; used in synchronization between Cortex M0 and Cortex M4| IRQ_PRIO_SYSTEM=0|
|IPCC_C1_RX_IRQ;IPCC_C1_TX_IRQ| Used in synchronization between Cortex M0 and Cortex M4| IRQ_PRIO_SYSTEM=0 |

The system tick is used in various HAL timing functions such as HAL_GetTicks(). With these IRQ settings it should become
possible to use synchronous debug traces in interrupt routines of interrupt handlers with priority IRQ_PRIO_APP.
(In release build there should not be any debug traces!)

## Testing

The firmware is capable to receive input from the host and trigger predefined operations.
During test, the **NormalOperation** state can be forced into a TestState where it
does not update the display anymore.
For now this only works if a debugger is attached as USART is not able to wake-up the device from lower power mode stop2.
