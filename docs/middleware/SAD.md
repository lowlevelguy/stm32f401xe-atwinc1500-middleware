# Software Architecture Description: STM32F401xE-WINC1500 Middleware
Version 0.1.

## 1. Identification and Overview

This document is the architecture description (AD) of the STM32F401xE-WINC1500
middleware. It is structured after ISO/IEC/IEEE 42010:2022 and claims
conformance to that standard as an architecture description; its intended
purpose, entity of interest, environment, scope and conformance are stated
below, and the remaining sections express the architecture itself. It is a
companion to, and traces to, the software requirements specification (SRS) of
the same middleware (`SRS.md`, v1.0).

### 1.1 Identification

| Field                    | Value                                                              |
|--------------------------|--------------------------------------------------------------------|
| Document title           | Software Architecture Description: STM32F401xE-WINC1500 Middleware |
| Document identifier      | SAD-MW-001                                                         |
| Version                  | 0.1                                                                |
| Status                   | Draft                                                              |
| Date of issue            | 2026-09-15                                                         |
| Author                   | lowlevelguy                                                        |
| Reviewers                | None                                                               |
| Approving authority      | The author                                                         |
| Issuing organisation     | Not applicable                                                     |
| Configuration management | Version-controlled as a single file in this repository             |
| Governing standard       | ISO/IEC/IEEE 42010:2022                                            |
| Upstream document        | `SRS.md` (software requirements specification, v1.0)               |

Change history:

| Version | Date       | Description                |
|---------|------------|----------------------------|
| 0.1     | 2026-09-15 | Initial draft (Section 1). |

### 1.2 Purpose

The purpose of this architecture description is to express the architecture of
the entity of interest: its decomposition into parts, the allocation of the SRS
requirements to those parts, the interfaces it exposes and consumes, its
interaction and dynamic behaviour, its mapping onto the hardware resources of
the target platform, and the architecture decisions and their rationale that
account for its present shape.

The document is intended to serve three uses:

- as the architectural design of ASPICE SWE.2, from which the software detailed
  design (SWE.3) is derived;
- as the object of the architecture-level verification methods planned in the
  SRS, and as a reference for unit-level verification; and
- as a means of communicating the architecture to the stakeholders identified
  in Section 2.

It does not restate the requirements: the SRS is the authority on what the
middleware shall do, and every architecture element in this document traces to
it.

### 1.3 Entity of Interest and Environment

#### 1.3.1 Entity of Interest

The entity of interest (EoI) is the STM32F401xE-WINC1500 middleware. It
comprises two parts: the board and transport abstraction layer, and the porting
interface.

The board and transport abstraction layer is the implementation, for the
STM32F401xE family of microcontrollers, of the two Hardware Abstraction Layer
interfaces expected by the vendor driver &mdash; that is, the Microchip WINC1500
driver &mdash;: the Board Support Package (BSP) and the bus wrapper.

The BSP is responsible for module power control and module-to-host interrupt
management, while the bus wrapper is responsible for abstracting away data bus
operations from the vendor driver; however, only the parts of the latter that
are relevant to SPI-based communication are implemented.

The porting interface is a surface that the middleware defines for the purposes
of its configuration. By way of it, the middleware obtains application-specific
hardware peripheral mappings and configurations (GPIO, EXTI, SysTick, SPI, and,
optionally, DMA), synchronisation primitives for thread-safe SPI data transfers,
and hooks for module-to-host interrupt and SPI event handling.

These parts and the interfaces through which they relate are expressed in the
internal decomposition view (§4.3) and the structure and interfaces view
(§4.2).

#### 1.3.2 Environment

The environment of the EoI is the four-stratum software stack into which it is
embedded. The topmost stratum is the application firmware, followed by the
vendor driver, then our middleware, and finally, on the very bottom, the STM32
HAL and CMSIS libraries. Where going down a stratum is equivalent to stripping
down a layer of abstraction.

This software stack is physically embedded on an STM32F401xE microcontroller,
connected via SPI to an ATWINC1500 module running its own firmware (version
19.7.11).

The user application &mdash; the application firmware together with the porting
implementation &mdash; is part of the environment, not of the EoI. In
particular, the porting implementation is written by the user and lies outside
the middleware boundary; a reference realisation is cited in this document to
make the porting interface concrete, but its architecture is not described
here.

The runtime environment can be baremetal or an RTOS at the user's discretion;
the middleware is agnostic to the choice, as all blocking and synchronisation
behaviour is obtained through the porting interface.

The EoI and its environment, and the boundary between them, are expressed in
the context view (§4.1).

### 1.4 Scope

This architecture description covers:

- the board and transport abstraction layer, in the two parts defined above;
- the porting interface, as a contract.

It does not cover, save where needed to describe the EoI's interfaces and
interaction:

- the architecture of the vendor driver, which is described only as consumed,
  through its call-in and call-out contracts;
- the architecture of the porting implementation, or of any other part of the
  user application, which appears only as a reference realisation of the
  porting interface; and
- verification on target hardware &mdash; testing against the real module
  &mdash; which the SRS excludes from its verification scope; the AD therefore
  makes no provision for it.

The ownership boundary between the middleware and the vendor driver is expressed
in the ownership-boundary view (§4.8).

### 1.5 Document Structure

The document follows the architecture description content requirements of
ISO/IEC/IEEE 42010:2022, Clause 6. The mapping is as follows.

| Section | Title                                            | 42010:2022 clause |
|---------|--------------------------------------------------|-------------------|
| 1       | Identification and Overview                      | 6.1               |
| 2       | Stakeholders, Perspectives, Concerns and Aspects | 6.2&ndash;6.5     |
| 3       | Architecture Viewpoints                          | 6.6, 8.1          |
| 4       | Architecture Views                               | 6.7&ndash;6.8     |
| 5       | Architecture Correspondences                     | 6.9               |
| 6       | Architecture Decisions and Rationale             | 6.10              |

### 1.6 Glossary

This subsection defines the architecture-description vocabulary used throughout
the document, as adopted from ISO/IEC/IEEE 42010:2022. The project vocabulary of
the middleware &mdash; the layers and signals, and the meaning of
"user-defined" &mdash; is defined once in `SRS.md` §1.4 and is not repeated
here.

| Term                          | Meaning                                                                                                                                           |
|-------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------|
| Architecture                  | The fundamental concepts or properties of an entity in its environment, together with the governing principles for its realisation and evolution. |
| Architecture description (AD) | The work product that expresses an architecture.                                                                                                  |
| AD element                    | An identified or named part of an architecture description, such as a stakeholder, a concern, a view or a decision.                               |
| Entity of interest (EoI)      | The subject of the architecture description &mdash; here, the middleware.                                                                         |
| Environment                   | The surrounding things, conditions or influences upon the EoI, including the entities with which it interacts.                                    |
| Stakeholder                   | A role, position, individual or organisation having an interest in the EoI.                                                                       |
| Concern                       | A matter of relevance or importance to a stakeholder.                                                                                             |
| Stakeholder perspective       | A way of thinking about the EoI, especially as it relates to concerns.                                                                            |
| Aspect                        | A part of the EoI's character or nature; for example, its structural or behavioural character.                                                    |
| Architecture viewpoint        | A set of conventions for the creation, interpretation and use of an architecture view, framing one or more concerns.                              |
| Architecture view             | A portion of the architecture description that addresses the concerns framed by its governing viewpoint.                                          |
| View component                | A separable portion of one or more views, governed by a model kind or a legend.                                                                   |
| Model kind                    | A category of model, distinguished by its key characteristics and modelling conventions.                                                          |
| Correspondence                | An identified or named relationship between two or more AD elements, such as traceability or satisfaction.                                        |
| Architecture decision         | A decision about the architecture that is essential within the scope and purpose of the architecture description.                                 |
| Rationale                     | The explanation of why a decision was chosen, including the alternatives considered and rejected.                                                 |

### 1.7 References

- [Software Requirements Specification: STM32F401xE-WINC1500
  Middleware](SRS.md), v1.0.
- ISO/IEC/IEEE 42010:2022, *Software, systems and enterprise &mdash;
  Architecture description*.
- Microchip ATWINC1500 module datasheet (DS70005304F, 2025).
- ATWINC1500 19.7.11 Software API reference manual.
- ST STM32F401xE reference manual (`RM0368`, Rev 6, January 2025).