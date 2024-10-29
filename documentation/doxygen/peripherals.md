# Microcontroller Peripherals {#doc_peripherals}

[TOC]

## General

| Name              | Value                            |
|:------------------|:---------------------------------|
| `V_CORE`          | 1.2 V                            |

All the below settings are configured using the STM32CubeMX. The corresponding .ioc file is checked in as a reference.

## Clocks


| Clock      | Configuration                  | Usage  |
|:-----------|:-------------------------------|:-------|
| `LSI1 RC`  | Disabled => 32 kHz             | Not used |
| `LSI2 RC`  | Disabled => 32 kHz             | Not used |
| `LSE`      | Enabled => 32.744 kHz          | Clock source for RTC, system wake-up |
| `HSE`      | Enabled => 32 MHz              | System clock when system is using BLE|
| `HSI RC`   | Enabled => 16 MHz              | System clock for wake-up after stop mode|
| `MSI RC`   | Enabled after POR => 32 MHz    | System clock after POR;<br>When the application is initialized, the MSI is switched off.|
| `RC`       | Enabled => 48 MHz              | Radio clock |
| `HCLK1`    | `SYSCLK /2` => 16 MHz          | Clock for CPU1 (ARM4)<br> Using 16 MHz allows to switch to a lower voltage in the voltage regulator.<br>Further reducing the clock speed increases the time the application is awake and therefore increases power consumption. |
| `HCLK2`    | `SYSCLK /1` => 32 MHz          | Clock for CPU2 (ARM M0) |

The system boots with MSI switched on. In a very early stage of the boot process, the clocks and voltage regulators are configured. The system is configured to
run on regulated 1 V supply. This implies that the SystemClock must not be above 16 MHz.
The step down converter is enabled in order to improve power efficiency.

## USART1

| Name                 | Value                                  |
|:---------------------|:---------------------------------------|
| Clock Source         | `HSI` => 16 MHz                        |
| Baudrate             | 19200 bps                              |
| Data bits            | 8                                      |
| Stop Bits            | 1                                      |
| Parity               | None                                   |
| Hardware flow control| None                                   |

The USART is used for tracing application and debug information. The software supports synchronous (blocking) and asynchronous write requests.
Furthermore it is possible to send data from PC to the application in order execute selected tests.

This is also the reason, why the baudrate is reduced to 19200 bps. With higher speeds it is not possible to receive data reliably anymore.


## I2C3

| Name              | Value                                   |
|:------------------|:----------------------------------------|
| Clock Source      | `HSI` => 16 MHz                       |
| Timing value      |  0x0010061A                             |
| Target frequency  |  400 kHz                                |

The demo board communicates with the sensor using I2C. The timing value is computed by STM32CubeMX using the configured clock configuration.
When changing the clock configuration in the code, this value needs to be adapted.

In order to minimize the power up time of the application, all transfers
(rx and tx) are done asynchronously using the DMA. Care must be taken, that
the application does not enter stop-mode 2 while waiting on data from the sensor.


## RTC
| Name              | Value                                    |
|:------------------|:-----------------------------------------|
| Clock Source      | `LSE` => 32.774 kHz                      |
| Prescaler         | DIV 1 => 32.774 kHz                      |

*Notes:*

- *Timings are measured with release configuration.*
