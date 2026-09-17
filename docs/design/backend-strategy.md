# Backend Strategy

This document describes CJM's JSON backend strategy.

The [JSON-first extension strategy](json-first-extension-strategy.md) defines
the current product boundary and sequencing. Prefer nlohmann/json reference
integration, the active simdjson generated codec, a future optional Glaze JSON
metadata adapter, and a future C-oriented yyjson compact-DOM binding. Broader
runtime categories below are research taxonomy, not a commitment to support
many libraries. A Glaze custom codec requires a separate product decision and
is not part of the planned metadata adapter.

Glaze metadata must remain an output of CJM IR, not a second IR or a bridge
through which other backends must pass. Extra formats require verified optional
ecosystem support; native YAML/TOML/binary backends are not core deliverables.
The bounded pre-v1.0 C frontend spike is followed by a scope-frozen C MVP with
one JSON runtime binding; completing it is a v1.0 release gate. yyjson is the
preferred candidate, subject to evidence, not a frontend dependency. Glaze
JSON metadata adapter integration keeps its earlier place in the sequence.
Broader C capabilities remain v1.x work; none of this expands current v0.6.

CJM is a build-time code generation tool.

Backends determine what kind of serialization code CJM emits.

---

# Overview

The CJM pipeline is:

```text
C++ Source
    │
    ▼
Parser
    │
    ▼
Semantic Analysis
    │
    ▼
Metadata Model
    │
    ▼
Code Generator
    │
    ▼
Backend-Specific C++
```

The backend is responsible for deciding the shape of generated serialization code.

---

# Initial Backend

The first supported backend is:

```text
nlohmann/json
```

This choice is intentional.

`nlohmann/json` is widely used, easy to understand, and provides a convenient first target for validating the CJM pipeline.

The initial goal is not to compete with JSON libraries.

The initial goal is to prove that CJM can:

- parse C++ metadata
- build a Metadata Model
- generate correct C++ code
- integrate with CMake
- pass tests through generated output

---

# Why Not Start With a Native Backend?

A native backend is valuable.

However, implementing one from the beginning would significantly increase the MVP scope.

It would require CJM to solve two problems at once:

1. Build-time C++ metadata extraction and code generation.
2. JSON parsing and writing infrastructure.

These are both large projects.

The MVP should focus on the CJM pipeline first.

---

# Long-Term Native Backend

Long term, CJM may support a native backend.

A CJM-native backend could reduce dependency risk and improve product
independence.

Potential benefits:

- fewer third-party dependencies
- stable generated API
- tighter integration with CJM metadata
- predictable performance characteristics
- better control over diagnostics
- better cross-platform packaging

This does not mean that the main CJM repository should own a full native JSON
engine.

CJM owns model knowledge and generated model-specific code. A high-performance
JSON runtime owns generic JSON primitives such as scanning, escaping, parsing,
formatting, buffering, fuzzing, and low-level performance work.

The provisional name for a possible independent runtime project is
`cjm-json`. That project does not exist as part of this document, and it must
not become a CJM v1.0 blocker.

See [High-Performance JSON Strategy](high-performance-json-strategy.md) and
[Runtime Backend Program](runtime-backend-program.md) for the backend taxonomy,
repository boundary, benchmark dimensions, runtime work order, and promotion
gates.

---

# Backend Independence

The public CJM workflow should remain stable as backends evolve.

For example:

```cmake
cjm_generate(
    TARGET app
    HEADERS user.hpp
)
```

should remain valid even if the default backend changes in the future.

Backend choices should be expressed as options only when necessary.

Example future API:

```cmake
cjm_generate(
    TARGET app
    HEADERS user.hpp
    JSON_BACKEND nlohmann
)
```

or:

```cmake
cjm_generate(
    TARGET app
    HEADERS user.hpp
    JSON_BACKEND simdjson
)
```

`JSON_BACKEND` is the preferred future shape because runtime JSON backend
selection is different from artifact backend requests such as JSON Schema
generation. v0.6 defines this static selection policy before promoting
additional runtime backends.

See [Static Backend Selection](static-backend-selection.md) for the CLI/CMake
selection contract, dependency isolation rules, generated artifact identity, and
unsupported capability diagnostics.

---

# Generated API Stability

Generated user-facing code should remain as stable as possible.

Backend internals may differ.

But user code should ideally remain simple:

```cpp
nlohmann::json j = value;
```

or, for a native backend:

```cpp
std::string json = cjm::to_json(value);
```

The exact API will be defined as the backend design matures.

---

# Backend Interface

Internally, a backend should consume the Metadata Model.

It should not depend on parser-specific AST nodes.

Conceptually:

```text
Metadata Model

    ↓

Backend

    ↓

Generated C++
```

This preserves the separation between frontend and backend.

---

# MVP Backend Scope

For v0.1, the backend scope is intentionally narrow.

Target:

- generate `to_json`
- generate `from_json`
- support basic scalar fields
- support `std::string`
- target nlohmann/json
- golden-file tests

Out of scope:

- native JSON writer
- custom allocators
- streaming JSON
- SIMD JSON processing
- schema validation
- advanced adapters

---

# Runtime Backend Direction

Future runtime work should distinguish three integration categories.

## Compatibility Backend

Example: `nlohmann/json`.

CJM generates readable library integration around a mature, stable runtime API.
The primary goals are adoption, compatibility, C++17 support, and low
maintenance cost. A compatibility backend remains valuable even when it is not
the fastest available path.

## Metadata Adapter Backend

Example: CJM-generated `glz::meta<T>`.

CJM generates runtime-specific metadata and lets the runtime's native typed
engine own object dispatch, reflection, parsing, and writing. This can remove
duplicate user metadata, but it does not automatically remove the runtime's
template instantiation, compile-time maps, or object lookup generation.

## Generated Codec Backend

Examples include simdjson On-Demand plus builder and a possible Glaze custom
codec experiment.

CJM generates explicit model-specific control flow for field dispatch,
presence, recursive calls, and portable error propagation. The selected runtime
continues to own generic JSON parsing, escaping, number conversion, formatting,
and buffering.

Generated codecs are CJM's preferred high-performance differentiation
hypothesis. They are not required for every runtime. A metadata adapter or
compatibility integration should be retained when it provides better total
engineering value.

Some runtimes may be evaluated in more than one category. In particular, a
Glaze metadata adapter and a Glaze generated custom codec are separate
experiments with different maintenance and build-cost questions.

Other possible runtime shapes include compact document integration through
yyjson and later SAX or state-machine experiments through libraries such as
RapidJSON SAX.

Possible future generated-codec usage:

```cpp
std::string json = cjm::to_json(user);

User user = cjm::from_json<User>(json);
```

or:

```cpp
cjm::json_writer writer;
cjm::write_json(writer, user);
```

These examples are conceptual only.

This should be designed only after field semantics, runtime semantics, and
backend conformance have been defined. A high-performance backend should remain
optional and should not change the default backend or public workflow without
explicit approval.

Generated-codec experiments must be compared with the strongest relevant
runtime-native path. For simdjson this may include documented custom-type
conversion or static reflection, depending on the selected C++ standard and
compiler. For Glaze it includes automatic reflection and explicit metadata.
Different toolchain and semantic requirements must be labeled rather than
treated as equivalent configurations.

CJM should not introduce a universal runtime facade before real backend
implementations prove a small shared abstraction is necessary.

---

# Dependency Policy

Third-party dependencies are acceptable when they accelerate development.

However, core product value should not permanently depend on unnecessary external libraries.

CJM should avoid becoming a thin wrapper around any single JSON library.

The long-term product should be valuable on its own.

Optional runtime dependencies must stay isolated:

- users who do not select a backend should not download it
- users who do not select a backend should not include its headers
- users who do not select a backend should not inherit its C++ standard
  requirement
- backend-specific generated targets may require stricter language standards
  when the selected runtime requires them

---

# Summary

The initial backend optimized for progress.

The post-v0.5 strategy optimizes for shared semantics, explicit backend choice,
and evidence-driven promotion.

CJM froze default field mapping and canonical runtime semantics before beginning
runtime implementation work.

The completed simdjson scalar decode spike validated generated-codec feasibility
without turning CJM itself into an unbounded JSON parser project. Broader support
and promotion still require native baselines, vertical slices, conformance, and
user-consumable integration.
