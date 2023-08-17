# SHT43 DemoBoard Firmware

This changelog lists the most important changes for each release version. For
the full log, please refer to the git commit history.

## v0.3.0 (16-08-2023)
**General**

* Use uniform error handling strategy.
* Make usage of ST headers explicit.
* Streamline interrupt priorities.
* Extend the test handler and add the ability to pass parameters to test functions.
* Implement logging macros and possibility to disable UART.
* Implement application states with adaptive readout interval and button event
  handling.
* Optimize current consumption.

This will be the last release developed on gitlab. The repository will be hosted
on github (https://github.com/Sensirion/sht43-demoboard-ble-firmware)

**CI Pipeline**
* Provide `DEBUG` macro in debug build.
* Set `CMAKE_C_STANDARD_INCLUDE_DIRECTORIES`.


## v0.2.0 (26-06-2023)

**General**
* Implement BLE GATT service "Device Information Service".
* Implement BLE GATT service "OTA Firmware Update".
* Implement firmware version compatibility checks during startup.
* Implement voltage monitoring and make firmware features being (de-) activated
depending on remaining battery power.
* Integrate presentation controller being used to update user via screen and
UART of the app states.

**CI Pipeline**
* Add job for English language spelling check.

## v0.1.0 (17-05-2023)

**General**
* Building firmware source code structure based on the STM32CubeWB SDK.
* Implement firmware building environment based on Cmake.
* Initialize inter processor control structure (IPCC) that allows the application to communicate with the RF stack.
* Add MessageBroker architecture to handle timings of sensor, sensor data and the BLE stack.
* Initialized various peripherals which are required for the application:
  * LCD peripheral to show values on the LCD screen.
  * QSPI peripheral for communication with external flash memory.
  * UART peripheral for host communication.
* Peripherals that are only used temporarily can be released in order to reduce power consumption.
* Initialized hardware button with interrupt to trigger actions within the firmware.

**CI Pipeline**
* Setup CI pipeline to check, build and document the source code.
  * clang-format is used for code formatting checks.
  * Cppcheck is used for static code analysis.
  * GCC-ARM is used for building the firmware
  * Doxygen is used for source code documentation.
