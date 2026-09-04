# High-Performance JSON Strategy

CJM is a build-time metadata compiler.

Its product center is:

```text
Source model
    |
    v
Frontend
    |
    v
Semantic Analysis
    |
    v
Metadata IR
    |
    v
Backend-specific generated artifacts
```

High-performance JSON support is a backend strategy, not a change to CJM's
identity.

The Metadata IR remains the boundary between source-language understanding and
backend-specific output.

---

# Strategic Decision

CJM should support high-performance JSON through multiple optional backend
strategies.

The CJM repository should not, by default, own a full experimental
high-performance JSON engine.

The governing boundary is:

> CJM owns model knowledge. A JSON runtime owns generic JSON primitives.

CJM should prioritize explicit model-specific codec generation as its strongest
high-performance hypothesis. This is an experiment strategy, not a requirement
that every runtime receive a generated codec and not a present-tense
performance claim.

Each runtime should use the integration form that provides the best total
engineering value:

- compatibility integration when API stability and adoption matter most
- metadata adapters when native typed engines already provide the required
  behavior
- generated codecs when explicit model-specific control flow may add measurable
  semantic, build-cost, diagnostic, or runtime value

Runtime-native typed paths are mandatory comparison baselines. CJM must not
justify a generated codec by assuming that the selected runtime lacks binding
facilities.

For a possible future native engine, the current working repository name is:

```text
cjmworks/cjm-json
```

This name is provisional. The repository does not exist as part of this
decision, and this document does not create or commit to it.

---

# Responsibility Boundary

## CJM Responsibilities

CJM owns:

- source frontends
- semantic analysis
- Metadata IR
- backend selection
- model-specific generated code
- generated model contracts
- JSON Schema generation
- source-aware diagnostics

Generated model-specific code knows:

- record names
- field names
- JSON keys
- field ordering
- field types
- optional semantics
- enum representation
- nested object relationships
- unknown-field policy when documented
- required and default policy when documented

Conceptual generated code might eventually include an in-place helper:

```cpp
bool read_user(
    cjm::json::reader& reader,
    User& output,
    cjm::json::error& error);

void write_user(
    cjm::json::writer& writer,
    const User& value);
```

These examples are conceptual only. They do not define a public API.

The default public decode API should prefer returning a new object so failed
decodes do not expose a partially mutated caller-owned object:

```cpp
auto result = cjm::decode<User>(json_input);
```

An in-place API may exist later, but it must document whether it provides a
strong guarantee or only leaves the target object valid after failure.

## JSON Runtime Responsibilities

A runtime such as a possible `cjm-json` owns:

- JSON syntax primitives
- structural scanning
- output buffers
- writer primitives
- scanner or tokenizer logic
- scalar parsing and formatting
- string escaping and unescaping
- error positions
- depth and size limits
- allocation and buffer hooks
- optimized SIMD or SWAR kernels
- fuzzing
- differential testing
- performance benchmarks

It must not depend on:

- CJM parser nodes
- Tree-sitter nodes
- CJM semantic analysis classes
- Metadata IR implementation types
- CJM private generator headers
- CJM CMake internals

The intended dependency direction is:

```text
CJM Metadata IR
    |
    v
CJM backend
    |
    v
generated model-specific codec
    |
    v
public JSON runtime API
```

The dependency direction must not reverse.

---

# Avoid a DOM-First Native Identity

The initial native high-performance direction should not start as:

```cpp
cjm::json::value document = cjm::json::parse(input);
```

A full mutable DOM expands scope into:

- dynamic object storage
- iterators
- mutation
- JSON Pointer
- JSON Patch
- generic conversions
- allocator-aware trees
- arbitrary document editing

The stronger CJM-aligned direction is:

> Generated high-performance typed JSON codecs for known C and C++ models.

A future DOM may be explored only if independently justified.

---

# Backend Taxonomy

This taxonomy is directional. It is not a commitment to implement every backend.
Each backend requires independent product justification.

## Artifact Backends

Examples:

- generated model contract
- JSON Schema

CJM generates metadata or schema artifacts. No runtime JSON parsing or writing
library is involved.

Artifact backends consume Metadata IR field facts such as C++ name, effective
JSON name, ignored status, type category, source location, and enum values.

## Compatibility And Document Backends

Examples:

- `nlohmann/json`
- yyjson

Conceptual flow:

```text
JSON text
    |
    v
generic JSON document / value layer
    |
    v
CJM-generated model binding
    |
    v
C++ object
```

`nlohmann/json` is CJM's official compatibility backend. It remains valuable for
adoption, readable generated code, and C++17-friendly workflows.

yyjson is a compact document / DOM candidate. It is valuable as a high-
performance DOM control group and possible C-compatible runtime, but it should
not be described as a no-DOM or direct-typed backend.

## Metadata Adapter Backends

Examples:

- Glaze
- DAW JSON Link

Conceptual flow:

```text
Metadata IR
    |
    v
generated runtime-specific metadata or contract
    |
    v
mature typed runtime
    |
    v
JSON bytes <-> C++ object
```

These backends let CJM remove duplicate user metadata while preserving the
runtime library's typed read/write path. They do not automatically remove the
runtime's template instantiation, compile-time reflection, lookup-table
generation, or recursive typed-engine instantiation.

Glaze is a strong optional candidate but currently raises C++ standard concerns
for selected backend targets. That requirement must not raise CJM core or
nlohmann users to C++23.

DAW JSON Link is a possible C++17 direct-typed candidate. It should enter only a
time-boxed spike until contract semantics, maintenance risk, and error behavior
are understood.

## Generated Codec Backends

Examples:

- simdjson On-Demand plus builder
- a possible Glaze custom codec
- a possible yyjson model binding

Conceptual decode flow:

```text
JSON text
    |
    v
simdjson On-Demand iterator
    |
    v
CJM-generated field dispatch and recursive decode
    |
    v
C++ object
```

This is CJM's preferred first high-performance runtime experiment because it
tests CJM's own generated-codec value directly.

The generated codec owns model-specific behavior such as field dispatch,
presence tracking, optional/null policy, recursive model calls, and structured
error propagation. The runtime continues to own structural parsing, string
unescaping, numeric conversion, output buffering, escaping, and formatting.

The first implementation proved decode over a limited scalar conformance subset.
It established one-pass field dispatch, new-object output, required-field
tracking, numeric range checks, portable errors, and trailing-content rejection.
It did not establish broader type coverage or a performance result.

The prerequisite set completed before the spike was:

- runtime JSON semantic profile
- minimal decode error and structured path model
- conformance fixture skeleton
- static backend selection shape
- [simdjson On-Demand decode spike boundary](simdjson-ondemand-decode-spike.md)

The next simdjson work should preserve this context while comparing relevant
runtime-native typed-conversion paths and extending the generated decoder through
owned strings, optional presence, and one nested model. Decode should then proceed
to a limited MVP, followed contiguously by a builder/write encode spike.

simdjson-specific constraints must be documented before promotion:

- input buffer and padding lifetime
- parser and document lifetime
- forward-only value consumption
- nested object and array consumption order
- copy versus borrow policy for strings
- unknown-field validation behavior
- partial-output policy after errors

The reason to begin with simdjson is not that simdjson lacks typed conversion.
The reason is that its forward-consumption model makes generated control flow,
lifetime ownership, field dispatch, and portable error translation directly
measurable.

## Runtime-Native Baselines

Generated integrations must be compared with the strongest relevant native
path supported by the pinned runtime release.

For simdjson, candidate baselines include:

- handwritten On-Demand field traversal
- documented pre-C++20 `get<T>` specialization
- documented C++20 `tag_invoke` customization
- experimental C++20 `simdjson::from`, when useful as a labeled comparison
- C++26 static-reflection conversion when a suitable compiler is available

For Glaze, candidate baselines include:

- automatic reflection
- handwritten explicit `glz::meta<T>`
- handwritten custom serialization

The C++ standard, compiler, runtime version, semantic options, and error
behavior of every baseline must be recorded. A C++17 generated path and a C++26
reflection path are useful comparisons, but they are not the same toolchain
contract.

## Glaze Integration Modes

Glaze requires two independent evaluations.

The metadata-adapter evaluation asks whether CJM-generated `glz::meta<T>`
removes meaningful user maintenance while preserving Glaze's native typed
engine.

The generated-custom-codec evaluation asks whether documented Glaze `from/to`
and `parse/serialize` customization points can support explicit CJM-generated
control flow with acceptable build cost, runtime performance, readability, and
upgrade cost.

The second experiment is optional. A valid outcome is to keep only the metadata
adapter because native reflection is sufficient or the lower-level integration
surface is too costly to maintain.

Current primary-source snapshot:

- simdjson v4.6.4 documents custom types and reflection in its
  [versioned basics guide](https://github.com/simdjson/simdjson/blob/v4.6.4/doc/basics.md)
  and custom encoding in its
  [versioned builder guide](https://github.com/simdjson/simdjson/blob/v4.6.4/doc/builder.md)
- Glaze documents [pure reflection](https://stephenberry.github.io/glaze/pure-reflection/),
  [custom serialization](https://stephenberry.github.io/glaze/custom-serialization/),
  and [C++23 toolchain requirements](https://stephenberry.github.io/glaze/installation/)

Glaze v7.5.0 is the current inspection reference, not a selected CJM
dependency. A future G0 evaluation must pin an exact release and reclassify the
API surface against matching source and documentation.

## SAX / State-Machine Backends

Example:

- RapidJSON SAX

Conceptual flow:

```text
JSON parser events
    |
    v
CJM-generated state machine
    |
    v
C++ object
```

This path can avoid a DOM, but nested objects, arrays, unknown-field skipping,
partial cleanup, and error paths make it a poor first runtime backend. It is a
possible later portability or low-level baseline.

## cjm-json Native Runtime Research

A possible `cjm-json` runtime remains separate optional research. The CJM
repository should not implement a scanner, parser, formatter, generic DOM,
number parser, or SIMD structural scanner merely to support runtime backend
work.

---

# Runtime API Stability Policy

Every runtime API used by generated code or a backend support layer must be
classified as one of:

- documented public API
- documented extension or customization API
- callable public-header implementation helper without a compatibility promise
- internal implementation detail

Official integrations should use documented public or extension APIs. A pinned
research spike may isolate a weaker helper behind one backend-specific support
layer, but internal APIs must not leak into Metadata IR, CJM core, or generated
public contracts.

Pinning a runtime version makes the experiment reproducible; it does not make
an internal API stable. Each backend evaluation must record the runtime version,
headers and APIs used, C++ and compiler requirements, tested compatibility
window, upgrade procedure, and fallback plan.

---

# No Universal Runtime Facade

CJM should not introduce a generic runtime interface such as:

```cpp
class IJsonRuntime;
class IJsonReader;
class IJsonWriter;
class IJsonValue;
```

The candidate libraries have different optimal integration models:

- Glaze and DAW use direct typed metadata or contracts
- simdjson On-Demand uses forward-only generated binding
- yyjson uses document/value ownership
- RapidJSON SAX uses push-event state machines

CJM should share:

- Metadata IR semantics
- backend-neutral runtime semantic profile
- capability matrix
- conformance fixtures
- generated diagnostics
- benchmark methodology

Code-level runtime abstractions should be extracted only after at least two real
runtime backends prove a small shared helper is necessary.

Backend selection should be static: generation time or build time, not dynamic
runtime selection through a universal interface.

Conceptual CLI and CMake shapes:

```bash
cjm generate --backend simdjson --input models.hpp --output models.simdjson.cjm.hpp
```

```cmake
cjm_generate(
  TARGET app
  HEADERS models.hpp
  JSON_BACKEND simdjson
)
```

The exact spelling is future work, but the policy is not: backend-specific
dependencies, generated filenames, C++ standard requirements, and unsupported
capabilities must be resolved before generated code is compiled.

The detailed static selection contract lives in
[Static Backend Selection](static-backend-selection.md).

---

# Possible cjm-json Development Sequence

This sequence is for a separate experimental project if the owner later creates
one.

It is not a CJM product roadmap requirement.

## Phase 0 - Research and Benchmark Design

No parser implementation.

Define:

- target workloads
- correctness requirements
- baseline libraries
- compiler and hardware matrix
- validation equivalence rules
- measurement methodology
- stop criteria

## Phase 1 - Direct Writer

Scope:

- generated model-specific JSON writer
- output buffer
- bool
- integers
- floating point
- strings
- escaping
- nested records
- arrays
- optional fields
- enums
- buffer reuse

Why start here:

- smaller state space
- easier correctness model
- easier differential testing
- clear performance measurement
- strong learning value

## Phase 2 - Generated Typed Binder over a Mature Parser

Conceptual flow:

```text
mature structural parser
    |
    v
generated field dispatch and typed assignment
    |
    v
C or C++ model
```

Purpose:

- measure generated typed binding before writing a native parser
- separate model-dispatch value from runtime parsing value

## Phase 3 - Native Scanner and Scalar Primitives

Possible scope:

- structural scanning
- whitespace
- strings
- escapes
- integers
- floating point
- nesting
- key dispatch

## Phase 4 - Native Typed Reader

Only begin after earlier stages show worthwhile performance potential.

Require:

- malformed-input correctness
- range checking
- UTF-8 policy
- unknown-field skipping
- depth limits
- size limits
- structured errors
- partial-object cleanup
- allocation policy

## Phase 5 - Stable Engine Evaluation

Only consider stable public release after:

- fuzzing
- sanitizers
- differential tests
- cross-platform CI
- public benchmark methodology
- stable error model
- documented supported JSON surface
- real downstream dogfood

---

# Benchmark Dimensions

Runtime metrics may include:

- write throughput
- read throughput
- roundtrip time
- p50 latency
- p95 latency
- p99 latency
- allocations
- bytes allocated
- branch misses where practical
- cache misses where practical
- instructions per byte
- cycles per byte
- binary size

Build-cost metrics may include:

- clean Debug build
- clean Release build
- incremental build
- peak compiler memory
- generated source size
- template-instantiation cost
- link time

Correctness equivalence must record whether each configuration validates:

- UTF-8
- number ranges
- invalid escapes
- malformed numbers
- unknown fields
- missing fields
- null
- duplicate keys
- nesting depth
- trailing content

A permissive fast path must not be compared against a strict validated path
without labeling the difference.

Relevant native baselines are mandatory for the runtime under evaluation.

The simdjson experiment should distinguish:

- handwritten On-Demand traversal
- native custom-type conversion
- experimental convenience conversion when relevant
- C++26 reflection when the compiler is available
- CJM-generated On-Demand codec

The Glaze experiment should distinguish:

- automatic reflection
- handwritten explicit metadata
- CJM-generated metadata
- handwritten custom codec
- CJM-generated custom codec

Additional control groups may include handwritten and CJM-generated
`nlohmann/json`, yyjson plus binding, RapidJSON, reflect-cpp, or future
`cjm-json`. This document does not require unrelated runtime baselines before a
focused experiment can continue.

---

# Correctness and Security Gates

A native engine must not become an official CJM backend solely because it is
fast.

Minimum gates include:

- documented supported JSON standard surface
- differential testing against mature libraries
- property-based tests
- fuzzing
- ASan
- UBSan
- integer overflow tests
- floating-point edge cases
- UTF-8 and escape tests
- deep nesting limits
- large-input limits
- malformed-input corpus
- stable error locations
- no partial output leakage
- failure cleanup
- cross-platform testing

For C decode support, also require:

- allocation ownership
- partial initialization cleanup
- failure rollback
- buffer capacity policy
- string lifetime
- sequence lifetime
- caller-owned versus runtime-owned memory

Performance never overrides correctness or explicit ownership.

---

# Integration Promotion Stages

## Stage A - External Experiment

Properties:

- separate repository
- no CJM dependency
- no official CJM support
- unstable APIs
- research and benchmark focus

## Stage B - Experimental CJM Backend

Promotion requires:

- public runtime API
- stable enough generated-code boundary
- fuzzing and sanitizers
- differential correctness
- bounded supported surface
- demonstrable product value
- optional integration
- no default-behavior changes

## Stage C - Official CJM Backend

Promotion requires:

- stable release
- compatibility policy
- documented support matrix
- cross-platform CI
- real downstream use
- maintainable annual cost
- clear benefit over mature alternatives

Creating a repository does not automatically trigger promotion.

## Stop Or Narrow Decision

A generated-codec experiment should stop or narrow when it depends broadly on
implementation-detail APIs, duplicates generic parser logic, cannot preserve
CJM semantics safely, shows negligible build-cost benefit, materially regresses
runtime performance or code size, or costs substantially more to maintain than
the runtime's best native typed path.

Valid outcomes include retaining only a metadata adapter, keeping one exact
version experimental, documenting a negative result, deferring the work, or
rejecting the backend.

---

# Version Relationship

Before broad runtime backend work, CJM completed the v0.5.x semantic foundation:

- default field mapping
- effective JSON field-name normalization
- explicit ignored-field semantics
- fail-closed unsupported included fields

The v0.6 runtime foundation and first implementation spike are also complete:

- canonical runtime semantic profile
- minimal decode error and structured path model
- backend taxonomy and capability matrix
- shared conformance fixture skeleton
- [static backend selection design](static-backend-selection.md)
- simdjson On-Demand decode spike

Remaining v0.6 work may continue with:

- simdjson native typed-conversion baselines
- meaningful generated-codec vertical slice
- simdjson decode MVP over a limited conformance subset
- simdjson builder / encode spike
- simdjson experimental backend if evidence supports promotion

Later v0.6 work may evaluate:

- Glaze metadata generation as an optional adapter backend
- Glaze custom codec generation as a separate, stoppable experiment
- yyjson as a compact document / DOM candidate
- DAW JSON Link as a time-boxed direct-typed C++17 spike
- RapidJSON SAX as a lower-priority state-machine experiment

Not allowed by default:

- native parser implementation in the CJM repository
- making cjm-json a required dependency
- making cjm-json a v1.0 blocker
- changing default runtime behavior
- expanding the v1.0 mapping matrix solely for cjm-json
- promoting any runtime backend based on speed alone

CJM v1.x may explore:

- promoting experimental runtime backends
- additional backend-specific capability expansion
- cjm-json generated-code boundary experiments if independent evidence supports
  the work
- performance and build-time comparisons

These are evidence-driven options.

A future C direction must not depend on cjm-json being complete.

CJM v2.0 may still succeed with first-class C and C++ frontends, shared
Metadata IR, language-neutral schema output, and explicit backend performance
choices even if cjm-json remains experimental.

Future binary representation experiments are separate from the active JSON
runtime backend program. See [Binary Format Strategy](binary-format-strategy.md)
for the distinction between binary JSON-like formats, tagged wire protocols,
borrowed view semantics, and serialized layouts.

---

# Product Messaging Boundary

CJM should not be marketed as:

- the fastest JSON library
- a replacement for Glaze
- a replacement for yyjson
- a native JSON parser

A more accurate product principle is:

> One source model. One validated Metadata IR. Multiple integration and
> performance choices.

Do not add speculative performance claims without evidence.

Codec-first describes the preferred experiment order. It does not mean that a
generated codec has already proved superior or that every evaluated runtime
must become an official backend.
