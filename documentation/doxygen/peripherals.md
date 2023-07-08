# Microcontroller Peripherals {#doc_peripherals}

[TOC]

## General

| Name              | Value                            |
|:-----------------:|:--------------------------------:|
| `V_CORE`          | 1.2V                             |

## Clocks

| Clock      | Configuration                 |
|:----------:|:-----------------------------:|
| `LSI1 RC`  | Enabled => 32 KHz             |
| `LSI2 RC`  | Enabled => 32 KHz             |
| `LSE`      | Enabled => 32.744KHz          |
| `HSE`      | Enabled => 32MHz              |
| `HSI RC`   | Enabled => 16MHz              |
| `MSI RC`   | Enabled => 32MHz              |
| `RC`       | Enabled => 48MHz              |
| `PLLIN`    | `MSI / 1` => 32MHz            |
| `fVCO`     | Disabled                      |
| `PLLQCLK`  | `PLLIN x 8` => 128MHz         |
| `PLLPCLK`  | `PLLIN x 8` => 128MHz         |
| `SYSCLK`   | `MSI` => 32MHz                |
| `HCLK1`    | `SYSCLK /1` => 32MHz          |
| `HCLK2`    | `SYSCLK /1` => 32MHz          |
| `PCLK1`    | `HCLK1 /1` => 32MHz           |
| `PCLK1`    | `HCLK /1` => 32MHz            |

## USART1

| Name              | Value                                    |
|:-----------------:|:----------------------------------------:|
| Clock Source      | `PCLK2` => 4MHz                          |
| Prescaler         | DIV 1 => 4MHz                            |
| Oversampling      | 16x (reset value, not set explicitly)    |
| BRR = USARTDIV    |  35 => 114'285.714 bit/s => -0.8% error  |

## RTC
| Name              | Value                                    |
|:-----------------:|:----------------------------------------:|
| Clock Source      | `LSE` => 32.774KHz                       |
| Prescaler         | DIV 1 => 32.774KHz                       |

*Notes:*

- *Timings are measured with release configuration.*
