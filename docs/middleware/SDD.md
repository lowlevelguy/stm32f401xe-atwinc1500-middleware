# Software Design Description: STM32F401xE-WINC1500 Middleware
Version 0.1.

## 1. Identification and Overview

This document is the software design description (SDD) of the
STM32F401xE-WINC1500 middleware. It is structured following IEEE Std 1016-2009,
though claims of conformance are delayed until the document is fully elaborated.
Its intended purpose, design subject and environment, and scope are stated
below, and the remaining sections express the design itself. It is a companion
to, and traces to, the software requirements specification (`SRS.md`, v1.0) and
the software architecture description (`SAD.md`, v1.0) of the same middleware.

### 1.1 Identification

| Field                    | Value                                                                                                    |
|--------------------------|----------------------------------------------------------------------------------------------------------|
| Document title           | Software Design Description: STM32F401xE-WINC1500 Middleware                                             |
| Document identifier      | SDD-MW-001                                                                                               |
| Version                  | 0.1                                                                                                      |
| Status                   | Draft                                                                                                    |
| Date of issue            | 2026-10-13                                                                                               |
| Author                   | lowlevelguy                                                                                              |
| Reviewers                | None                                                                                                     |
| Approving authority      | The author                                                                                               |
| Issuing organisation     | Not applicable                                                                                           |
| Configuration management | Version-controlled as a single file in this repository                                                   |
| Governing standard       | IEEE 1016-2009                                                                                           |
| Upstream documents       | `SAD.md` (software architecture description, v1.0); `SRS.md` (software requirements specification, v1.0) |

Change history:

| Version | Date       | Description                                    |
|---------|------------|------------------------------------------------|
| 0.1     | 2026-10-16 | Initial draft (frontispiece and Section 1).    |

### 1.2 Purpose

The purpose of this design description is to express the detailed design of the
design subject: its decomposition into design entities, the allocation of the
SRS requirements to those entities, the interfaces it exposes and consumes, its
interaction and dynamic behaviour, its internal algorithms, its mapping onto the
hardware resources of the target platform, and the design decisions, with their
rationales, that account for its present shape.

The document is intended to serve three uses:

- as the software detailed design required by ASPICE SWE.3;
- as the object of the design-level verification methods planned in the SRS, and
  as a reference for unit-level verification; and
- as a means of communicating the detailed design to the stakeholders identified
  in §2.

It does not restate the requirements or the architecture: the SRS is the
authority on what the middleware shall do, and the SAD on the architecture of
the design subject. Where a design view addresses an architectural element, it
identifies that element's source in the SAD and remains consistent with it;
the design may further refine choices that the architecture leaves open.

### 1.3 Context

#### 1.3.1 Design Subject

The design subject is the STM32F401xE-WINC1500 middleware. It comprises two
parts: the board and transport abstraction layer, and the porting interface.

The board and transport abstraction layer is the implementation, for the
STM32F401xE family of microcontrollers, of the two Hardware Abstraction Layer
interfaces expected by the vendor driver (that is, the Microchip WINC1500
driver): the Board Support Package (BSP) and the bus wrapper.

The BSP is responsible for module power control and module-to-host interrupt
management, while the bus wrapper is responsible for abstracting away data bus
operations from the vendor driver; however, only the parts of the latter that
are relevant to SPI-based communication are implemented.

The porting interface is a surface that the middleware defines for the purposes
of its configuration. By way of it, the middleware obtains application-specific
hardware peripheral mappings and configurations (GPIO, EXTI, SysTick, SPI, and,
optionally, DMA), synchronisation primitives for thread-safe SPI data transfers,
and hooks for module-to-host interrupt and SPI event handling.

#### 1.3.2 Environment

The environment of the design subject is the four-stratum software stack into
which it is embedded. The topmost stratum is the application firmware, followed
by the vendor driver, then our middleware, and finally, at the very bottom, the
STM32 HAL and CMSIS libraries. Going down a stratum thus corresponds to
stripping away a layer of abstraction.

This software stack is physically embedded on an STM32F401xE microcontroller,
connected via SPI to an ATWINC1500 module running its own firmware (version
19.7.11).

The user application &mdash; the application firmware together with the porting
implementation &mdash; is part of the environment, not of the design subject. In
particular, the porting implementation is written by the user and lies outside
the middleware boundary; a reference realisation is cited in this document to
make the porting interface concrete, but its design is not described here.

The runtime environment can be bare metal or an RTOS at the user's discretion;
the middleware is agnostic about the choice, as all blocking and synchronisation
behaviour is obtained through the porting interface.

### 1.4 Scope

This design description covers:

- the board and transport abstraction layer, that is, its BSP and bus wrapper
  (§1.3.1);
- the porting interface, as a contract.

It does not cover, save where needed to describe the design subject's interfaces
and interaction:

- the design of the vendor driver, which is described only as consumed, through
  its call-in and call-out contracts;
- the design of the porting implementation, or of any other part of the user
  application, which appears only as a reference realisation of the porting
  interface; and
- verification on target hardware (testing against the real module), which the
  SRS excludes from its verification scope; the SDD therefore makes no provision
  for it.

### 1.5 References

- [Software Architecture Description: STM32F401xE-WINC1500 Middleware](SAD.md),
  v1.0.
- [Software Requirements Specification: STM32F401xE-WINC1500 Middleware](SRS.md),
  v1.0.
- IEEE 1016-2009, *IEEE Standard for Information Technology — Systems Design —
  Software Design Descriptions*.
- Microchip ATWINC1500 module datasheet (`DS70005304F`, 2025).
- ATWINC1500 19.7.11 software API reference manual.
- ST STM32F401xE reference manual (`RM0368`, Rev 6, January 2025).

### 1.6 Glossary

This glossary defines the IEEE 1016 design-description vocabulary used in this
document. Project terms, including BSP, bus wrapper, porting interface and
IRQN, are defined in SRS §1.4; architecture-description terms are defined in
SAD §1.6 and are not repeated here.

| Term                | Meaning                                                                                                                                      |
|---------------------|----------------------------------------------------------------------------------------------------------------------------------------------|
| Design attribute    | A characteristic or property of a design entity, relationship or constraint.                                                                 |
| Design concern      | An area of interest with respect to a software design.                                                                                       |
| Design constraint   | A rule or restriction imposed on a design entity, attribute or relationship.                                                                 |
| Design element      | An item in a design view: a design entity, relationship, attribute or constraint.                                                            |
| Design entity       | A structurally, functionally or otherwise distinct element of a design, or one that plays a distinct role relative to other design entities. |
| Design overlay      | A representation of additional, detailed or derived design information organised with reference to an existing design view.                  |
| Design rationale    | Information capturing the reasoning that led to the design, including options, trade-offs, decisions and their justifications.               |
| Design relationship | A connection or correspondence between design entities.                                                                                      |
| Design stakeholder  | An individual, organisation or group having an interest in, or design concerns relative to, the design of a software item.                   |
| Design subject      | The middleware described by this SDD. IEEE Std 1016-2009 uses this term in its design vocabulary.                                            |
| Design view         | A representation comprising design elements that addresses concerns from a specified design viewpoint.                                       |
| Design viewpoint    | The specification of the elements and conventions available for constructing and using a design view.                                        |
