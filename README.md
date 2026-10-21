# Middleware for ATWINC1500 Integration on STM32F401xE Platforms

This repository contains the code for a middleware allowing STM32F401xE MCUs to
interface with the Microchip ATWINC1500 module over SPI, through the
[official driver](https://github.com/MicrochipTech/WINC-Releases) (version 
19.7.11). The official driver is written in such a way that it makes no 
assumption on the host device's platform (baremetal MCU, RTOS, Linux), but that
puts the responsibility on the user to provide the abstraction layer.

This middleware answers to the above requirement, providing a highly portable
abstraction layer, with as the main feature freedom in the five following axes:
- hardware peripheral mappings
- hardware peripheral configurations
- memory bus access method employed by hardware peripherals (i.e., CPU-managed
  access vs bypass via DMA)
- runtime environment choice (i.e., baremetal vs Real-Time Operating System
  (RTOS)) &mdash; where thread-safety is a relevant concern
- management of hardware and software resources shared by the driver and 
  application contexts

## Contents

- The middleware files: `bsp/include/nm_bsp_stm32f401xe.h`,
  `bsp/source/nm_bsp_stm32f401xe.c`,  `bus_wrapper/source/nm_bus_stm32f401xe.c`,
  and `core/inc/conf_winc.h` as an example; change the values of the macros
  according to your needs.
- Vendor driver: everything under `winc1500/` that is not a middleware file
- Example application: `core/` (note: currently not working)
- Unit tests: `tests/`, host-executed using Unity + CMock
- Documentation: `docs/`, contains ASPICE SWE process group artefacts; SWE.1
  through SWE.4

## Building

Prerequisites: `arm-none-eabi-gcc`, CMake and Ninja.

```
$ cmake --preset Debug
$ cmake --build build/Debug
```

Depending on your board, you can then adapt the OpenOCD script
`nucleo-f401re-jlink.cfg` to flash to it.

## Testing

The test suite is built and executed on the host; no target hardware is
required.

```
$ cmake -S . -B build/tests -DBUILD_TESTING=ON -G Ninja
$ cmake --build build/tests
$ ctest --test-dir build/tests --output-on-failure
```

The suite builds the real BSP and bus wrapper against a mocked STM32 HAL and
checks the middleware's requirements: 17 tests in two targets. CMock's
generator is a Ruby script, so Ruby is required.

## Documentation

- [`docs/SRS.md`](docs/SRS.md) — Software Requirements Specification
  (ISO/IEC/IEEE 29148:2018 compliant)
- [`docs/SAD.md`](docs/SAD.md) — Software Architecture Description
  (ISO/IEC/IEEE 42010:2022 compliant)
- [`docs/SDD.md`](docs/SDD.md) — Software Design Description (IEEE Std 1016:2009
  compliant)
- [`docs/test-coverage.md`](docs/test-coverage.md) — Unit test coverage summary

## Licence

BSD Zero Clause License (0BSD); see `LICENSE`.
