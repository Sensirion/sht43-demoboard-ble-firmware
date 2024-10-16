# Changelog

This changelog lists the most important changes for each release version. For
the full log, please refer to the git commit history.

## Unreleased

### Fixed

* Default logging interval set to 10'
* Zero mailbox memory
* Enable debugger in sleep mode for debug builds

### Changed

* Update STM Framework to version 1.20

## v0.6.0 (2024-06-04)

### Fixed

* Stabilize data download
* Cleanup logging
* Improve screen flickering

### Changed

* Default name of DemoBoard
* Log BLE stack version to UART in release mode
* I2C frequency changed to ~400KHz
* Lower POR level to ~1.7V

### Added

* Pairing screen and secure pairing
* Support for debug pins PA12 and PB11
* Double click for change of temperature units

## v0.5.3 (2024-02-28)

### Fixed

* Update advertisement data
* Stop pending data download

### Changed

* Add warning in readme for wired connection.
* Introduce filter for battery level measurements.
* Security requirements for GATT services.

### Removed

* Function getBatteryVoltage() not used and removed.

## v0.5.2 (2023-12-21)

### Fixed

* Get rid of clang-tidy warnings
* Fix compile time configuration for sequencer priorities

### Changed

* Rework readme and documentation intro page
* Use again compiler optimization -O2 in release build

### Added

* Checklist for release and pull requests

## v0.5.1 (2023-11-16)

### Fixed

* Fix bug that prevented release build from booting.
* Fix wrong initialization of itemsToSkip when no items are in an item store
  and we want to enumerate in reversed order.

## v0.5.0 (2023-11-15)

### Added

* Toggle display between RH and dew point

### Fixed

* Fix bug that leads to hard-fault when trying to enumerate the items in the
  item store immediately after all items where deleted.

### Changed

* Delete measurement samples when power is removed in order to keep time series
  consistent.
* Immediate update of screen after button press.
* Format changelog according to [`Keep a Changelog`](https://keepachangelog.com/en/1.0.0/)

## v0.4.0 (05-10-2023)
**General**

* Repository was moved to github.
* Implement of persistent item store on flash.
* Compute CRC with Hardware for sensor data and device settings.
* Implement of BLE_GATT services:
  * DataLogging Service.
  * Battery Service.
  * DeviceSettings service:
    * Enable/Disable UART tracing over BLE.
    * Enable/Disable measurement sample advertisement over BLE.
    * Save an alternative device name.
  * Humidity Service.
  * Temperature Service.
  * SHT SerialNumber Service.

**CI Pipeline**

* Docker image available to execute each build step
* Build pipeline ported to github actions
* clang-tidy checks added to pipeline

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
