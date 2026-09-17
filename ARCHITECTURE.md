# CJM Architecture

This document describes the high-level architecture of CJM.

It focuses on the responsibilities of each component rather than implementation details.

Specific parser implementations, code generation algorithms, and internal optimizations are documented separately.

                Frontend
────────────────────────────────────

Parser

↓

AST

↓

Semantic Analysis

────────────────────────────────────

IR

↓

Metadata Model

────────────────────────────────────

Backend

↓

Code Generator

↓

Generated C++

────────────────────────────────────

Toolchain

↓

CMake

↓

Compiler

↓

Linker

---

# Overview

CJM is a build-time code generator.

Its purpose is to transform standard C++ source code into additional standard C++ source code.

The generated code is then compiled by the user's normal compiler.

```
        User Source Code
               │
               ▼
      Metadata Extraction
               │
               ▼
       Intermediate Model
               │
               ▼
        Code Generation
               │
               ▼
      Generated C++ Source
               │
               ▼
      User's C++ Compiler
               │
               ▼
          Final Binary
```

Users interact only with:

- Standard C++
- CMake
- Generated C++ code

Everything else is an implementation detail.

---

# Design Goals

The architecture is designed around the following principles:

- Build-time execution
- Standard C++ input
- Standard C++ output
- Clear separation of responsibilities
- Replaceable implementation components
- Deterministic code generation

---

# Pipeline

CJM consists of four logical stages.

## 1. Metadata Extraction

Input:

```
C++ source files
```

Responsibility:

- Parse source files
- Discover annotated declarations
- Extract user-defined metadata
- Resolve symbols when necessary

Output:

```
Metadata Model
```

The parser implementation is intentionally hidden behind this stage.

---

## 2. Intermediate Model

The Metadata Model is the internal representation used by CJM.

It contains information required for code generation, such as:

- types
- fields
- namespaces
- templates
- metadata
- relationships

These are examples of possible model information.
They are not all required for the v0.1 Metadata Model.

The model is independent of the parser implementation.

This separation allows the parser to evolve without affecting the generator.

The Metadata Model serves as the intermediate representation (IR) of CJM.

All parser implementations produce it.

All generators consume it.

Parser implementations and generators must never directly depend on each other.

---

## 3. Code Generation

The generator transforms the Metadata Model into ordinary C++ source code.

Responsibilities include:

- serialization code
- deserialization code
- helper utilities
- generated headers

The generator should produce deterministic and readable output.

---

## 4. Build Integration

Generated files become normal compilation units.

The user's compiler is responsible for:

- compilation
- optimization
- linking

CJM does not replace the compiler.

It extends the build process.

---

# Component Responsibilities

```
+-----------------------+
| Metadata Extraction   |
+-----------------------+
| Parse source          |
| Collect metadata      |
+-----------+-----------+
            |
            ▼
+-----------------------+
| Metadata Model        |
+-----------------------+
| Internal structures   |
| Type relationships    |
| Generation context    |
+-----------+-----------+
            |
            ▼
+-----------------------+
| Code Generator        |
+-----------------------+
| Generate C++          |
| Generate helpers      |
+-----------+-----------+
            |
            ▼
+-----------------------+
| Build Integration     |
+-----------------------+
| CMake                 |
| Incremental build     |
+-----------------------+
```

---

# Why an Intermediate Model?

The Metadata Model separates parsing from generation.

Without this layer:

```
Parser
    ↓
Generator
```

both become tightly coupled.

Instead:

```
Parser
      ↓
Semantic Analysis 
      ↓
Metadata Model
      ↓
Generator
```

Each stage has a single responsibility.

Future parser implementations can reuse the same generator.

Future generators can reuse the same metadata model.

---

# Parser Independence

Parsing is an implementation detail.

The public architecture does not require any specific parser.

Possible implementations include:

- Clang
- Future parser implementations
- Other tooling

As long as the Metadata Model remains stable, the rest of the architecture remains unchanged.

---

# Generated Code

Generated files are ordinary C++.

They should be:

- deterministic
- readable
- inspectable
- debuggable

Generated files are considered part of the build output rather than hidden implementation artifacts.

---

# Build System

CJM integrates into existing CMake workflows.

A simplified build process is:

```
configure

        │

        ▼

cjm_generate()

       ├──> Generate *.cjm.hpp
       │
       │        ↓
       │
       │    Normal C++ compilation
       │
       │        ↓
       │
       │    Executable
       │
       └──> Optionally generate *.schema.json
```

Users continue using their existing compiler and build system.

---

# Extensibility

The architecture intentionally separates:

- parsing
- metadata
- generation
- build integration

Future features should be implemented by extending existing stages rather than introducing cross-cutting dependencies.

This keeps the architecture maintainable as the project grows.

---

# Long-term Architecture Vision

CJM is evolving from a JSON code generator into a build-time metadata
compiler.

## Compiler Layers

The long-term architecture separates the project into three independent
layers:

```text
Source-language Frontend
        ↓
Language-neutral Metadata IR
        ↓
Code-generation Backend
```

Frontends parse source languages and perform source-language semantic
analysis.

The Metadata IR is a language-neutral intermediate representation describing
user-defined types, fields, relationships, and metadata.

Backends generate integrations for different libraries, runtimes, or
programming languages from the Metadata IR.

The Metadata IR is the stable contract between source-language understanding
and backend-specific code generation.

## Current Implementation

The current implementation is:

```text
C++ frontend
        │
        ▼
Metadata IR
   │             │
   ▼             ▼
nlohmann/json    JSON Schema
Backend          Backend
```

The current implementation has one C++ frontend and two Metadata IR consumers:
the `nlohmann/json` C++ backend and the JSON Schema artifact backend.

## Dependency Direction

Allowed dependencies:

```text
frontends ─┐
           ├──> core
backends ──┘
```

Rules:

- Core must not depend on frontends or backends.
- Frontends may depend on Core.
- Backends may depend on Core.
- Frontends and backends must not depend on each other.
- Backends consume Metadata IR only and must not inspect frontend syntax.

## Possible Future Extensions

CJM remains JSON-first. Prefer nlohmann/json compatibility/reference integration,
the simdjson generated codec, an optional Glaze JSON metadata adapter, and yyjson
as a future C-oriented compact-DOM candidate. Candidate status is not support.

Glaze metadata is an output artifact, never a second canonical IR or an
intermediate representation consumed by other backends. Core and Semantic
Analysis must not depend on Glaze. Extra ecosystem formats do not become
CJM-native product scope.

A bounded C frontend research spike before v1.0 should validate:

```text
C++ source -> C++ semantic normalization --+
                                           +-> shared Metadata IR -> JSON backends
C source   -> C semantic normalization ----+                      -> artifacts
              (future research)
```

Current IR reuse is a hypothesis to test with real C DTOs, not a reason to
rewrite it preemptively. C-specific pointer/length facts may need temporary
semantic normalization; backends should receive canonical meaning and source
bindings needed for generation, not parser nodes or yyjson facts. C/C++ share
JSON semantics, not necessarily generated APIs or ownership models.

After the spike, freeze and implement a bounded C MVP with one JSON binding,
explicit ownership/error contracts, build integration, and conformance. Its
completion is a v1.0 release condition alongside stable C++ JSON codegen.
Keep Glaze JSON adapter integration in the preceding plan; it consumes current
IR and does not wait for C. Evidence-backed IR changes must migrate and test
affected backends together. C capabilities beyond the frozen MVP remain v1.x
work, not implied release requirements. See the
[JSON-first extension strategy](docs/design/json-first-extension-strategy.md)
for experiment limits, ownership questions, and IR migration gates.

## Non-goals

CJM is not intended to become:

- a universal serialization library
- a runtime reflection system
- a replacement for the C++ compiler
- a commitment to implement every possible frontend or backend

---

## CJM prioritizes:

1. Correctness
2. User experience
3. Maintainability
4. Predictable generated code
5. Performance

---

# Summary

The CJM architecture follows a simple philosophy:

```
Standard C++ in.

Standard C++ out.
```

Everything between those two points is CJM's responsibility.

Users should never need to understand the internal implementation in order to use the tool effectively.
