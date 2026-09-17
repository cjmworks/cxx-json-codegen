# Binary Format Strategy

Status update, 2026-09-16: retained as historical, uncommitted research notes.
The [JSON-first extension strategy](json-first-extension-strategy.md) supersedes
the broader product direction and staged roadmap below. CJM does not currently
plan native YAML/TOML/BEVE/CBOR/MessagePack backends or a wire platform. Glaze is
first a JSON metadata adapter; extra formats require version-specific evidence
and remain optional ecosystem capabilities, not CJM-owned semantics. Format
lists and API sketches below are hypotheses, not verified library support or
approved implementation tasks. Binary work does not block any JSON release.

This document records CJM's future strategy for binary representations, tagged
wire protocols, borrowed view models, and related benchmark work.

It is intentionally a future-option preservation document. It does not add a
binary backend, dependency, Metadata IR field, CMake option, or user-facing
metadata syntax.

---

# Goal

CJM may eventually let one validated C or C++ model drive more than JSON
integration:

```text
C/C++ source model
    |
    v
Semantic Analysis
    |
    v
Metadata IR
    +--> JSON codecs
    +--> generated model contract
    +--> JSON Schema
    +--> future binary JSON-like format integration
    +--> future tagged wire protocol integration
    +--> future generated serialized views
```

This document defines the boundaries that must hold before any binary direction
becomes implementation work.

The active priority remains the v0.6 runtime JSON backend program:

```text
runtime JSON semantics
    |
    v
conformance foundation
    |
    v
simdjson-first runtime backend program
```

Binary work must not interrupt or dilute that sequence.

---

# Product Boundary

CJM is a build-time metadata compiler.

CJM owns:

- source-language model understanding
- semantic analysis
- Metadata IR
- model-specific generated code
- generated contracts
- schema generation
- source-aware diagnostics
- conformance expectations

Runtime and transport layers own:

- generic JSON or binary parsing primitives
- generic encoding and decoding primitives
- buffer allocation
- request and response buffer ownership
- parser and document lifetime
- network and transport lifetime
- async and thread handoff policy
- low-level optimization
- fuzzing and malformed-input hardening

The governing boundary is:

```text
CJM owns model knowledge.
Mature runtimes own generic format primitives.
Transport layers own buffer lifetime.
```

The dependency direction must not reverse.

---

# Format Taxonomy

Binary support must be described by layer. The phrase "binary backend" is too
ambiguous by itself.

## Binary JSON-Like Representations

Examples:

```text
BEVE
CBOR
MessagePack
BSON
```

These formats are structurally closest to JSON. Their type surface usually
resembles:

```text
bool
number
string
bytes
array
map/object
null
nested values
```

They are the lowest-complexity binary experiments for CJM because they can
reuse many existing JSON mapping decisions:

- field participation
- logical field name
- ignored-field semantics
- optional and null semantics
- enum string or numeric policy, once explicitly chosen
- nested object structure
- container mappings

They still require explicit decisions about:

- object key representation
- enum representation
- integer width and overflow policy
- canonical ordering
- duplicate keys
- unknown fields
- native byte strings
- malformed payload behavior

Binary JSON-like formats should be evaluated before CJM invents a new
schema-based wire protocol.

## Tagged Wire Protocols

Examples:

```text
Protobuf-like protocol
Thrift-like protocol
```

These require semantics beyond current JSON metadata:

- stable numeric field identity
- reserved field IDs
- wire types
- presence rules
- default semantics
- unknown-field behavior
- field addition and removal policy
- deprecation policy
- compatibility checking
- message framing
- version evolution

Current facts such as C++ field name, JSON field name, and source order are not
sufficient as a stable wire contract.

A tagged binary protocol must not be presented as "just another backend" until
the required wire semantics exist.

## Serialized View Layouts

Examples:

```text
FlatBuffers-like layout
Cap'n Proto-like layout
```

These require a canonical serialized layout:

- offsets
- alignment
- endianness
- bounds checks
- table or struct layout
- verifier rules
- buffer lifetime
- generated accessors
- schema evolution

An ordinary C or C++ object in memory is not a portable wire format because of:

- padding
- alignment
- ABI
- endianness
- pointer values
- `std::string` layout
- `std::vector` layout
- architecture width
- object lifetime
- strict aliasing

A serialized view layout would expose generated accessors over a canonical
buffer. It would not reinterpret arbitrary user objects as wire messages.

---

# Model-First IDL Boundary

CJM's source of truth remains:

```text
user-owned C/C++ source models
```

This means CJM should not require a second user-authored `.proto`, `.fbs`, or
other external IDL merely to use another representation.

However:

```text
no separate user-authored IDL
```

does not mean:

```text
no protocol schema or compatibility contract
```

For a tagged wire protocol, CJM would still need to derive and preserve a stable
wire contract from the source model and approved metadata.

The accurate future claim is:

```text
A deliberately restricted C/C++ model profile may serve as a model-first IDL.
```

Not:

```text
Every C++ struct is automatically a portable cross-language IDL.
```

Portable profiles must restrict or explicitly define types such as:

- fixed-width integers
- floating point
- UTF-8 string
- bytes
- enum
- object
- optional
- sequence
- string-keyed map
- explicit tagged union
- stable field identity

Types such as `std::size_t`, `long`, `std::filesystem::path`, `std::chrono`
types, custom containers, allocator-aware containers, pointers, and
polymorphic objects require explicit portable semantics or converters.

---

# Generic And Format-Specific Metadata

CJM must not mechanically rename existing `json:` tags to `cjm:`.

The long-term metadata model should distinguish:

```text
field identity
backend-neutral model semantics
format-specific mapping
runtime policy
```

Possible future namespaces:

```text
cjm:
    backend-neutral model semantics

json:
    JSON-specific representation

wire:
    tagged wire identity and encoding

format-specific namespaces:
    only when unavoidable
```

Existing `json:` metadata remains valid and backward compatible.

Current same-name fields should remain tag-free:

```cpp
std::string name;
```

Current JSON rename metadata should remain valid:

```cpp
std::string display_name; // json:"displayName"
```

Future generic metadata must not be added until real backend work proves a
missing semantic fact.

---

# Logical Type: bytes

`bytes` is a future backend-neutral logical type.

It means:

```text
a sequence of arbitrary binary octets with no text or character-encoding semantics
```

Possible owned C++ representation:

```cpp
std::vector<std::uint8_t>
```

Possible borrowed C++ representation:

```cpp
std::span<const std::uint8_t>
```

Different formats may map `bytes` differently:

```text
JSON
    Base64 string or explicitly selected integer array

CBOR
    native byte string

MessagePack
    native binary value

tagged binary protocol
    length-delimited byte field

serialized view layout
    offset plus length view
```

CJM must not treat bytes as UTF-8 text.

Current CJM does not implement `bytes` as a logical type. `std::uint8_t` is
currently an unsigned integer mapping, not a byte-string policy.

---

# Owned And Borrowed Semantics

Owned fields control the lifetime of their data.

Examples:

```cpp
std::string
std::vector<std::uint8_t>
```

Borrowed fields reference memory owned elsewhere.

Examples:

```cpp
std::string_view
std::span<const std::uint8_t>
```

Borrowed fields may reduce allocation and copying, but they introduce lifetime
requirements:

- who owns the input buffer
- how long the buffer remains valid
- whether the object can escape the handler
- whether the object can cross threads
- whether the runtime unescapes into separate storage
- whether parser scratch memory is involved

Borrowed means:

```text
non-owning data view with an explicit external lifetime
```

It does not mean:

```text
always zero-copy
always allocation-free
always safe to store
```

Current CJM does not implement borrowed field semantics.

---

# Borrowed View And Transport Boundary

Zero-copy is not a CJM backend feature by itself.

True zero-copy or low-copy request handling spans multiple layers:

- socket or file input buffer
- HTTP body or message frame lifetime
- parser lifetime
- request handler lifetime
- coroutine and async boundaries
- thread handoff
- buffer pools
- backpressure
- response writer ownership

Those are transport/runtime concerns. A future layer such as `cjm-rest` is a
more natural owner for request and response buffer lifetime.

CJM may eventually preserve metadata needed by a transport layer to attempt
borrowed or low-copy decoding:

- field is eligible for borrowed view semantics
- field is bytes or text
- generated contract exposes ownership expectations
- generated view model is separate from owned domain model

CJM must not claim:

```text
CJM supports zero-copy
```

unless a concrete transport/runtime integration owns and verifies the necessary
lifetime contract.

Preferred wording:

```text
CJM may preserve metadata needed by a future transport layer to attempt borrowed
or low-copy decoding for eligible fields.
```

---

# Safety Rules For Serialized Views

Never generate unchecked code such as:

```cpp
auto value = *reinterpret_cast<const std::int64_t*>(buffer);
```

without solving:

- bounds
- alignment
- endianness
- object lifetime
- strict aliasing
- malformed offsets
- integer overflow
- untrusted input

For fixed-width scalar reads, a safe baseline may use copying plus endian
conversion:

```cpp
std::int64_t value;
std::memcpy(&value, buffer, sizeof(value));
value = from_wire_endian(value);
```

A true serialized view design must define:

- canonical layout
- alignment policy
- offset validation
- buffer ownership
- verifier rules
- accessor lifetime
- schema evolution

---

# Explicit Format Selection

Protocol selection must be explicit.

It must not be implicitly tied to:

- Debug build
- Release build
- `NDEBUG`
- optimization level

Rejected design:

```cpp
#ifdef NDEBUG
using Format = Binary;
#else
using Format = Json;
#endif
```

Wire format is an application contract, not a compiler optimization.

Approved future selection mechanisms may include:

- generation-time selection
- build-time explicit selection
- endpoint-specific policy
- content negotiation
- protocol negotiation

Conceptual future CMake:

```cmake
cjm_generate(
    TARGET service
    HEADERS models.hpp
    FORMATS json cbor
)
```

This syntax is conceptual only.

---

# Recommended First Binary Experiment

CJM should not invent a native binary protocol first.

The preferred first experiment, if binary work becomes justified, is:

```text
C++ model
    |
    v
CJM Metadata IR
    |
    v
CJM-generated Glaze metadata or integration
    |
    v
Glaze JSON / selected experimentally verified ecosystem format
```

This experiment can test:

- one model source
- multiple representations
- typed direct runtime integration
- minimal new codec implementation
- payload size differences
- encode and decode performance
- semantic consistency
- build cost

The experiment must be time-boxed. It must not automatically promote every
format supported by the runtime.

---

# Benchmark Policy

Benchmarks must compare complete equivalent pipelines.

JSON runtime baselines may include:

```text
CJM + nlohmann
raw Glaze
CJM + Glaze
handwritten simdjson binding
CJM + simdjson
handwritten yyjson binding
CJM + yyjson
```

Binary representation baselines may include:

```text
CJM + Glaze JSON
CJM + Glaze BEVE
CJM + Glaze CBOR
CJM + Glaze MessagePack
```

Later, only if justified:

```text
Protobuf baseline
FlatBuffers baseline
other mature binary runtimes
```

Measure:

- encoded payload size
- encode throughput
- decode throughput
- round-trip latency
- p50, p95, and p99 latency
- allocations
- bytes allocated
- peak memory
- generated source size
- clean compile time
- incremental compile time
- peak compiler memory
- binary size
- dependency footprint
- diagnostic quality
- debuggability
- maintenance surface

Document semantic equivalence for every comparison:

- UTF-8 validation
- range checking
- unknown fields
- duplicate fields
- missing and default behavior
- null
- schema compatibility
- verification
- borrowed versus owned decode

Do not compare unchecked binary decode against strict validated JSON decode
without labeling the difference.

---

# Promotion Gates

No representation or runtime becomes official based on throughput alone.

Promotion requires evidence for:

- correctness
- malformed-input behavior
- error quality
- ownership safety
- lifetime safety
- semantic parity
- build cost
- dependency cost
- platform coverage
- downstream use
- maintenance burden

Binary work remains optional. CJM remains a successful product even if binary
experiments are deferred or abandoned.

---

# Repository Boundary

CJM may own:

- Metadata IR extensions when a real backend proves a missing semantic fact
- generic model semantics
- format-specific metadata
- backend generators
- generated model-specific codecs
- conformance tests
- diagnostics
- benchmark harnesses

CJM should not own by default:

- generic JSON parser
- generic binary parser
- generic mutable DOM
- SIMD structural scanner
- varint primitive library
- generic CBOR runtime
- generic MessagePack runtime
- generic BEVE runtime
- generic serialized-view verifier
- transport buffer lifetime

If a CJM-native runtime is proposed, it should require:

- separate repository consideration
- independent product justification
- benchmark evidence
- correctness plan
- fuzzing plan
- security plan
- maintenance estimate
- stop criteria

before implementation begins.

---

# Staged Investment Model

## Stage 0 - Documentation

Active now.

Record:

- format taxonomy
- metadata layering
- bytes semantics
- owned and borrowed boundary
- wire-contract requirements
- serialized-view safety rules
- benchmark methodology
- promotion gates

No code.

## Stage 1 - Complete JSON Runtime Foundation

Required first:

- runtime semantic profile
- decode error/path model
- conformance fixtures
- static backend selection
- simdjson experiment
- Glaze evaluation
- yyjson evaluation
- fair JSON benchmark

## Stage 2 - Binary JSON-Like Spike

Potential time-boxed experiment:

```text
CJM + Glaze
JSON / selected experimentally verified ecosystem format
```

## Stage 3 - Product Signal Review

Evaluate:

- performance
- payload size
- integration cost
- semantic consistency
- real downstream demand
- maintenance burden

Possible decisions:

- promote one format
- keep experimental
- defer
- stop

## Stage 4 - Generic Metadata RFC

Only if multiple formats prove a real need.

Research:

- `cjm:` generic semantics
- `json:` compatibility
- `wire:` identity and encoding
- conflict precedence
- portable type profile
- C and C++ syntax alignment

Do not rename existing tags without migration policy.

## Stage 5 - Tagged Wire Contract Research

Only if real versioned transport demand exists.

## Stage 6 - Borrowed View And Serialized Layout Research

Only if real workloads justify generated views and lifetime complexity.

No stage is automatic.

---

# Non-Goals

Do not:

- implement a binary backend now
- add Glaze binary code now
- introduce `cjm:` syntax now
- rename `json:` tags now
- add wire IDs now
- invent a CJM binary protocol now
- create a universal serialization facade
- implement serialized views by raw struct cast
- claim arbitrary C++ is a portable IDL
- generate other languages now
- create `cjm-json` or `cjm-binary` repositories
- change the v0.6 implementation order
- make binary a v1.0 blocker
- promise a Protobuf or FlatBuffers replacement
- tie protocol selection to Debug or Release builds
- claim zero-copy as a CJM feature without a transport/runtime owner

---

# Relationship To Other Documents

This strategy depends on:

- [High-Performance JSON Strategy](high-performance-json-strategy.md)
- [Runtime Backend Program](runtime-backend-program.md)
- [Static Backend Selection](static-backend-selection.md)
- [Metadata Model](metadata-model.md)
- [Generated Model Contract](generated-model-contract.md)
- [JSON Mapping Scope](json-mapping-scope.md)

It feeds possible future work:

- binary JSON-like representation experiments
- generic metadata RFC
- tagged wire-contract research
- borrowed view and transport integration research
- backend benchmark methodology
