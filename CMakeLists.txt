cmake_minimum_required(VERSION 3.16)
include(version.cmake)

#search installation of toolchain using the path variable
set(TOOLCHAIN arm-none-eabi)
find_path(TOOLCHAIN_PATH ${TOOLCHAIN})
message("Toolchain " ${TOOLCHAIN} " installed in " ${TOOLCHAIN_PATH})
set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES ${TOOLCHAIN_PATH}/${TOOLCHAIN}/include)
message("Include directory of standard headers = " ${CMAKE_C_STANDARD_INCLUDE_DIRECTORIES})

# Toolchain configuration
set(CMAKE_SYSTEM_NAME       Generic)
set(CMAKE_SYSTEM_PROCESSOR  arm)
set(TOOLCHAIN_PREFIX        ${TOOLCHAIN}-)
set(CPP_FLAGS               -fno-rtti -fno-exceptions -fno-threadsafe-statics)
set(ASM_FLAGS               -x assembler-with-cpp)
set(CMAKE_C_COMPILER        ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER      ${TOOLCHAIN_PREFIX}g++ ${CPP_FLAGS})
set(CMAKE_ASM_COMPILER      ${CMAKE_C_COMPILER} ${ASM_FLAGS})
set(CMAKE_OBJCOPY           ${TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_SIZE              ${TOOLCHAIN_PREFIX}size)
set(CMAKE_EXECUTABLE_SUFFIX_ASM     .elf)
set(CMAKE_EXECUTABLE_SUFFIX_C       .elf)
set(CMAKE_EXECUTABLE_SUFFIX_CXX     .elf)
set(CMAKE_TRY_COMPILE_TARGET_TYPE   STATIC_LIBRARY)


# Project settings
set(PROJECT_NAME SHT43_DB)
set(PROJECT_TARGET          ${PROJECT_NAME}-${APPLICATION_FW_VERSION_MAJOR}_${APPLICATION_FW_VERSION_MINOR}_${APPLICATION_FW_VERSION_PATCH}${APPLICATION_FW_SUFFIX})
set(CMAKE_CXX_STANDARD      17)
set(CMAKE_C_STANDARD        11)
set(CPU_FLAGS
    -mthumb
    -mcpu=cortex-m4
    -mfloat-abi=hard        # Use hardware FPU
    -mfpu=fpv4-sp-d16       # Type of the FPU for the STM32WB
)
set(FLASH_LD                stm32wb5mxx_flash_cm4.ld)
set(LINKER_SCRIPT           ${CMAKE_SOURCE_DIR}/${FLASH_LD})
project(${PROJECT_TARGET}
      LANGUAGES C CXX ASM )

# Global Definitions
add_definitions(-DUSE_HAL_DRIVER)
add_definitions(-DSTM32WB5Mxx)
add_compile_definitions($<$<CONFIG:Debug>:DEBUG>)

# Compiler Options
add_compile_options(
    ${CPU_FLAGS}
    -fdata-sections        # Generate info for removing dead code during linking
    -ffunction-sections    # Generate info for removing dead code during linking
    -fstack-usage          # Generate info about maximal stack usage
    -Wall                  # Turn on many warnings
    -Wextra                # Turn on extra warnings
    -Wno-unused-parameter  # Disable, because HAL contains unused parameters
    -Wshadow
    -Wdouble-promotion
    -Werror                # Promote all warnings to errors
)

# Linker Options
add_link_options(
    ${CPU_FLAGS}
    -specs=nano.specs      # Use newlib-nano
    -T${LINKER_SCRIPT}
    -lc -lm -lnosys        # Libraries to link
    -Wl,--print-memory-usage
    -Wl,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map,--cref
    -Wl,--gc-sections      # Remove unused code during linking
)

# Build type
if ("${CMAKE_BUILD_TYPE}" STREQUAL "release")
    message(STATUS "Optimization for release build")
    add_compile_options(-O2)
else ()
    message(STATUS "No optimization for debug build")
    add_compile_options(-O0 -g)

endif ()

# Build libraries
set(HAL_LIB_TARGET          stm32wb-hal)
add_subdirectory(lib)

# Application include directories
set(APP_INCLUDE_DIRS
    source
    st_app/includes
)

# Application sources
set(APP_SOURCES
    # unmodified code from st
    st_app/startup_stm32wb5mxx_cm4.s
    st_app/system_stm32wbxx.c
    st_app/sysmem.c
    # written explicitly for this application
    source/Main.c
    source/app/System.c
    source/app/SysTest.c
    source/app/Presentation.c
    source/app/test/QspiTest.c
    source/app/test/CyclicBufferTest.c
    source/app/test/ListTest.c
    source/app/test/TraceTest.c
    source/app/test/ScreenTest.c
    source/app/test/AdcTest.c
    source/app/test/PresentationTest.c
    source/app/test/FlashTest.c
    source/app/BleContext.c
    source/app_service/networking/HciTransport.c
    source/app_service/networking/ble/BleInterface.c
    source/app_service/networking/ble/BleOverrides.c
    source/app_service/networking/ble/BleGap.c
    source/app_service/networking/ble/BleGatt.c
    source/app_service/networking/ble/BleHelper.c
    source/app_service/networking/ble/gatt_service/DeviceInfo.c
    source/app_service/networking/ble/gatt_service/Reboot.c
    source/app_service/networking/ble/gatt_service/ShtService.c
    source/app_service/networking/ble/gatt_service/TemperatureService.c
    source/app_service/networking/ble/gatt_service/HumidityService.c
    source/app_service/networking/ble/gatt_service/BatteryService.c
    source/app_service/power_manager/PowerManager.c
    source/app_service/power_manager/LpmHooks.c
    source/app_service/power_manager/SchedulerOverride.c
    source/app_service/power_manager/BatteryMonitor.c
    source/app_service/sensor/Sht4x.c
    source/app_service/screen/Screen.c
    source/app_service/sensor/SensorController.c
    source/app_service/nvm/ProductionParameters.c
    source/app_service/timer_server/TimerServer.c
    source/app_service/timer_server/TimerServerHelper.c
    source/app_service/timer_server/TimerServerRtcInterface.c
    source/app_service/user_button/Button.c
    source/app_service/user_button/ButtonEvent.c
    source/hal/Adc.c
    source/hal/Clock.c
    source/hal/Flash.c
    source/hal/Gpio.c
    source/hal/Ipcc.c
    source/hal/Rtc.c
    source/hal/I2c3.c
    source/hal/InterruptHandler.c
    source/hal/Overrides.c
    source/hal/Qspi.c
    source/hal/Uart.c
    source/utility/log/Trace.c
    source/utility/concurrency/Concurrency.c
    source/utility/collection/CyclicBuffer.c
    source/utility/collection/LinkedList.c
    source/utility/scheduler/MessageBroker.c
)


# Specify application executable
add_executable(${PROJECT_TARGET} ${APP_SOURCES} ${LINKER_SCRIPT})
target_link_libraries(${PROJECT_TARGET} PRIVATE ${HAL_LIB_TARGET})
target_include_directories(${PROJECT_TARGET}
    PRIVATE ${APP_INCLUDE_DIRS} ${HAL_INCLUDES})

# Build HEX file
set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_TARGET}.hex)
add_custom_command(TARGET  ${PROJECT_TARGET} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_TARGET}> ${HEX_FILE}
)
# Build BIN file
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_TARGET}.bin)
add_custom_command(TARGET  ${PROJECT_TARGET} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_TARGET}> ${BIN_FILE}
)
