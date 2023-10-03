# Compile the STM32CubeWB HAL library
message(STATUS "Processing lib ${HAL_LIB_TARGET}...")

# Definitions
add_definitions(-DCFG_DEBUG_TRACE=1)

set(HAL_SOURCES
    Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_adc.c
    Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_adc_ex.c
    Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_uart.c
    Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_uart_ex.c
    Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_rcc.c
    Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_ll_rcc.c
    Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_hsem.c
    Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_rcc_ex.c
    Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_ipcc.c
    Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_i2c.c
    Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_i2c_ex.c
    Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_lcd.c
    Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_rtc.c
    Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_rtc_ex.c
    Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_gpio.c
    Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_pwr.c
    Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_pwr_ex.c
    Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_cortex.c
    Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_qspi.c
    Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_dma.c
    Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal.c
    Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_flash.c
    Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_flash_ex.c
    Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_crc.c
    Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_crc_ex.c
    Utilities/sequencer/stm32_seq.c
    Utilities/lpm/tiny_lpm/stm32_lpm.c
    Middlewares/ST/STM32_WPAN/utilities/otp.c
    Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/tl/tl_mbox.c
    Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/tl/hci_tl.c
    Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/tl/hci_tl_if.c
    Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/tl/shci_tl.c
    Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/tl/shci_tl_if.c
    Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/shci/shci.c
    Middlewares/ST/STM32_WPAN/ble/core/auto/ble_gap_aci.c
    Middlewares/ST/STM32_WPAN/ble/core/auto/ble_gatt_aci.c
    Middlewares/ST/STM32_WPAN/ble/core/auto/ble_hal_aci.c
    Middlewares/ST/STM32_WPAN/ble/core/auto/ble_hci_le.c
    Middlewares/ST/STM32_WPAN/ble/core/template/osal.c
    Middlewares/ST/STM32_WPAN/ble/svc/Src/svc_ctl.c
    Middlewares/ST/STM32_WPAN/utilities/stm_list.c
    STM32_WPAN/Target/hw_ipcc.c
)
set(HAL_INCLUDES
    shared/include
    Drivers/STM32WBxx_HAL_Driver/Inc
    Drivers/STM32WBxx_HAL_Driver/Inc/Legacy
    Drivers/CMSIS/Device/ST/STM32WBxx/Include
    Drivers/CMSIS/Include
    Utilities/sequencer
    Utilities/lpm/tiny_lpm
    Middlewares/ST/STM32_WPAN
    Middlewares/ST/STM32_WPAN/ble
    Middlewares/ST/STM32_WPAN/ble/core
    Middlewares/ST/STM32_WPAN/ble/core/template
    Middlewares/ST/STM32_WPAN/utilities
    Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread
    Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/tl
    Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/shci
    STM32_WPAN/App
)
add_library(${HAL_LIB_TARGET})
target_sources(${HAL_LIB_TARGET} PRIVATE ${HAL_SOURCES})
target_include_directories(${HAL_LIB_TARGET} PUBLIC ${HAL_INCLUDES})
