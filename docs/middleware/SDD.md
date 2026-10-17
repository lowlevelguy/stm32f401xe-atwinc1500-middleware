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

| Version | Date       | Description                                                                                |
|---------|------------|--------------------------------------------------------------------------------------------|
| 0.1     | 2026-10-16 | Initial draft (frontispiece and Section 1).                                                |
| 0.2     | 2026-10-17 | Document structure addendum to section 1, and section 2: design stakeholders and concerns. |

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

### 1.5 Document Structure

The document follows the software design description content requirements of
IEEE Std 1016-2009, Clause 4. The mapping is as follows; it covers the sections
elaborated so far and will be extended as §3 to §5 are written.

| Section | Title                            | 1016-2009 clause |
|---------|----------------------------------|------------------|
| 1       | Identification and Overview      | 4.2              |
| 2       | Design Stakeholders and Concerns | 4.3              |

### 1.6 References

- [Software Architecture Description: STM32F401xE-WINC1500 Middleware](SAD.md),
  v1.0.
- [Software Requirements Specification: STM32F401xE-WINC1500 Middleware](SRS.md),
  v1.0.
- IEEE Std 1016-2009, *IEEE Standard for Information Technology &mdash; Systems
  Design &mdash; Software Design Descriptions*.
- Microchip ATWINC1500 module datasheet (`DS70005304F`, 2025).
- ATWINC1500 19.7.11 software API reference manual.
- ST STM32F401xE reference manual (`RM0368`, Rev 6, January 2025).

### 1.7 Glossary

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

## 2. Design Stakeholders and Concerns

This section identifies the design stakeholders of the middleware and the design
concerns they hold. The identifiers introduced here are used throughout §3, §4
and §5.

### 2.1 Design Stakeholders

Two design stakeholders are identified; the same ones identified in the SAD as
architecture stakeholders.

| ID    | Role                    | Description                                                                                                                                                                                                                                                                                              |
|-------|-------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| STK-1 | Integrator              | Authors the user application and the porting implementation, and consumes the middleware through the vendor driver. SRS §1.3.1.1 defines the same role as the user of the middleware. The integrator is kept distinct from the maintainer in principle, as the role for which the middleware is written. |
| STK-2 | Maintainer and verifier | The middleware's author, who evolves it against future changes of the SRS and executes the verification methods of SRS §4 &mdash; analysis, inspection and host-executed unit tests.                                                                                                                     |

The design fixes, for the integrator, the porting interface as the sole
adaptation surface, such that the board and transport abstraction layer need
never be modified (REQ-USE-01). For the maintainer, it specifies the design
entities and their contracts so that the host-executed unit tests of SRS §4 can
verify them, and records the design decisions the architecture left open (§5);
both constrain how the middleware may evolve.

The roles above are discharged, in this project, by a single individual; the
integrator role is kept distinct in principle, as it is the role for which the
middleware is written. The single-author situation is the principal resource
limitation of the design effort, and no design concern identified in this
section was left unaddressed on its account.

### 2.2 Design Concerns

Six design concerns are identified. Each concern is presented along with
statement, the shareholders that behold it, and the source that grounds it.

The concerns presented are identical to those identified in SAD §2.3. Their
statements are sometimes slightly modified to adapt to the purposes of design,
but are otherwise copied verbatim from the SAD &mdash; even the order is kept
intact.

| ID     | Design concern     | Statement                                                                                                                                                                                                                          | Held by      | Source                               |
|--------|--------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--------------|--------------------------------------|
| DCON-1 | Adaptability       | Adapting the middleware to a target application must only require producing a porting implementation.                                                                                                                              | STK-1        | REQ-USE-01                           |
| DCON-2 | Non-intrusiveness  | The middleware must not require changes to the vendor driver or the board and transport abstraction layer, beyond the documented exceptions (SRS §5.1), and must not claim exclusive ownership of the SPI, DMA and EXTI resources. | STK-1, STK-2 | REQ-DES-02, REQ-USE-03               |
| DCON-3 | Concurrency safety | SPI data transfers must be performed from the invoking thread context only; access to internally shared state must be regulated in a thread-safe manner; and blocking waits must return only on transfer completion or failure.    | STK-2        | REQ-ATTR-01, REQ-ATTR-03, REQ-FUN-37 |
| DCON-4 | Host verifiability | Conformance to the SRS must be demonstrable by the host-executed verification methods of SRS §4, without on-target hardware.                                                                                                       | STK-2        | SRS §4                               |
| DCON-5 | Traceability       | Design claims that address an SRS requirement or architectural element must identify it, so that changes expose the affected views; the design may constrain choices the SRS and SAD leave open, but it may not contradict either. | STK-2        | §1.2                                 |
| DCON-6 | Boundary hygiene   | The middleware must only perform byte transport and transaction framing; all protocol semantics must remain owned by the vendor driver.                                                                                            | STK-2        | SAD §4.7                             |
