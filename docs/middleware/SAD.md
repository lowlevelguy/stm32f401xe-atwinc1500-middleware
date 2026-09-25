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
| 0.1     | 2026-09-20 | Initial draft (Section 1). |
| 0.2     | 2026-09-22 | Added Section 2.           |
| 0.3     | 2026-09-25 | Added Section 3.           |

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
  in §2.

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
"user-defined" &mdash; is defined once in SRS §1.4 and is not repeated
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

## 2. Stakeholders, Perspectives, Concerns and Aspects

This section identifies the stakeholders of the middleware architecture, their
perspectives, the concerns they hold, and the architectural aspects said
concerns relate to. The identifiers introduced here are used throughout §3, §5
and §6.

### 2.1 Stakeholders

Two stakeholders are identified; they are listed below, together with the
impact of the architecture on each.

| ID    | Role                    | Description                                                                                                                                                                                                                                                                                              |
|-------|-------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| STK-1 | Integrator              | Authors the user application and the porting implementation, and consumes the middleware through the vendor driver. SRS §1.3.1.1 defines the same role as the user of the middleware. The integrator is kept distinct from the maintainer in principle, as the role for which the middleware is written. |
| STK-2 | Maintainer and verifier | The middleware's author, who evolves it against future changes of the SRS and executes the verification methods of SRS §4 &mdash; analysis, inspection and host-executed unit tests.                                                                                                                     |

The architecture impacts its stakeholders as follows. For the integrator, it
confines application-specific behaviour to the porting implementation, so
that the board and transport abstraction layer need never be modified
(REQ-USE-01). For the maintainer, it commits the middleware to the ownership
boundary of the ownership-boundary view (§4.8) and to the host-executed
verification methods of SRS §4; both constrain how the middleware may evolve.

The roles above are discharged, in this project, by a single individual; the
integrator role is kept distinct in principle, as it is the role for which
the middleware is written. The single-author situation is the principal
resource limitation of the architecting effort; no concern identified in this
section was left unaddressed on its account.

### 2.2 Perspectives

Three perspectives are identified. 

| ID    | Perspective  | Held by | Concerns            |
|-------|--------------|---------|---------------------|
| PER-1 | Integration  | STK-1   | CON-1, CON-2        |
| PER-2 | Verification | STK-2   | CON-4, CON-5        |
| PER-3 | Maintenance  | STK-2   | CON-3, CON-5, CON-6 |

The verification and maintenance perspectives share a holder &mdash; the
maintainer and the verifier are one and the same (STK-2) &mdash; but remain
distinct: the former asks how the architecture is demonstrated to satisfy the
SRS, the latter how it can evolve without eroding its boundaries.

### 2.3 Architecture Concerns

Six concerns are identified. Each concern traces to the SRS requirement that
raises it; where a concern is grounded in this document instead, its source is
cited directly.

| ID    | Concern            | Statement                                                                                                                                                                                                                          | Held by      | Source                               |
|-------|--------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--------------|--------------------------------------|
| CON-1 | Adaptability       | Adapting the middleware to a target application must only require producing a porting implementation.                                                                                                                              | STK-1        | REQ-USE-01                           |
| CON-2 | Non-intrusiveness  | The middleware must not require changes to the vendor driver or the board and transport abstraction layer, beyond the documented exceptions (SRS §5.1), and must not claim exclusive ownership of the SPI, DMA and EXTI resources. | STK-1, STK-2 | REQ-DES-02, REQ-USE-03               |
| CON-3 | Concurrency safety | SPI data transfers must be performed from the invoking thread context only; access to internally shared state must be regulated in a thread-safe manner; and blocking waits must return only on transfer completion or failure.    | STK-2        | REQ-ATTR-01, REQ-ATTR-03, REQ-FUN-37 |
| CON-4 | Host verifiability | Conformance to the SRS must be demonstrable by the host-executed verification methods of SRS §4, without on-target hardware.                                                                                                       | STK-2        | SRS §4                               |
| CON-5 | Traceability       | Every architecture element of this document must trace to the SRS, so that a change of requirements exposes its architectural consequences.                                                                                        | STK-2        | §1.2                                 |
| CON-6 | Boundary hygiene   | The middleware must only perform byte transport and transaction framing; all protocol semantics must remain owned by the vendor driver.                                                                                            | STK-2        | §4.8                                 |

### 2.4 Aspects

Three aspects are identified. No functional aspect is identified, since all
concerns are the province of SRS §1.2; their architectural answers are given
under the structural and behavioural aspects.

| ID    | Aspect       | Concerns            | Description                                                                                                 |
|-------|--------------|---------------------|-------------------------------------------------------------------------------------------------------------|
| ASP-1 | Structural   | CON-1, CON-2, CON-6 | The decomposition of the middleware into parts, and the boundaries it keeps with its environment.           |
| ASP-2 | Behavioural  | CON-3, CON-4        | The interaction and dynamic behaviour of those parts over time, exercised on the host through mocked seams. |
| ASP-3 | Programmatic | CON-5               | How the architecture is expressed and checked as source artefacts: build, tests and traces.                 |

Every concern of §2.3 is covered by at least one aspect. The viewpoints of §3
are formulated over these aspects, and the correspondence between concerns and
views is recorded in §5.

## 3. Architecture Viewpoints

This section specifies the three architecture viewpoints that govern the views
of §4. Each specification states the views it governs, its version, the
stakeholder perspectives associated with it, the concerns it frames together
with their aspects and holders, its model kinds, its correspondence methods,
and its sources. The model kinds are defined once in §3.4 and are
forward-referenced by the viewpoints.

The three viewpoints are not arbitrary; they are derived from the concerns of
Section 2.3 and the questions they raise. Likewise, the model kinds used are
aligned to the needs of the aspects of architecture.

### 3.1 Context and boundaries

This viewpoint considers the middleware as a bounded whole &mdash; what lies
within it, what lies outside it, and where ownership passes. It governs the
context (§4.1) and ownership-boundary (§4.8) views.

| Field                    | Specification                                                                                                                                                                          |
|--------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Version                  | 1.0                                                                                                                                                                                    |
| Stakeholder perspectives | PER-1 Integration; PER-2 Verification; PER-3 Maintenance                                                                                                                               |
| Concerns framed          | CON-2 Non-intrusiveness; CON-5 Traceability; CON-6 Boundary hygiene                                                                                                                    |
| Aspects                  | ASP-1 Structural; ASP-3 Programmatic                                                                                                                                                   |
| Known stakeholders       | STK-1 Integrator; STK-2 Maintainer and verifier                                                                                                                                        |
| Model kinds              | Block diagram (§3.4.1)                                                                                                                                                                 |
| Correspondence methods   | Boundary consistency, by inspection: the middleware boundary drawn in each view of this viewpoint is one and the same. Concern traceability: elements of a view trace to the SRS (§5). |
| Sources                  | The standard, Clause 8 and Annex B; SRS §3.4.1 for the hardware interfaces at the boundary.                                                                                            |

### 3.2 Static structure

This viewpoint considers the middleware as parts and the contracts between
them, time abstracted away. It governs the structure and interfaces view (§4.2)
and the internal decomposition view (§4.3).

| Field                    | Specification                                                                                                                                                                                             |
|--------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Version                  | 1.0                                                                                                                                                                                                       |
| Stakeholder perspectives | PER-1 Integration; PER-2 Verification; PER-3 Maintenance                                                                                                                                                  |
| Concerns framed          | CON-1 Adaptability; CON-2 Non-intrusiveness; CON-5 Traceability                                                                                                                                           |
| Aspects                  | ASP-1 Structural; ASP-3 Programmatic                                                                                                                                                                      |
| Known stakeholders       | STK-1 Integrator; STK-2 Maintainer and verifier                                                                                                                                                           |
| Model kinds              | Block diagram (§3.4.1)                                                                                                                                                                                    |
| Correspondence methods   | Interface consistency, by inspection: the porting interface appears in §4.2 and §4.3 as one and the same contract, defined by SRS §3.4.2. Concern traceability: elements of a view trace to the SRS (§5). |
| Sources                  | The standard, Clause 8 and Annex B; SRS §3.4 for the interface inventory.                                                                                                                                 |

### 3.3 Interaction and dynamics

This viewpoint considers the middleware as behaviour ordered in time &mdash;
invocations, notifications and state changes, in thread and interrupt context.
It governs the bring-up (§4.4), SPI transaction (§4.5), event delivery (§4.6)
and module power lifecycle (§4.7) views.

| Field                    | Specification                                                                                                                                                                                                                                                                   |
|--------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Version                  | 1.0                                                                                                                                                                                                                                                                             |
| Stakeholder perspectives | PER-2 Verification; PER-3 Maintenance                                                                                                                                                                                                                                           |
| Concerns framed          | CON-3 Concurrency safety; CON-4 Host verifiability; CON-5 Traceability                                                                                                                                                                                                          |
| Aspects                  | ASP-2 Behavioural; ASP-3 Programmatic                                                                                                                                                                                                                                           |
| Known stakeholders       | STK-2 Maintainer and verifier                                                                                                                                                                                                                                                   |
| Model kinds              | Sequence diagram (§3.4.2); state machine diagram (§3.4.3)                                                                                                                                                                                                                       |
| Correspondence methods   | Context consistency, by inspection: the thread or interrupt annotation of each interaction agrees across the views of this viewpoint. Behaviour traceability: every interaction traces to an SRS requirement, and to a host-executed verification method where one exists (§5). |
| Sources                  | The standard, Clause 8 and Annex B; SRS §3.1.3 for synchronisation and bus ownership; the WINC1500 API reference for call semantics.                                                                                                                                            |

### 3.4 Model kinds

The model kinds used by the viewpoints above are specified here once. The
conventions of a model kind are the legend for every view component of that
kind; the reading notes of §4 add view-specific guidance but do not override
them.

#### 3.4.1 Block diagram

| Field        | Specification                                                                                                                                                                                                                                                                                                                                                                                                |
|--------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Conventions  | A rectangle represents an entity &mdash; a part of the middleware, a layer of its environment, or an external system; nested rectangles represent its decomposition. An «interface» rectangle attached by a line represents a contract; the reading note of each view names the relation a connection encodes (provides, consumes, requires or implements). Colour encodes nothing; it is presentation only. |
| View methods | Interpreted by inspection; no formal analysis method is defined.                                                                                                                                                                                                                                                                                                                                             |
| Version      | 1.0                                                                                                                                                                                                                                                                                                                                                                                                          |
| Sources      | Informal conventions adapted from UML 2.x.                                                                                                                                                                                                                                                                                                                                                                   |

#### 3.4.2 Sequence diagram

| Field        | Specification                                                                                                                                                                                                                                                |
|--------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Conventions  | A lifeline represents an entity, annotated with the execution context &mdash; thread or interrupt &mdash; in which its events occur. An arrow between lifelines represents an invocation or notification, named by its label; time flows from top to bottom. |
| View methods | Interpreted by inspection; no formal analysis method is defined.                                                                                                                                                                                             |
| Version      | 1.0                                                                                                                                                                                                                                                          |
| Sources      | Informal conventions adapted from UML 2.x.                                                                                                                                                                                                                   |

#### 3.4.3 State machine diagram

| Field        | Specification                                                                                                                                                                                                   |
|--------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Conventions  | A rounded rectangle represents a state of the entity; an arrow represents a transition, labelled with its trigger &mdash; a call, a pin level or an internal condition. An entry arrow marks the initial state. |
| View methods | Interpreted by inspection; no formal analysis method is defined.                                                                                                                                                |
| Version      | 1.0                                                                                                                                                                                                             |
| Sources      | Informal conventions adapted from UML 2.x.                                                                                                                                                                      |

### 3.5 Coverage

Every concern is framed by at least one viewpoint, and every perspective is
associated with the viewpoints covering it; the matrix below records both, with
a row per concern and per perspective.

|       | 3.1 Context and boundaries | 3.2 Static structure | 3.3 Interaction and dynamics |
|-------|----------------------------|----------------------|------------------------------|
| CON-1 | &mdash;                    | yes                  | &mdash;                      |
| CON-2 | yes                        | yes                  | &mdash;                      |
| CON-3 | &mdash;                    | &mdash;              | yes                          |
| CON-4 | &mdash;                    | &mdash;              | yes                          |
| CON-5 | yes                        | yes                  | yes                          |
| CON-6 | yes                        | &mdash;              | &mdash;                      |
| PER-1 | yes                        | yes                  | &mdash;                      |
| PER-2 | yes                        | yes                  | yes                          |
| PER-3 | yes                        | yes                  | yes                          |