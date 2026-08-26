# Middleware Unit Test Coverage

**Draft v0.1** &mdash; reflects commit `eac53c4` (branch `master`). Suite status at
this revision: **17 tests, all passing** (`ctest`, 6 BSP + 11 bus wrapper).

## 1. Purpose and scope

This document maps every middleware requirement that is verified by the
host-executed unit-test suite to the test(s) providing that verification. It is
the concrete instantiation of the *Test* column of the SRS (&sect;4);
requirements verified exclusively by Inspection or Analysis &mdash; `REQ-PERF-01`,
`REQ-USE-*`, `REQ-DES-*`, `REQ-ATTR-*` &mdash; are intentionally absent here and
remain covered by SRS &sect;4.3&ndash;&sect;4.6.

Tables are ordered **verification &rarr; tests**: scanning the requirement IDs
against the SRS makes coverage gaps directly visible. The inverse mapping is
recoverable from any row, and each test additionally annotates its own coverage
in its Doxygen comment (the two are kept in sync).

## 2. Suite overview

| Aspect | Detail |
|---|---|
| Framework | Unity + CMock v2.7.0 (CMake `FetchContent`; Ruby required for mock generation) |
| Targets | `test_middleware_bsp` (6 tests), `test_middleware_bus_wrapper` (11 tests) |
| SUTs | `winc1500/bsp/source/nm_bsp_stm32f401xe.c`, `winc1500/bus_wrapper/source/nm_bus_wrapper_stm32f401xe.c` |
| Build & run | `cmake --build cmake-build-debug-testing && ctest --test-dir cmake-build-debug-testing --output-on-failure` |

Behaviour-pinning mechanisms the traceability below relies on:

- `:when_ptr: :compare_ptr` &mdash; pointer arguments compare by address, so
  forwarded buffers/handles are asserted identity-wise (REQ-FUN-25).
- `fail_on_unexpected_calls: true` &mdash; unset expectations double as
  *absence-of-calls* assertions (e.g. REQ-FUN-21, REQ-FUN-24 bus inactivity).
- `:enforce_strict_ordering: true` &mdash; expectations must be consumed in the
  global order they are queued; the FIFO chains in the transfer tests thereby
  pin the SS/synchronisation sequencing of REQ-FUN-32, 34&ndash;37.
- Per-target `BUILD_TESTING` export of `STATIC` functions exposes the ISR
  handlers for direct invocation (REQ-FUN-38).
- Transfer-selection expectations are `#ifdef CONF_WINC_SPI_USE_DMA` guarded
  with `_IT` alternatives, so the same source serves a future non-DMA build;
  only the DMA configuration is a CMake target today.

Coverage summary: `REQ-FUN-01..38` and `REQ-PERF-02` verified; no functional
requirement lacks a test.

## 3. Requirement-to-test traceability

### 3.1 BSP functions (`tests/middleware/test_bsp.c`)

| ID | Verified by | Asserts |
|---|---|---|
| REQ-FUN-01 | `test_nm_bsp_init_configures_gpio_pins_deregisters_isr_enables_irqn_irq_and_powers_off_module_and_returns_M2M_SUCCESS` | CHIP_EN and RESET_N driven low on return |
| REQ-FUN-02 | (same as REQ-FUN-01) | IRQN pin initialised falling-edge with pull-up |
| REQ-FUN-03 | (same as REQ-FUN-01) | EXTI ISR deregistered before interrupt enable |
| REQ-FUN-04 | (same as REQ-FUN-01) | Returns `M2M_SUCCESS` |
| REQ-FUN-05 | `test_nm_bsp_deinit_disables_irqn_interrupt_deinitialises_gpio_pins_and_returns_M2M_SUCCESS` | IRQN interrupt disabled; control pins deinitialised |
| REQ-FUN-06 | (same as REQ-FUN-05) | Returns `M2M_SUCCESS` |
| REQ-FUN-07 | `test_nm_bsp_reset_executes_reset_sequence` | RESET_N and CHIP_EN held low first, with >= 1 ms hold |
| REQ-FUN-08 | (same as REQ-FUN-07) | CHIP_EN rises before RESET_N, with >= 10 ms separation |
| REQ-FUN-09 | `test_nm_bsp_sleep_forwards_to_HAL_Delay` | Sleep duration forwarded verbatim |
| REQ-FUN-10 | `test_nm_bsp_register_isr_updates_internal_isr_callback_and_registers_isr` | Callback stored; `mw_port_exti_register_isr(irqn_on_irq)` invoked |
| REQ-FUN-11 | `test_nm_bsp_interrupt_ctrl_enables_and_disables_irqn_irq` | Input 1 enables the EXTI IRQ in NVIC, 0 disables it |

### 3.2 Bus wrapper: initialisation, deinitialisation, dispatch (`tests/middleware/test_bus_wrapper.c`)

| ID | Verified by | Asserts |
|---|---|---|
| REQ-FUN-12 | `test_nm_bus_init_configures_spi_interface_and_ss_pin` | All SPI handle fields hold the module-required values after an unconditional `HAL_SPI_Init` |
| REQ-FUN-13 | `test_nm_bus_init_on_dma_misconfigured_overwrites_config`; `test_nm_bus_init_on_failure_returns_negative_error_code` | Port DMA inits invoked when handles are in reset; suppression when already configured (verified in the failure-path scenarios, where `HAL_DMA_STATE_READY` must not trigger them) |
| REQ-FUN-14 | `test_nm_bus_init_on_dma_misconfigured_overwrites_config`; `test_nm_bus_init_on_failure_returns_negative_error_code` | Misconfigured Direction/Mode corrected via `HAL_DMA_Init`; heal failure reported |
| REQ-FUN-15 | `test_nm_bus_init_on_dma_misconfigured_overwrites_config` | `hdmatx`/`hdmarx` linked (both directions, incl. `Parent`), both DMA IRQs prioritised and enabled in NVIC |
| REQ-FUN-16 | `test_nm_bus_init_configures_spi_interface_and_ss_pin` | All four event handlers registered with the exported handlers; SPI IRQ enabled in NVIC; handlers signal completion/failure (also REQ-FUN-38) |
| REQ-FUN-17 | `test_nm_bus_init_deasserts_ss_pin_and_calls_nm_bsp_reset` | SS driven high on completion |
| REQ-FUN-18 | (same as REQ-FUN-17) | `nm_bsp_reset()` invoked |
| REQ-FUN-19 | `test_nm_bus_init_configures_spi_interface_and_ss_pin`; `test_nm_bus_init_on_dma_misconfigured_overwrites_config`; `test_nm_bus_init_deasserts_ss_pin_and_calls_nm_bsp_reset`; `test_nm_bus_init_on_failure_returns_negative_error_code` | `M2M_SUCCESS` on the success paths; `M2M_ERR_BUS_FAIL` on each of the five failure paths (TX/RX port init, TX/RX self-heal, SPI init) |
| REQ-FUN-20 | `test_nm_bus_deinit_deinitialises_ss_pin_and_deregisters_spi_isrs` | SS pin deinitialised; all four handlers deregistered |
| REQ-FUN-21 | (same as REQ-FUN-20) | No SPI/DMA-affecting call occurs (absence enforced by fail-on-unexpected-calls) |
| REQ-FUN-22 | (same as REQ-FUN-20) | Returns `M2M_SUCCESS` |
| REQ-FUN-23 | `test_nm_bus_ioctl_on_rw_opcode_forwards_its_second_parameter_to_nm_spi_rw_and_propagates_back_its_return_value`; `test_nm_bus_ioctl_on_non_rw_opcode_returns_invalid_arg` | Opcode selects the RW operation; together the two tests cover the dispatcher exhaustively (the opcode can only select RW or nothing) |
| REQ-FUN-24 | `test_nm_bus_ioctl_on_non_rw_opcode_returns_invalid_arg` | All defined non-SPI opcodes plus an out-of-range value rejected with `M2M_ERR_INVALID_ARG`, bus untouched |
| REQ-FUN-25 | `test_nm_bus_ioctl_on_rw_opcode_forwards_its_second_parameter_to_nm_spi_rw_and_propagates_back_its_return_value` | Buffers and size from the parameter struct arrive identity-wise at the transfer call |
| REQ-FUN-26 | (same as REQ-FUN-25) | Underlying result propagated on both success and failure |

### 3.3 Bus wrapper: transfer path and synchronisation

| ID | Verified by | Asserts |
|---|---|---|
| REQ-FUN-27 | `test_nm_spi_rw_on_invalid_arguments_returns_bus_fail` | Zero size rejected before any bus activity |
| REQ-FUN-28 | (same as REQ-FUN-27) | Both buffers NULL rejected |
| REQ-FUN-29 | `test_nm_spi_rw_on_null_rx_buffer_transmits_half_duplex` | Transmit-only selection (`HAL_SPI_Transmit_DMA`) with forwarded buffer/size |
| REQ-FUN-30 | `test_nm_spi_rw_on_null_tx_buffer_receives_half_duplex` | Receive-only selection (`HAL_SPI_Receive_DMA`) |
| REQ-FUN-31 | `test_nm_bus_ioctl_on_rw_opcode_forwards_its_second_parameter_to_nm_spi_rw_and_propagates_back_its_return_value` | Full-duplex selection (`HAL_SPI_TransmitReceive_DMA`) |
| REQ-FUN-32 | Jointly: the REQ-FUN-29, REQ-FUN-30 and REQ-FUN-31 tests | SS asserted before and deasserted after each transfer (strict-order chains) |
| REQ-FUN-33 | `test_nm_bus_ioctl_on_rw_opcode_forwards_its_second_parameter_to_nm_spi_rw_and_propagates_back_its_return_value` (failure phase) | Error status yields deassert + release + `M2M_ERR_BUS_FAIL` |
| REQ-FUN-34&ndash;37 | Jointly: the REQ-FUN-29, REQ-FUN-30 and REQ-FUN-31 tests | Strict-order chains pin acquire&rarr;SS assert&rarr;prepare&rarr;transfer&rarr;wait&rarr;error query&rarr;SS deassert&rarr;release |
| REQ-FUN-38 | `test_nm_bus_init_configures_spi_interface_and_ss_pin` | Registered success/error handlers invoke `mw_port_spi_sync_notify(_error)` on the owned SPI instance |
| REQ-PERF-02 | `test_spi_transfer_capacity_greater_than_16_bytes` | `egstrNmBusCapabilities.u16MaxTrxSz` >= 16 (implementation sets 1024) |

## 4. Notes and limitations

1. **Non-DMA (IT) transfer paths**: the `_IT` expectation variants are present
   behind `CONF_WINC_SPI_USE_DMA` guards but only the DMA configuration is
   built and run today; exercising the IT branches requires building the same
   sources with the option off (per REQ-USE-04, the switch itself remains an
   Inspection item in the SRS).
2. **REQ-FUN-12 "replacing any configuration in effect"**: the forced-config
   assertion currently starts from a zeroed handle. Pre-seeding a deliberately
   wrong user configuration before `nm_bus_init` would strengthen the claim.
3. **REQ-FUN-38 instance matching**: the notification-on-owned-instance path is
   verified positively; the ignore-on-foreign-instance branch is not exercised.
4. A blocking full-duplex redesign of `nm_spi_rw` was explored and reverted
   (2026-08-26, see `.agents/CONTEXT.md` &sect;6); should it be revived, the
   transfer-path tests above require realignment.
