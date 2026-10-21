# Software Requirements Specification: STM32F401xE-WINC1500 Middleware
Version 1.0.

## Table of Contents
* [1. Introduction](#1-introduction)
    * [1.1 Purpose](#11-purpose)
    * [1.2 Scope](#12-scope)
    * [1.3 Component Overview](#13-component-overview)
        * [1.3.1 Component Perspective](#131-component-perspective)
            * [1.3.1.1 User Interface](#1311-user-interface)
        * [1.3.2 Component Functions](#132-component-functions)
    * [1.4 Definitions](#14-definitions)
* [2. References](#2-references)
* [3. Requirements](#3-requirements)
    * [3.1 Functions](#31-functions)
        * [3.1.1 BSP Functions](#311-bsp-functions)
        * [3.1.2 Bus wrapper functions](#312-bus-wrapper-functions)
        * [3.1.3 Synchronisation and bus ownership](#313-synchronisation-and-bus-ownership)
    * [3.2 Performance](#32-performance)
    * [3.3 Usability](#33-usability)
    * [3.4 Interface](#34-interface)
        * [3.4.1 Hardware Interface](#341-hardware-interface)
        * [3.4.2 Porting Interface](#342-porting-interface)
    * [3.5 Design Constraints](#35-design-constraints)
    * [3.6 Software System Attributes](#36-software-system-attributes)
        * [3.6.1 Reliability](#361-reliability)
        * [3.6.2 Maintainability](#362-maintainability)
* [4. Verification](#4-verification)
    * [4.1 Functions](#41-functions)
    * [4.2 Performance](#42-performance)
    * [4.3 Usability](#43-usability)
    * [4.4 Interface](#44-interface)
    * [4.5 Design Constraints](#45-design-constraints)
    * [4.6 Software System Attributes](#46-software-system-attributes)
* [5. Appendices](#5-appendices)
    * [5.1 Assumptions and dependencies](#51-assumptions-and-dependencies)

## 1. Introduction

### 1.1 Purpose

This middleware serves the purpose of allowing STM32F401xE firmware to interface
with the Microchip ATWINC1500 Wi-Fi module official driver, to enable Wi-Fi
connectivity via an SPI connection.

### 1.2 Scope

The middleware covers the implementations of the two Hardware Abstraction Layer
interfaces expected by the vendor driver: the Board Support Package (BSP)
(`winc1500/bsp/include/nm_bsp.h`) and bus wrapper
(`winc1500/bus_wrapper/include/nm_bus_wrapper.h`) interfaces. In particular,
only the part of the latter that's relevant for SPI-based communication is
covered.

Additionally, it defines a porting interface for the user to implement. The
purpose of this interface is to allow the firmware flexibility in terms of
peripheral configuration, bus availability and runtime environment (namely,
baremetal or RTOS).

### 1.3 Component Overview
#### 1.3.1 Component Perspective

The middleware lies at the second-most bottom layer of a four-stratum stack:
1) the application firmware calls
2) the vendor driver to execute networking functions, which in turn invokes
3) the middleware to convert them into platform-specific read-write commands,
   which in turn invokes
4) the STM32 HAL and the porting implementation to perform low-level SPI, DMA
   and GPIO operations

![Inter-Component Interfaces](diagrams/middleware-component-perspective-2.png)

##### 1.3.1.1 User Interface

The user of the middleware is the STM32F401xE firmware developer. The user
interface is the porting interface part of the middleware. In their
implementation, the user is expected to define:
- GPIO connections mappings
- SPI peripheral mappings
- optionally, DMA peripheral mappings and configuration functions
- acquire/release semantics for SPI bus access
- a notification-based synchronisation mechanism for SPI operations
- EXTI, SPI and DMA ISR callback registration/de-registration mechanisms

#### 1.3.2 Component Functions

The middleware performs four groups of functions.

- As an implementation of the BSP interface, it performs **module power
  control**: power on, power off and reset sequences executed in a manner
  compliant with datasheet specifications.
- As an implementation of the BSP interface, it is charged with **managing 
  module-to-host interrupt**: the module exposes a specialised pin to transmit
  interrupt requests to the board &mdash; the middleware configures said pin
  appropriately.
- As an implementation of the bus wrapper interface, it **handles WINC-relevant
  SPI transport**: peripheral configuration, thread-safe full-duplex and
  half-duplex communication, optionally leveraged by a DMA peripheral.
 
**Note, on peripheral ownership**: the bus wrapper makes use of, but does not
own, either of the SPI or &mdash; optional &mdash; DMA peripherals. All the
more, it does not own the SPI bus. The application firmware does. Hence, all SPI
bus and SPI and DMA peripheral access is done in a manner that is thread-safe
*and* agnostic to whether they are a shared resource.

### 1.4 Definitions

| Term                                  | Meaning                                                                                                        |
|---------------------------------------|----------------------------------------------------------------------------------------------------------------|
| Driver layer                          | The vendor source under `winc1500/`.                                                                           |
| Board and transport abstraction layer | The complex of the BSP and the bus wrapper implementations. Also called board &amp; transport layer for short. |
| Porting API                           | The porting interface.                                                                                         |
| Application layer                     | The application firmware.                                                                                      |
| User application                      | The application layer plus the porting implementation.                                                         |
| CHIP_EN                               | The module chip-enable input, active high.                                                                     |
| RESET_N                               | The module reset input, active low.                                                                            |
| IRQN                                  | The module interrupt-request output, active low.                                                               |
| SS                                    | Slave Select. The module chip-select input for the SPI interface, active low.                                  |
| SPI event                             | A transmit-complete, receive-complete, transmit-receive-complete or error SPI interrupt.                       |
| EXTI                                  | The STM32 external interrupt/event controller.                                                                 |

When the phrase "user-specified" or "user-defined" is used, it means that the
object of interest is exposed to the board &amp; transport layer via the porting
interface, and defined by the user in their implementation.

## 2. References

- Microchip ATWINC1500 module datasheet (DS70005304F, 2025).
- ATWINC1500 19.7.11 Software API reference manual.
- ST STM32F401xE reference manual (RM0368, Rev 6, January 2025).

## 3. Requirements

This section lists the various requirements that an implementation of this
middleware shall meet. Each requirement is a single, verifiable capability and
is uniquely identified by a `REQ-<section>` number that is never reused.

### 3.1 Functions

This subsection lists the functional requirements that the middleware shall
meet.

#### 3.1.1 BSP Functions

The BSP implements the `nm_bsp_*` API declared in
`winc1500/bsp/include/nm_bsp.h`. It is responsible for module power control,
module-to-host interrupt management and putting the driver to sleep.

|     ID     | Requirement                                                                                                                                                   | Rationale                                                                                                                                                                    |
|:----------:|---------------------------------------------------------------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| REQ-FUN-01 | On invocation, `nm_bsp_init` shall hold the CHIP_EN and RESET_N signals low when it returns.                                                                  | The module is powered off by default until a call is made to explicitly start it. CHIP_EN is active-high and RESET_N is active-low (ATWINC1500 datasheet, sections 7.4-7.6). |
| REQ-FUN-02 | `nm_bsp_init` shall configure the IRQN signal to generate an interrupt on the falling edge, with the internal pull-up resistor enabled.                       | IRQN is active-low; the pull-up forces the line high while the module is powered off.                                                                                        |
| REQ-FUN-03 | `nm_bsp_init` shall invoke the porting API to ensure that no driver ISR callback is registered for the IRQN signal.                                           | If the function is invoked as part of a re-initialisation procedure, no leftover configuration from past state should remain.                                                |
| REQ-FUN-04 | `nm_bsp_init` shall return `M2M_SUCCESS` on success, and a negative integer on failure to fulfill any of the above requirements.                              | The vendor contract requires it.                                                                                                                                             |
| REQ-FUN-05 | On invocation, `nm_bsp_deinit` shall deinitialise the CHIP_EN, RESET_N and IRQN control pins.                                                                 | Every successful `nm_bsp_init` is matched by a `nm_bsp_deinit` per the vendor contract.                                                                                      |
| REQ-FUN-06 | `nm_bsp_deinit` shall return `M2M_SUCCESS` on success, and a negative integer on failure to fulfill the above requirement.                                    | The vendor contract requires it.                                                                                                                                             |
| REQ-FUN-07 | On invocation, `nm_bsp_reset` shall begin by holding the CHIP_EN and RESET_N signals low for at least 2 &mu;s.                                                | The datasheet specifies a minimum RESET_N pulse of 1 &mu;s. We double that value as a safety margin for timing correctness.                                                  |
| REQ-FUN-08 | `nm_bsp_reset` shall follow by setting the CHIP_EN signal high and shall hold it high for at least 10 ms before setting the RESET_N signal high.              | The datasheet specifies a minimum of 5 ms between the CHIP_EN and RESET_N rises. We double that value for safety.                                                            |
| REQ-FUN-09 | On invocation, `nm_bsp_sleep` shall block the invoking context for the specified number of milliseconds.                                                      | &mdash;                                                                                                                                                                      |
| REQ-FUN-10 | On invocation, `nm_bsp_register_isr` shall invoke the porting API to register the specified callback as ISR for the IRQN signal.                              | &mdash;                                                                                                                                                                      |
| REQ-FUN-11 | On invocation, `nm_bsp_interrupt_ctrl` shall enable the IRQN interrupt when invoked with an input of 1, and shall disable it when invoked with an input of 0. | &mdash;                                                                                                                                                                      |

#### 3.1.2 Bus wrapper functions

The bus wrapper implements the `nm_bus_*` and `nm_spi_rw` interfaces declared
in `winc1500/bus_wrapper/include/nm_bus_wrapper.h`, restricted to SPI transport.

For the purposes of this subsection:

- The *module-required SPI configuration* and the *module-required DMA
  parameters* are defined in section 3.4.
- An *SPI event* is defined in section 1.4.

|     ID     | Requirement                                                                                                                                                                                   | Rationale                                                                                                                                                                                                            |
|:----------:|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| REQ-FUN-12 | On invocation, `nm_bus_init` shall configure the SPI interface to the module-required SPI configuration, replacing any configuration in effect at entry.                                      | The SPI configuration in its entirety is crucial to establishing communication with the module. Ensuring its compliance with the module requirements becomes the middleware's responsibility rather than the user's. |
| REQ-FUN-13 | When the DMA option is enabled, `nm_bus_init` shall invoke the porting API to initialise SPI TX and RX DMA streams, if needed.                                                                | Since DMA configuration is largely inconsequential to the driver functionality, more liberty is given to the user than with the SPI configuration.                                                                   |
| REQ-FUN-14 | When the DMA option is enabled, `nm_bus_init` shall ensure that each DMA stream holds the module-required DMA parameters, and shall reconfigure any stream that does not.                     | A minimal set of DMA parameters remains module-critical, so the wrapper validates and self-heals them.                                                                                                               |
| REQ-FUN-15 | When the DMA option is enabled, `nm_bus_init` shall link SPI interface and DMA controller, and shall enable DMA interrupts in the NVIC.                                                       | &mdash;                                                                                                                                                                                                              |
| REQ-FUN-16 | `nm_bus_init` shall invoke the porting API to register a handler for each SPI event, and shall enable SPI interrupts in the NVIC.                                                             | Interrupt-driven transfers enable power-efficient synchronisation mechanisms, compared to timeout-based transfers which force polling.                                                                               |
| REQ-FUN-17 | `nm_bus_init` shall leave the SS signal deasserted on completion.                                                                                                                             | The module is disabled by default until a call is made to explicitly enable it.                                                                                                                                      |
| REQ-FUN-18 | `nm_bus_init` shall invoke the BSP API to perform a module reset.                                                                                                                             | The module is powered on only once the interface is ready.                                                                                                                                                           |
| REQ-FUN-19 | `nm_bus_init` shall return `M2M_SUCCESS` on success, and a negative integer on failure to fulfill any of the above requirements.                                                              | The vendor contract requires it.                                                                                                                                                                                     |
| REQ-FUN-20 | On invocation, `nm_bus_deinit` shall deinitialise the SS pin and deregister any SPI event handlers.                                                                                           | &mdash;                                                                                                                                                                                                              |
| REQ-FUN-21 | `nm_bus_deinit` shall not affect the SPI and DMA interfaces in any respect.                                                                                                                   | Ownership of the SPI and DMA peripherals stays with the user.                                                                                                                                                        |
| REQ-FUN-22 | `nm_bus_deinit` shall return `M2M_SUCCESS` on success, and a negative integer on failure to fulfill any of the above requirements.                                                            | The vendor contract requires it.                                                                                                                                                                                     |
| REQ-FUN-23 | On invocation, `nm_bus_ioctl` shall use its first parameter to select the bus I/O function to which it redirects.                                                                             | The first parameter acts as a dispatch key selecting among the bus operations exposed by the interface (`nm_bus_wrapper.h`).                                                                                         |
| REQ-FUN-24 | `nm_bus_ioctl` shall return `M2M_ERR_INVALID_ARG` when called with a first parameter that has no corresponding bus I/O function.                                                              | The SPI read/write command is the only operation the driver issues in SPI mode; unrecognised commands are rejected per the vendor contract.                                                                          |
| REQ-FUN-25 | `nm_bus_ioctl` shall repurpose its second parameter to pass to the selected bus I/O function as that function's parameters.                                                                   | The second parameter is forwarded so each bus operation receives its own argument structure.                                                                                                                         |
| REQ-FUN-26 | `nm_bus_ioctl` shall return the return value of the selected bus I/O function.                                                                                                                | The vendor contract requires propagating the result of the performed operation.                                                                                                                                      |
| REQ-FUN-27 | On invocation, `nm_spi_rw` shall return `M2M_ERR_BUS_FAIL` when invoked with a transfer size of zero.                                                                                         | The vendor contract requires it.                                                                                                                                                                                     |
| REQ-FUN-28 | `nm_spi_rw` shall return `M2M_ERR_BUS_FAIL` when invoked with both the transmit and receive buffers NULL.                                                                                     | The vendor contract requires it.                                                                                                                                                                                     |
| REQ-FUN-29 | When invoked with a transmit buffer and a NULL receive buffer, `nm_spi_rw` shall transmit the specified number of bytes and shall return `M2M_SUCCESS` on completion.                         | Half-duplex transmit path.                                                                                                                                                                                           |
| REQ-FUN-30 | When invoked with a receive buffer and a NULL transmit buffer, `nm_spi_rw` shall receive the specified number of bytes and shall return `M2M_SUCCESS` on completion.                          | Half-duplex receive path.                                                                                                                                                                                            |
| REQ-FUN-31 | When invoked with both non-NULL transmit and receive buffers, `nm_spi_rw` shall transmit and receive the specified number of bytes concurrently and shall return `M2M_SUCCESS` on completion. | Full-duplex transmit and receive path.                                                                                                                                                                               |
| REQ-FUN-32 | `nm_spi_rw` shall assert the SS signal before starting each SPI transfer and shall deassert it after the transfer completes.                                                                  | &mdash;                                                                                                                                                                                                              |
| REQ-FUN-33 | `nm_spi_rw` shall return `M2M_ERR_BUS_FAIL` when a transfer fails.                                                                                                                            | The vendor contract requires the wrapper to report transfer failure for its own recovery logic.                                                                                                                      |

No requirement is made on `nm_bus_reinit`, as it is unused in the current driver
version.

#### 3.1.3 Synchronisation and bus ownership

The middleware does not own the SPI bus, the SPI peripheral or the DMA
peripheral; these may be shared with the user application. It therefore relies
on user-specified mechanisms, exposed through the porting interface, for bus
exclusivity and operation synchronisation.

|     ID     | Requirement                                                                                                                                     | Rationale                                                                                                                      |
|:----------:|-------------------------------------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------|
| REQ-FUN-34 | `nm_spi_rw` shall invoke the porting API to acquire the SPI bus before asserting the SS signal.                                                 | The SPI bus may be shared, so the wrapper claims it before any transfer.                                                       |
| REQ-FUN-35 | `nm_spi_rw` shall invoke the porting API to release the SPI bus after deasserting the SS signal.                                                | The bus is freed to allow for any pending accesses to proceed.                                                                 |
| REQ-FUN-36 | `nm_spi_rw` shall invoke the porting API to prepare the synchronisation mechanism, before starting each SPI transfer.                           | &mdash;                                                                                                                        |
| REQ-FUN-37 | `nm_spi_rw` shall invoke the porting API to block middleware execution until transfer completion or failure, before it deasserts the SS signal. | The vendor contract requires the wrapper to report transfer success and failure, hence it must block until either takes place. |
| REQ-FUN-38 | On each SPI event, the handler shall invoke the porting API to signal transfer completion or failure.                                           | Prevents `nm_spi_rw` from deadlocking.                                                                                         |

### 3.2 Performance

This subsection specifies the measurable performance characteristics that the
middleware shall meet.

|     ID      | Requirement                                                                                                | Rationale                                                                  |
|:-----------:|------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------|
| REQ-PERF-01 | The SPI clock frequency shall not exceed 48 MHz.                                                           | The datasheet specifies this as the maximum supported SPI clock frequency. |
| REQ-PERF-02 | The middleware shall define a maximum SPI transfer size of at least 16 bytes via `egstrNmBusCapabilities`. | The vendor contract requires it (`nm_bus_wrapper.h`).                      |

### 3.3 Usability

This subsection specifies the requirements that ease the adaptation of the
middleware to a target application by the user (defined
[earlier](#1311-user-interface)). The target platform is the STM32F401xE family
of microcontrollers; the middleware is not required to function on any other
platform.

|     ID     | Requirement                                                                                                                                                                                            | Rationale                                                                                                           |
|:----------:|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------|
| REQ-USE-01 | Adapting the middleware to a target application shall not require changes to the board and transport abstraction layer, and shall require only an appropriate implementation of the porting interface. | All application-specific behaviour is confined to the porting interface and its implementation.                     |
| REQ-USE-02 | The middleware shall obtain SPI, DMA and EXTI ISR callback registration and deregistration through the porting interface exclusively.                                                                  | Enables sharing of the SPI, DMA and EXTI peripherals.                                                               |
| REQ-USE-03 | The middleware shall not claim exclusive ownership of the SPI, DMA and EXTI resources.                                                                                                                 | As there is no reason to make an exclusive claim to either of SPI, DMA and EXTI peripherals, they remain shareable. |
| REQ-USE-04 | Selecting between DMA-based and interrupt-based SPI transfers shall require a single configuration change in the porting interface.                                                                    | &mdash;                                                                                                             |
| REQ-USE-05 | The middleware shall determine transfer-completion blocking through the porting interface without imposing a particular blocking mechanism.                                                            | The blocking mechanism is user-defined in the porting interface, so the user may adopt any waiting strategy.        |

### 3.4 Interface

This subsection specifies the requirements that apply to the hardware interface
exposed by the middleware to the WINC1500 module on one side, and the software
interface exposed to the user application &mdash; the porting interface &mdash;
on the other.

#### 3.4.1 Hardware Interface

|    ID     | Requirement                                                                                                                                                                                                                                                                                            | Rationale                                                                        |
|:---------:|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------|
| REQ-IF-01 | The board &amp; transport layer shall expose the module control signals &mdash; CHIP_EN, RESET_N, IRQN &mdash; through the user-defined pins.                                                                                                                                                          | &mdash;                                                                          |
| REQ-IF-02 | The board &amp; transport layer shall expose the SPI data interface &mdash; SCK, MOSI, MISO &mdash; and SS through the user-defined pins.                                                                                                                                                              | &mdash;                                                                          |
| REQ-IF-03 | The board &amp; transport layer shall configure the SPI interface as a full-duplex master, with 8-bit data, most-significant-bit-first, clock polarity low and phase first-edge, software-controlled SS, and an SPI clock within the limit set by REQ-PERF-01.                                         | This configuration is required by the module for SPI communication.              |
| REQ-IF-04 | When the DMA option is enabled, the board &amp; transport layer shall require the SPI TX and RX DMA streams to hold the module-required DMA parameters: memory-to-peripheral / peripheral-to-memory direction (respectively), peripheral-increment disabled, memory-increment enabled and normal mode. | These configurations ensure data flow correctness and prevent memory corruption. |

#### 3.4.2 Porting Interface

|    ID     | Requirement                                                                                                                                                                                                                                                                                                                                                                                    |
|:---------:|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| REQ-IF-05 | The porting interface shall specify the GPIO port and pin for each module control and communication signal: IRQN, CHIP_EN, RESET_N, SS, and the SPI data lines (SCK, MOSI, MISO).                                                                                                                                                                                                              |
| REQ-IF-06 | The porting interface shall provide mechanisms to register and deregister an ISR callback for the IRQN EXTI signal.                                                                                                                                                                                                                                                                            |
| REQ-IF-07 | The porting interface shall provide mechanisms to register and deregister an ISR callback for each SPI event: transmit-complete, receive-complete, transmit-receive-complete, and error.                                                                                                                                                                                                       |
| REQ-IF-08 | The porting interface shall provide an acquire operation that blocks the invoking thread until the SPI bus is exclusively available, and a corresponding release operation.                                                                                                                                                                                                                    |
| REQ-IF-09 | The porting interface shall provide: a prepare operation that resets transfer-completion state before each transfer; a wait operation that blocks the invoking thread until completion or failure; a notify-success operation and a notify-failure operation that unblock a pending wait and record the outcome; and a means to query whether the last completed transfer succeeded or failed. |
| REQ-IF-10 | When the DMA option is enabled, the porting interface shall additionally identify the SPI TX and RX DMA streams and provide an initialisation function for each.                                                                                                                                                                                                                               |
| REQ-IF-11 | The porting interface shall provide a system clock configuration function.                                                                                                                                                                                                                                                                                                                     |

### 3.5 Design Constraints

This subsection lists the design constraints imposed on the middleware by the
vendor contract, the target platform and the project limitations. They restrict
the design freedom of the middleware, but do not of themselves describe
behaviour in response to an invocation.

|     ID     | Requirement                                                                                                                                                                                      |
|:----------:|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| REQ-DES-01 | The middleware shall implement the BSP and bus wrapper interfaces exactly as declared in the vendor headers `winc1500/bsp/include/nm_bsp.h` and `winc1500/bus_wrapper/include/nm_bus_wrapper.h`. |
| REQ-DES-02 | The middleware shall not require modifications to the vendor driver files beyond the documented exceptions in [5.1](#51-assumptions-and-dependencies).                                           |
| REQ-DES-03 | The middleware shall implement the SPI transport part of the bus wrapper.                                                                                                                        |
| REQ-DES-04 | The middleware shall require a configuration setting that selects a supported bus transport; a configuration selecting no transport, or an unsupported one, shall be rejected at compile time.   |
| REQ-DES-05 | The middleware shall target the STM32F401xE family of microcontrollers.                                                                                                                          |
| REQ-DES-06 | The middleware shall be implemented in the C11 / GNU11 language standard and shall have the STM32 HAL and the STM32 CMSIS headers as platform dependencies.                                      |
| REQ-DES-07 | The middleware shall be developed against the WINC1500 driver and module firmware version 19.7.11.                                                                                               |

Note: Given REQ-DES-05, freedom is allowed with regard to exploitation of
platform-specific behaviour.

### 3.6 Software System Attributes

This subsection specifies the attributes that the middleware implementation shall
possess beyond its functional behaviour, namely reliability and
maintainability. Each attribute is stated as a single, verifiable requirement.

#### 3.6.1 Reliability

|     ID      | Requirement                                                                                                                                                    | Rationale                                                                                                                          |
|:-----------:|----------------------------------------------------------------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------|
| REQ-ATTR-01 | The middleware shall perform SPI data transfers from the invoking thread context only; it shall not perform any SPI transfer from within an interrupt handler. | Transfers block the invoking context until completion (REQ-FUN-37), whereas ISRs should reduce execution time as much as possible. |
| REQ-ATTR-02 | The middleware shall report failures not resolved internally to the invoking driver as error return values.                                                    | Refer back to REQ-FUN-33.                                                                                                          |
| REQ-ATTR-03 | The board and transport layer shall regulate access to its internally shared state in a thread-safe manner.                                                    | &mdash;                                                                                                                            |

#### 3.6.2 Maintainability

|     ID      | Requirement                                                                 |
|:-----------:|-----------------------------------------------------------------------------|
| REQ-ATTR-04 | The middleware shall document the functions of its public API with Doxygen. |

## 4. Verification

This section specifies, for each requirement in Section 3, the verification
method planned to provide objective evidence that the requirement is fulfilled.
The methods used are the following.

- **Inspection** &mdash; examination of the implementation against the vendor
  interface declarations, the module datasheet and the configuration surface,
  without executing it. Includes build-time and compile-time checks.
- **Analysis** &mdash; logical reasoning from implementation constants or code
  structure to a behavioural conclusion that cannot be directly read from the
  artefact (e.g. "the delay is 1 ms, which satisfies the ≥2 μs requirement"),
  without executing the middleware on hardware.
- **Test** &mdash; host-executed unit tests, using the build's `BUILD_TESTING`
  option, the `STATIC` symbol export and mocks of the STM32 HAL and CMSIS.

Hardware-based verification is intentionally excluded: exercising the middleware
on target would additionally require an application firmware and a porting
implementation, adding points of failure unrelated to the middleware itself.

Validation &mdash; confirming that the middleware achieves its intended use, an
STM32F401xE interfacing with the WINC1500 over SPI, in its intended environment
&mdash; is outside the scope of this section.

Each method cell states the technique applied for the corresponding requirement;
an em dash (&mdash;) means the requirement does not require that method. Each
row is identified by the requirement it verifies.

### 4.1 Functions

| Requirement | Analysis                                                          | Test                                                                                                  |
|:-----------:|-------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------|
| REQ-FUN-01  | &mdash;                                                           | Mocked GPIO: CHIP_EN and RESET_N held low on return.                                                  |
| REQ-FUN-02  | &mdash;                                                           | Mocked GPIO: IRQN initialised with pull-up resistor and falling edge interrupt.                       |
| REQ-FUN-03  | &mdash;                                                           | Mocked porting API: no driver ISR callback registered on init.                                        |
| REQ-FUN-04  | &mdash;                                                           | Return value asserted.                                                                                |
| REQ-FUN-05  | &mdash;                                                           | Mocked GPIO: control pins deinitialised.                                                              |
| REQ-FUN-06  | &mdash;                                                           | Return value asserted.                                                                                |
| REQ-FUN-07  | Derive the low hold time from the implementation.                 | Mocked GPIO: CHIP_EN and RESET_N held low initially.                                                  |
| REQ-FUN-08  | Derive the CHIP_EN-before-RESET_N timing from the implementation. | Mocked GPIO: CHIP_EN and RESET_N held high on return.                                                 |
| REQ-FUN-09  | &mdash;                                                           | Mocked delay: blocking duration asserted.                                                             |
| REQ-FUN-10  | &mdash;                                                           | Mocked porting API: EXTI callback registered.                                                         |
| REQ-FUN-11  | &mdash;                                                           | Mocked NVIC: enable and disable calls asserted.                                                       |
| REQ-FUN-12  | &mdash;                                                           | Mocked SPI: init function invoked even when already configured.                                       |
| REQ-FUN-13  | &mdash;                                                           | Mocked porting API: DMA TX and RX init invoked when not configured.                                   |
| REQ-FUN-14  | &mdash;                                                           | Mocked DMA: reconfigured when a parameter is wrong.                                                   |
| REQ-FUN-15  | &mdash;                                                           | Mocked SPI, DMA and NVIC: SPI and DMA wired, DMA interrupt enabled in NVIC.                           |
| REQ-FUN-16  | &mdash;                                                           | Mocked porting API and NVIC: SPI callbacks registered and SPI enabled in NVIC.                        |
| REQ-FUN-17  | &mdash;                                                           | Mocked GPIO: SS deasserted on completion.                                                             |
| REQ-FUN-18  | &mdash;                                                           | Mocked BSP: module reset invoked.                                                                     |
| REQ-FUN-19  | &mdash;                                                           | Return value asserted.                                                                                |
| REQ-FUN-20  | &mdash;                                                           | Mocked GPIO and porting API: SS deinitialised, handlers deregistered.                                 |
| REQ-FUN-21  | &mdash;                                                           | Mocked SPI and DMA: no functions affecting them are called.                                           |
| REQ-FUN-22  | &mdash;                                                           | Return value asserted.                                                                                |
| REQ-FUN-23  | &mdash;                                                           | Mocked underlying function calls: fix second parameter and vary the first.                            |
| REQ-FUN-24  | &mdash;                                                           | Return value asserted.                                                                                |
| REQ-FUN-25  | &mdash;                                                           | Mocked underlying function calls: second parameter asserted.                                          |
| REQ-FUN-26  | &mdash;                                                           | Mocked underlying function calls: return value redirection asserted.                                  |
| REQ-FUN-27  | &mdash;                                                           | Return value asserted.                                                                                |
| REQ-FUN-28  | &mdash;                                                           | Return value asserted.                                                                                |
| REQ-FUN-29  | &mdash;                                                           | Mocked SPI: transmit-only transfer, return value asserted.                                            |
| REQ-FUN-30  | &mdash;                                                           | Mocked SPI: receive-only transfer, return value asserted.                                             |
| REQ-FUN-31  | &mdash;                                                           | Mocked SPI: full-duplex transfer, return value asserted.                                              |
| REQ-FUN-32  | &mdash;                                                           | Mocked GPIO: SS asserted before and deasserted after the transfer.                                    |
| REQ-FUN-33  | &mdash;                                                           | Simulated transfer failure: return value asserted.                                                    |
| REQ-FUN-34  | &mdash;                                                           | Mocked porting API and GPIO: bus acquire invoked, then SS asserted.                                   |
| REQ-FUN-35  | &mdash;                                                           | Mocked porting API and GPIO: SS deasserted, then bus release invoked.                                 |
| REQ-FUN-36  | &mdash;                                                           | Mocked porting API and SPI: synchronisation prepare invoked, then transfer started.                   |
| REQ-FUN-37  | Reason that the wait cannot return before completion or failure.  | Mocked porting API and SPI: simulated completion and failure each unblock the caller and deassert SS. |
| REQ-FUN-38  | &mdash;                                                           | Mocked porting API: completion or failure signalled on each SPI event.                                |

### 4.2 Performance

| Requirement | Analysis                                                                       | Test                                                             |
|:-----------:|--------------------------------------------------------------------------------|:-----------------------------------------------------------------|
| REQ-PERF-01 | Match the SPI clock derivation against the module datasheet maximum of 48 MHz. | &mdash;                                                          |
| REQ-PERF-02 | &mdash;                                                                        | Assert that `egstrNmBusCapabilities.u16MaxTrxSz` is at least 16. |

### 4.3 Usability

| Requirement | Inspection                                                                                                                                                    | Analysis                                                                                               |
|:-----------:|---------------------------------------------------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------|
| REQ-USE-01  | Review that a mock porting implementation suffices for an error-less build and link of the middleware, with the board and transport layer sources unmodified. | &mdash;                                                                                                |
| REQ-USE-02  | Review that SPI, DMA and EXTI ISR callback registration and deregistration is leveraged exclusively via the porting interface.                                | &mdash;                                                                                                |
| REQ-USE-03  | &mdash;                                                                                                                                                       | Reason that the implementation makes no exclusive ownership claim over the SPI, DMA or EXTI resources. |
| REQ-USE-04  | Review that a single configuration parameter is solely tasked with switching between DMA and interrupt-based SPI transfers.                                   | &mdash;                                                                                                |
| REQ-USE-05  | Review that no call to a blocking function is made for the purpose of SPI synchronisation, except through the porting interface.                              | &mdash;                                                                                                |

### 4.4 Interface

| Requirement | Inspection                                                                                                                                                                                                                                                                                               |
|:-----------:|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
|  REQ-IF-01  | Review that the control signals are exposed through user-defined pins.                                                                                                                                                                                                                                   |
|  REQ-IF-02  | Review that the SPI data signals and SS are exposed through user-defined pins.                                                                                                                                                                                                                           |
|  REQ-IF-03  | Review that the module-required SPI configuration is enforced.                                                                                                                                                                                                                                           |
|  REQ-IF-04  | Review that the module-required DMA configuration parameters are enforced.                                                                                                                                                                                                                               |
|  REQ-IF-05  | Review that the porting interface specifies the GPIO port and pin for each of IRQN, CHIP_EN, RESET_N, SS, SCK, MOSI, and MISO.                                                                                                                                                                           |
|  REQ-IF-06  | Review that the porting interface provides mechanisms to register and deregister an ISR callback for the IRQN EXTI signal.                                                                                                                                                                               |
|  REQ-IF-07  | Review that the porting interface provides mechanisms to register and deregister an ISR callback for each SPI event: transmit-complete, receive-complete, transmit-receive-complete, and error.                                                                                                          |
|  REQ-IF-08  | Review that the porting interface provides an acquire operation, blocking the invoking thread until SPI bus availability, with a release operation to match.                                                                                                                                             |
|  REQ-IF-09  | Review that the porting interface provides the sync operations: prepare, resets transfer-completion state; wait, blocks until completion or failure; notify-success and notify-failure, unblock a pending wait and record the outcome; and that the outcome of the last completed transfer is queryable. |
|  REQ-IF-10  | When DMA is enabled: review that the porting interface identifies the SPI TX and RX DMA streams and provides an initialisation function for each.                                                                                                                                                        |
|  REQ-IF-11  | Review that the porting interface provides a system clock configuration function.                                                                                                                                                                                                                        |

### 4.5 Design Constraints

| Requirement | Inspection                                                                                                                                                                   |
|:-----------:|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| REQ-DES-01  | Compare the implemented signatures against the vendor interface declarations.                                                                                                |
| REQ-DES-02  | Review the vendor files against the reference distribution, confirming no modifications &mdash; beyond the exceptions documented in [5.1](#51-assumptions-and-dependencies). |
| REQ-DES-03  | Review that the SPI transport is implemented.                                                                                                                                |
| REQ-DES-04  | Builds with no supported transport selected fail through the `#error` guard, while others don't.                                                                             |
| REQ-DES-05  | Review that the imported HAL and CMSIS headers match those of the STM32F401xE.                                                                                               |
| REQ-DES-06  | Review for C11 standard compliance, and that the middleware makes use of STM32 HAL and CMSIS.                                                                                |
| REQ-DES-07  | Review that the middleware builds, links and obeys the expectations and contracts of the WINC1500 driver and module firmware of version 19.7.11.                             |

### 4.6 Software System Attributes

| Requirement | Inspection                                                                        | Analysis                                                     |
|:-----------:|-----------------------------------------------------------------------------------|--------------------------------------------------------------|
| REQ-ATTR-01 | Review that SPI transfers are only initiated from within invoking thread context. | &mdash;                                                      |
| REQ-ATTR-02 | Review that, after any internal recovery attempts, further failures are reported. | &mdash;                                                      |
| REQ-ATTR-03 | &mdash;                                                                           | Audit access to internally shared state for race conditions. |
| REQ-ATTR-04 | Review Doxygen coverage of the public API.                                        | &mdash;                                                      |

## 5. Appendices

### 5.1 Assumptions and dependencies

The middleware assumes a WINC1500 driver and module firmware of version 19.7.11.

The middleware depends on the STM32 HAL, and on the user to supply a porting
implementation.

Building the middleware additionally requires resolving a known vendor-driver
linking quirk: `nm_common.h` declares `at_sb_printf` as a file-scope tentative
definition (empty `NMI_API`), which produces a multiple-definition link error
under GCC's default `-fno-common` (GCC 10 and later). The accepted resolutions
are the `extern` modifier, applied in this project, or the `-fcommon` compiler
flag. Moreover, depending on the build environment, some includes may not
resolve correctly due to some vendor files using backslash (`\`) paths.
