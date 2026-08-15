# CJM Roadmap

This roadmap describes CJM as both a product strategy and a development plan.

CJM should become useful through small, testable milestones while preserving a
clear long-term identity.

---

# Vision

CJM is a metadata-driven source generator for Modern C++.

The goal is **not** to become another JSON library.

The goal is to let existing C++ models remain the single source of truth while
generating JSON integration automatically during the build.

> CJM keeps your C++ models as the source of truth. It generates the JSON
> integration around them, not the models themselves.

Design principles:

- C++ source is the source of truth.
- Metadata lives next to the field it describes.
- Generated code is ordinary C++.
- Generation happens at build time, not through runtime reflection.
- The architecture remains backend-independent.
- The workflow remains CMake-first.
- v1.0 supports a documented, tested JSON mapping matrix rather than an
  open-ended promise to support the entire C++ type system.

---

# Architecture Direction

The long-term architecture is:

```text
C++ Source
    |
    v
Parser
    |
    v
Semantic Analysis
    |
    v
Metadata IR
    |
    +----------------+
    |                |
    v                v
JSON Backend    Schema Backend
    |                |
    v                v
Generated C++    JSON Schema
```

The Metadata IR is the center of CJM.

All parser implementations produce it.
All backends consume it.

Future backends may include:

- compatibility backends such as nlohmann/json
- schema backend
- documentation backend
- metadata adapter backends such as CJM-generated Glaze metadata or DAW JSON
  Link contracts
- generated codec backends such as simdjson On-Demand or an independently
  justified Glaze custom codec
- compact document / DOM backends such as yyjson
- SAX or state-machine experiments such as RapidJSON SAX
- optional native runtime research outside the core CJM repository

These are architectural directions, not immediate commitments. Every runtime
backend must consume normalized Metadata IR semantics rather than parser syntax
or backend-specific comments.

---

# Philosophy

CJM prefers:

- explicit over implicit
- generated code over runtime magic
- metadata over external configuration
- build-time errors over runtime surprises
- ordinary C++ over proprietary DSLs
- readable generated output over opaque machinery

---

# Non-Goals

CJM is not intended to become:

- a JSON parser
- a serialization runtime
- a reflection framework
- a schema-first code generator
- a JSON editor
- a UI renderer
- a general-purpose compiler
- a replacement for CMake
- a replacement for the user's compiler

These are intentionally left to existing projects.

CJM should remain focused:

> Build-time C++ code generation from source-level metadata.

---

# Current Status

CJM is in early development.

The current focus is to preserve the compiler architecture while expanding the
documented practical mapping surface:

- product identity
- architecture
- build pipeline
- metadata model
- explicit CMake integration
- practical JSON code generation through the first official backend

---

# Discovery and Indexing Strategy

CJM should distinguish indexing from SEO.

Indexing means:

> Search engines and developer platforms can discover that CJM exists and
> understand what it is.

SEO means:

> CJM is deliberately competing for search ranking.

Before the product is useful on practical projects, CJM should focus on
indexing and discovery rather than ranking optimization.

## Phase 0 - Indexing and Discovery

Status:

> Completed for the current early-adopter stage.

This phase may be completed once before or during v0.2.

Goal:

> Make CJM discoverable without distracting from core product development.

Scope:

- GitHub repository title and description
- GitHub About metadata
- GitHub topics
- website URL
- release description
- organization README
- basic website indexing metadata
- `robots.txt`
- `sitemap.xml`
- canonical URL metadata
- Google Search Console setup
- domain verification
- sitemap submission

Out of scope:

- SEO ranking optimization
- content marketing
- blog articles
- newsletter
- Discord or community operations
- documentation search
- API reference

Success criteria:

- Google can discover the CJM website
- GitHub clearly communicates what CJM is
- project metadata consistently describes CJM as a build-time metadata compiler
  for Modern C++
- indexing work can be left alone while product development continues

## Phase 1 - Adoption Documentation

Status:

> Initial repo and website adoption messaging is complete. A full documentation
> site remains future work.

This phase belongs with an adoption-focused milestone after v0.3.0, when the
practical type surface is strong enough for new users to try.

Goal:

> Help developers understand and try CJM through documentation, not SEO tricks.

Scope:

- documentation site foundation
- getting started guide
- installation guide
- CMake integration guide
- JSON tags guide
- examples
- FAQ
- architecture overview for users

## Phase 2 - Content and Dogfooding

Status:

> Initial dogfooding has completed through `ull-md-engine`. Broader content
> work should still wait for more real adoption data.

Dogfooding may begin after v0.3.0 so a downstream project can consume a real
tag. Content work should begin only after that dogfooding produces real
results.

Dogfooding requirement:

- integrate CJM into `ull-md-engine` through the supported installation and
  build workflow
- use `ull-md-engine` to validate that CJM works in a real downstream project,
  not only in repository examples
- treat any integration friction as product feedback before v1.0

Potential content:

- Why build-time code generation?
- Go struct tags in C++
- Reflection vs code generation
- case study: `ull-md-engine` after the integration is real
- benchmark writeups

## Phase 3 - Brand and Community

This phase belongs near v1.0, once CJM has stable public workflows.

Potential work:

- release blog
- newsletter
- GitHub Discussions
- Discord or community space
- documentation search
- public roadmap
- API reference
- community contribution paths

---

# v0.1 - Minimal Working Generator

Goal:

> Generate working JSON mapping code for one simple C++ struct.

Scope:

- CMake-first integration
- explicit header registration
- parse one or more C++ headers
- extract basic JSON field tags from comments
- build the initial Metadata Model
- generate `to_json()`
- generate `from_json()`
- generate `*.cjm.hpp`
- support basic scalar fields
- golden-file tests
- example project

Supported initial field types:

- bool
- integer types
- floating-point types
- std::string

Out of scope:

- automatic header discovery
- nested structs
- containers
- custom backends
- native JSON backend
- inheritance
- templates
- polymorphism
- private fields

Success criteria:

```text
struct User {
    std::string name; // json:"name"
    int age;          // json:"age"
};
```

to:

```cmake
cjm_generate(
    TARGET app
    HEADERS
        user.hpp
)
```

to:

```text
user.cjm.hpp
```

to:

```cpp
nlohmann::json j = user;
User parsed = j.get<User>();
```

---

# v0.2 - Practical Models

Goal:

> Support realistic production models.

Add support for:

- nested supported structs
- std::vector<T>
- std::optional<T>
- enums
- namespaces
- type aliases
- `omitempty` metadata
- `ignore` metadata
- multiple input headers
- dependency ordering

Begin the production mapping matrix:

- JSON primitives through common C++ scalar and string types
- JSON objects through supported structs
- JSON arrays through `std::vector<T>`
- missing/null semantics through `std::optional<T>`
- field rename metadata
- `ignore` metadata
- `omitempty` metadata
- basic unsupported-type diagnostics

Improve:

- diagnostics
- generated code formatting
- golden tests
- examples
- integration with realistic downstream model projects

Success criteria:

- multiple related structs generate correctly
- nested JSON objects work
- optional and omitted fields behave predictably
- generated output remains deterministic
- CJM can be dogfooded against practical model headers

---

# v0.3 - Practical Type Coverage

Status:

> Completed for v0.3.0.

Goal:

> Cover the next set of common JSON data shapes after v0.2 practical models.

Add:

- `std::map<std::string, T>`
- `std::unordered_map<std::string, T>`
- nested supported map value types
- fixed-width integer type coverage
- verification for composed practical types
- generated compile/run tests for the expanded mapping surface

Mapping scope:

- JSON objects with dynamic string keys through supported map types
- nested combinations of supported vectors, optionals, maps, enums, and
  generated structs
- common signed and unsigned fixed-width integer spellings

Out of scope:

- arbitrary JSON values
- `std::variant`
- `std::any`
- pointer fields
- polymorphism
- custom converters
- enum string policies
- time and datetime mappings
- automatic header discovery
- frontend parser migration

Success criteria:

- string-keyed maps work through Parser, Semantic Analysis, Metadata IR, and
  the nlohmann backend
- generated map code compiles and round-trips for practical fixtures
- unsupported map forms fail before generation
- generated output remains deterministic

---

# v0.3.2 - Adoption and Dogfooding

Status:

> Completed for the initial v0.3.2 adoption pass.

Goal:

> Let v0.3.0 be tried from real downstream and documentation workflows without
> blocking the v0.3.0 release tag.

Scope:

- use `ull-md-engine` as the first dogfood project for simple JSON logging
  models
- capture CMake, CLI, diagnostics, installation, and documentation friction as
  follow-up issues
- explore a less repetitive file-list or manifest-based input workflow for
  projects with many related model headers
- improve getting-started and adoption documentation
- keep GitHub, README, and website messaging aligned with the metadata compiler
  positioning

Out of scope:

- Tree-sitter frontend migration
- broad content marketing
- benchmark or case-study claims before dogfooding produces real results
- new practical mapping categories beyond the v0.3.0 surface

Success criteria:

- `ull-md-engine` has an experimental CJM integration path for a real logging
  use case
- friction discovered during dogfooding is captured as CJM follow-up work
- adoption documentation explains how to try the current supported workflow

Completed:

- `ull-md-engine` consumed CJM v0.3.0 through the public
  `FetchContent` + `cjm_generate` workflow
- dogfood covered a simple JSON logging model and a broader practical mapping
  surface model
- downstream warning friction was fixed in CJM
- README, release notes, dogfood report, issue templates, Discussions, and
  contributing guidance were updated for early adopters
- cjm-labs.org messaging was updated for early-adopter outreach
- first outreach posts were published to LinkedIn and the r/cpp Show and Tell
  thread

Deferred follow-up:

- a less repetitive file-list or manifest-based input workflow remains a
  future adoption improvement, to be driven by real downstream need

---

# v0.3.5 - Frontend Parser Research

Status:

> Recommendation recorded: CJM should adopt Tree-sitter as the C++ frontend
> implementation through a staged production migration.

Goal:

> Evaluate Tree-sitter as a future C++ frontend foundation without replacing
> the production parser prematurely.

Evaluate:

- `tree-sitter-cpp` as an implementation detail of the C++ frontend
- strict fail-closed parsing behavior
- deterministic comment-to-field attachment
- multiline declarations
- nested template type spellings
- source range and diagnostic quality
- preprocessor behavior
- grammar and runtime version pinning
- FetchContent versus vendored generated sources
- license and notice requirements

Deliver:

- Tree-sitter architecture note
- dependency assessment
- isolated adapter prototype
- correctness report
- staged migration plan
- evidence-based recommendation

Out of scope:

- CJM-C
- attribute metadata syntax
- full C++ grammar support
- Clang/LLVM integration
- Metadata IR redesign
- Semantic Analysis redesign
- backend changes
- removing the handwritten parser

Success criteria:

- current supported fixtures can be represented correctly
- unsupported or malformed managed syntax fails closed
- Tree-sitter code remains isolated inside the frontend layer
- Semantic Analysis, Metadata IR, backends, CLI behavior, and generated output
  remain unchanged during the spike
- users do not need Node.js, Rust, npm, Cargo, Python, or the Tree-sitter CLI

Decision:

- adopt Tree-sitter as the C++ frontend implementation
- switch the production parser in a dedicated follow-up migration milestone
- preserve `SourceFileSyntax` as the boundary into Semantic Analysis
- remove the handwritten parser only after the Tree-sitter path has become the
  tested production default

---

# v0.3.6 - Tree-sitter Production Parser Migration

Status:

> Completed. Tree-sitter is now the default C++ frontend parser for CJM's
> supported practical syntax surface.

Goal:

> Promote the validated Tree-sitter frontend from research spike to production
> parser while preserving CJM's public CLI, CMake, Metadata IR, and generated
> output behavior.

Delivered:

- Tree-sitter runtime and `tree-sitter-cpp` grammar sources are built as normal
  CJM frontend implementation details
- dependency fetching is centralized and cached for normal CMake builds
- parser parity fixtures cover the existing supported v0.3 syntax surface
- unsupported managed declarations fail closed instead of being silently
  partially parsed
- CLI and `cjm_generate` use the Tree-sitter-backed parser by default
- Tree-sitter smoke and adapter tests run in the normal test suite
- existing golden, semantic, CLI, generated compile, and example tests pass

Out of scope:

- expanding beyond the v0.3 practical mapping surface
- public Tree-sitter APIs
- full C++ grammar support
- removing every historical handwritten-parser implementation detail
- changing generated C++ output intentionally

---

# v0.4 - Extensibility and Downstream Workflow

Goal:

> Stabilize CJM as a build-time metadata compiler that downstream CMake
> projects and tools can consume reliably.

Add or design:

- backend abstraction
- generated model contract / traits for downstream tools
- reliable downstream CMake workflow
- custom converters
- metadata extensions
- stable IR boundaries
- backend-specific options
- explicit type mapping policies

Generated model contract:

- expose C++ type identity and qualified name through stable generated artifacts
- expose fields in deterministic order
- expose C++ field names and JSON field names
- expose field type categories such as scalar, string, enum, optional, vector,
  map, and nested object
- expose ignored-field and `omitempty` metadata
- avoid exposing CJM internal IR structures as a public API

Downstream CMake workflow:

- document stable generated include directory behavior
- document generated artifact paths and target dependencies
- support another target or downstream tool depending on CJM generated artifacts
- reduce friction for many related model headers
- keep generated artifacts in the build directory
- preserve `FetchContent` and `cjm_generate(...)` as the primary early-adopter
  workflow

Expand the mapping matrix:

- `std::array<T, N>`
- `enum` and `enum class` string mappings
- custom converter design for future non-core domain types such as UUID,
  decimal, filesystem path, duration, and project-specific identifiers

Common type strategy:

- `std::array<T, N>` and enum string mappings are v0.4 implementation targets
- custom converters are a v0.4 design target, not a blanket implementation
  target
- chrono/time mappings should remain deferred until converter boundaries and
  diagnostics are clearer
- pointers, variants, `std::any`, polymorphic types, arbitrary containers, and
  arbitrary map keys remain outside the default practical mapping surface

Dynamic JSON and sum-type strategy:

- dynamic request/response payloads should be tracked as JSON value passthrough,
  not as `std::any`
- `std::variant` should be treated as a future explicit union/sum-type mapping
  with a documented discriminator policy
- `std::any` does not fit CJM's build-time contract unless users provide an
  explicit closed set of possible types, which makes it closer to `variant`
- inheritance and polymorphic serialization require explicit type identity,
  ownership, and discriminator policy, and should remain future work

Initial backend:

- nlohmann/json

Potential later backends:

- artifact backends such as JSON Schema and documentation output
- metadata adapter backends such as CJM-generated Glaze metadata or DAW JSON
  Link contracts
- generated codec backends such as simdjson On-Demand or an independently
  justified Glaze custom codec
- compact document / DOM backends such as yyjson
- later SAX or state-machine experiments such as RapidJSON SAX
- optional native runtime research outside the core CJM repository

Success criteria:

- generators consume the Metadata IR rather than parser-specific structures
- backend-specific code remains isolated
- public user workflows stay stable as internals evolve
- custom conversion points do not leak parser-specific details
- downstream tools can consume generated model information without depending on
  parser internals or CJM internal IR types
- CMake users can depend on generated artifacts without relying on repository
  internals

Out of scope:

- HTTP routing
- server lifecycle
- middleware runtime
- TLS or networking
- auth, database, or logging policy
- OpenAPI route definitions
- application-specific endpoint policy

---

# v0.5 - Schema

Status:

> Completed for v0.5.0.

Goal:

> Generate JSON Schema from the Metadata IR.

Add:

- schema backend
- schema-oriented metadata mapping
- golden tests for schema output
- examples showing generated schemas
- documentation for schema limitations

Schema coverage should align with the supported JSON mapping matrix:

- primitive types
- objects
- arrays
- optional/null behavior
- string-keyed maps
- enum string mappings
- required fields when validation metadata has landed
- default values when default metadata has landed
- documented time string formats when time mappings have landed

Downstream alignment:

- schema output should let downstream tools generate OpenAPI components for
  supported DTOs
- CJM should not own OpenAPI route definitions or HTTP endpoint policy
- schema generation should consume Metadata IR or stable model-contract data,
  not parser-specific syntax

Strategic value:

```text
C++
  |
  v
CJM
  |
  v
JSON Schema
  |
  v
quicktype or other schema tooling
  |
  v
Rust / Go / TypeScript / other languages
```

CJM should not try to generate every language itself.
Schema export gives CJM a clean integration point with existing ecosystems.

---

# v0.5.x - Default Field Mapping and Canonical Field Semantics

> Completed in v0.5.1.

Goal:

> Remove redundant same-name JSON tags while preserving explicit metadata for
> exceptions.

Semantic changes:

- fields in managed models default to their exact C++ field names
- explicit rename metadata overrides the default field name
- `json:",omitempty"` uses the default field name and records `omit_empty`
- `json:"-"` records explicit ignored-field intent
- duplicate effective JSON field names are diagnosed
- unsupported included fields fail closed during Semantic Analysis
- all backends consume the same normalized Metadata IR field facts

Managed model rule:

- near-term CJM-managed models are supported public `struct` declarations in
  explicitly supplied input headers
- helper or implementation structs in those input headers are also considered
  managed unless a future type-level opt-in policy is adopted
- unsupported included fields must produce diagnostics rather than being silently
  dropped
- users may explicitly exclude a field with `json:"-"`

Metadata IR direction:

- ignored fields remain visible in Metadata IR through explicit ignored
  semantics
- an empty JSON name must not be used as the ignored-field marker
- backends that produce runtime JSON or schema surfaces skip ignored fields
- generated model-contract metadata may expose ignored fields for downstream
  inspection

Out of scope:

- automatic snake_case, camelCase, PascalCase, or acronym conversion
- type-level opt-in metadata syntax
- new container mappings
- custom converters
- runtime JSON backend implementation
- native JSON parser or formatter work

Success criteria:

- examples can omit redundant same-name tags
- explicit tags still work as overrides
- `json:",omitempty"` is accepted and deterministic
- `json:"-"` is represented consistently across IR, contract, schema, and
  runtime code generation
- generated nlohmann, contract, schema, CLI, and CMake outputs agree on effective
  JSON names

---

# v0.6 - Canonical Runtime Semantics and Backend Program

Goal:

> Define backend-neutral runtime semantics and prove that CJM Metadata IR can
> drive an optional high-performance JSON runtime path.

This milestone is a program, not a mechanical sequence of release tags. Internal
work packages such as decode spikes, encode spikes, and backend comparisons
should become public releases only when they produce user-consumable capability.

Codec-first is the preferred high-performance experiment order. It does not
require every runtime to receive a generated codec. A metadata adapter, exact-
version experiment, deferred result, or rejected backend is valid when it offers
better total engineering value.

The completed v0.6 epics are:

```text
v0.6 Foundation - Runtime Semantics and Conformance
v0.6 simdjson On-Demand Scalar Decode Spike
v0.6 simdjson Native Baselines And Generated Vertical Slice
```

The foundation defined CJM runtime semantics before simdjson code. The scalar
decode spike, internally tracked as the v0.6.0 epic, then proved that Metadata
IR can generate a C++17 simdjson On-Demand decoder with one-pass field dispatch,
required-field tracking, range checks, and portable structured errors.

Completing that epic did not create a public v0.6.0 release. The spike has no
public backend-selection path and does not make simdjson an official backend.

Foundation scope:

- canonical runtime JSON semantic profile
- minimal decode error and structured path model
- backend taxonomy
- backend capability matrix as documentation and conformance expectations
- conformance fixture skeleton shared by runtime backends, with core and optional
  strict capabilities
- static backend selection shape for CLI and CMake
- optional dependency policy
- C++ standard isolation per backend target
- generation-time diagnostics for unsupported backend capabilities

Minimum runtime semantic profile:

- missing required field behavior
- missing optional field behavior
- explicit `null` for optional fields
- explicit `null` for non-optional fields
- presence and nullability as separate concepts
- unknown-field policy
- duplicate-key policy or explicit unsupported strict capability
- numeric range overflow behavior
- invalid enum string behavior
- fixed-array extent mismatch behavior
- trailing-content behavior
- nested error path shape
- partial-output policy after decode failure
- default public decode shape, preferring new-object decode over in-place partial
  mutation

Completed runtime backend work packages:

- Runtime JSON Semantic Profile
- Minimal Decode Error / Path Model
- Conformance Fixture Skeleton
- Static Backend Selection Design
- simdjson On-Demand decode spike
- simdjson native typed-conversion baselines
- simdjson generated-codec vertical slice with owned strings, optional integer
  presence, and one required nested generated model

Remaining runtime backend work packages:

- simdjson decode MVP over a limited conformance subset
- simdjson builder / encode spike
- simdjson experimental backend with decode, encode, conformance, and docs
- Glaze metadata adapter evaluation after simdjson context is preserved
- separate Glaze generated custom codec evaluation only if its documented API
  and evidence justify the maintenance cost
- yyjson compact document / DOM evaluation
- backend comparison and promotion report

Backend classification:

- `nlohmann/json` is the official compatibility backend
- simdjson On-Demand is a validated generated-codec research candidate, not an
  official backend
- simdjson native custom-type conversion and reflection paths are comparison
  baselines, not capabilities CJM may ignore
- Glaze metadata generation is a later optional adapter candidate
- Glaze custom codec generation is a distinct, stoppable experiment
- yyjson is a compact document / DOM candidate, not a no-DOM backend
- DAW JSON Link is a possible time-boxed direct-typed C++17 spike
- RapidJSON SAX is a possible low-level state-machine experiment
- a native `cjm-json` runtime remains separate optional research

Non-goals:

- universal `JsonRuntime` facade
- making optional runtime dependencies mandatory
- raising CJM core or nlohmann users to C++23
- implementing all runtime candidates in one PR
- declaring the fastest backend before fair benchmarks
- assuming generated codecs are superior before comparison with runtime-native
  typed paths
- native scanner, parser, formatter, or generic DOM implementation inside CJM

Generated files should not be rewritten if contents do not change.

Success criteria:

- runtime backends share Metadata IR semantics, not parser syntax
- simdjson decode and encode work proceeds contiguously
- unsupported backend/type combinations fail clearly at generation time
- conformance fixtures describe shared behavior before backend-specific claims
- runtime-native baselines use labeled compiler, C++ standard, semantic options,
  and error behavior
- runtime APIs are classified before generated code depends on them
- no backend is promoted based on performance alone

The static backend selection contract lives in
[docs/design/static-backend-selection.md](docs/design/static-backend-selection.md).

---

# v0.7 - Reliability

Goal:

> Make CJM predictable and trustworthy on larger real-world inputs.

Improve:

- source-location aware diagnostics
- unsupported-type diagnostics
- richer structured JSON decode errors for supported generated readers
- duplicate tag detection
- invalid tag syntax diagnostics
- dependency cycle detection
- generated include stability
- deterministic dependency ordering
- clearer CMake failure messages
- regression tests for failure cases

Harden semantic mapping features:

- required metadata
- optional metadata
- default value metadata
- documented time string mappings, likely through `std::chrono` or converter
  policy rather than ad hoc built-ins
- missing required diagnostics
- type mismatch diagnostics
- invalid enum string diagnostics
- null-where-not-allowed diagnostics when supported by generated decode policy
- custom conversion failure diagnostics when custom converters exist
- failure regression corpus for the v0.6 runtime semantic profile

Success criteria:

- users can understand why generation failed
- diagnostics point to the user's source whenever possible
- unsupported input fails clearly instead of producing confusing generated code
- supported input produces stable output across repeated builds

---

# v0.8 - Documentation and Ecosystem

Goal:

> Make CJM understandable as a Modern C++ developer tool, not only as a JSON helper.

Add:

- complete quick start
- tutorial
- examples for each supported model pattern
- generated code explanation
- troubleshooting guide
- JSON mapping scope documentation
- backend strategy documentation
- competitive landscape documentation
- contribution guide
- release notes process

Mapping scope:

- complete supported type matrix documentation
- examples for every supported v1.0 mapping
- troubleshooting entries for common mapping failures
- generated code explanation for supported mappings

Example projects:

- simple struct
- nested structs
- vector fields
- optional fields
- enum fields
- namespace usage
- schema generation
- larger CMake project

Success criteria:

- a new user can understand what CJM is and what it is not
- documentation explains the source-of-truth model
- examples match supported behavior instead of future aspirations
- contributors can understand CJM's architecture before changing code

---

# v0.9 - Release Hardening

Goal:

> Prepare CJM for a stable v1.0 release.

Harden:

- public CMake API
- CLI behavior
- generated file conventions
- generated model contract / traits compatibility expectations
- Metadata IR compatibility expectations
- backend option behavior
- backend promotion classification
- versioning policy
- release artifact process
- cross-platform CI
- installation validation
- dogfood coverage
- final v1.0 supported mapping matrix review

Mapping scope:

- freeze the v1.0 mapping matrix
- complete golden tests for supported mappings
- complete schema backend tests for supported mappings
- classify runtime backends as official, experimental, research-only, or deferred
- move unfinished mapping work to Future Ideas or mark it experimental

Success criteria:

- all v1.0 public interfaces are either stable or explicitly marked experimental
- release artifacts can be produced repeatably
- supported platforms are tested continuously
- downstream projects can adopt CJM without relying on repository internals
- `ull-md-engine` consumes CJM through the supported installation workflow
- at least one downstream-tool experiment can consume CJM generated model
  contract without depending on parser internals
- all supported mappings have tests, examples, and diagnostics

---

# v1.0 - Production Ready

Goal:

> Make CJM stable enough for external production use.

Definition of done:

- production-ready core workflow
- documented public APIs
- stable CMake API
- stable generated file conventions
- stable generated model contract for downstream tools
- versioned releases
- installable packages
- release artifacts
- reliable diagnostics
- tested supported type matrix
- cross-platform CI
- examples
- troubleshooting documentation
- dogfooded on practical models, including `ull-md-engine`
- at least several external or downstream projects using CJM successfully

Required JSON mapping surface:

- `bool`
- signed and unsigned integer types
- floating-point types
- `std::string` as UTF-8 JSON strings
- nested supported structs
- namespaces
- multiple input headers
- `std::vector<T>`
- `std::array<T, N>`
- `std::optional<T>`
- `std::map<std::string, T>`
- `std::unordered_map<std::string, T>`
- `enum` and `enum class` string mappings
- untagged managed fields using exact C++ field names as JSON field names
- field rename metadata
- ignore metadata
- `omitempty` metadata
- required/default metadata only if accepted and implemented before v1.0
- schema output for supported mappings
- clear diagnostics for unsupported types and invalid metadata

Common types supported through built-ins or documented converter policy:

- fixed-size arrays through `std::array<T, N>`
- string enum representation for supported enums
- time/datetime values through accepted converter policy if implemented before
  v1.0
- dynamic JSON value passthrough only if a backend-neutral policy is accepted
- domain scalar types such as UUID, decimal, filesystem path, duration, and
  project-specific identifiers through custom converters where appropriate

Out of scope for v1.0 by default:

- arbitrary C++ templates
- arbitrary custom containers
- arbitrary map key conversion
- built-in support for every domain scalar type
- pointer ownership semantics
- `std::variant` without an explicit discriminator / union policy
- `std::any`
- inheritance and polymorphic serialization
- native UTF-16 or UTF-32 string conversion
- Unicode normalization
- automatic timezone inference
- complete JSON Schema validation runtime

v1.0 should represent a tool that external C++ developers can reasonably try in
real projects, not just a demo pipeline.

---

# Future Ideas

These are not commitments.

They may be explored only after the core workflow is stable.

Possible future work:

- C++26 reflection integration
- JSON Schema export improvements
- JSON value passthrough fields
- `std::variant` support with explicit discriminator policy
- selected extra containers through documented policies or converters
- custom map key converters
- RapidJSON backend
- simdjson backend promotion
- Glaze backend promotion
- yyjson backend promotion
- DAW JSON Link evaluation
- binary JSON-like representation experiments only after runtime backend
  evidence, following [Binary Format Strategy](docs/design/binary-format-strategy.md)
- YAML backend
- OpenAPI integration
- reflection backend
- validation metadata
- incremental generation improvements
- IDE integration
- VS Code extension
- compile database integration
- package manager integration
- richer metadata syntax
- custom field adapters

---

# Roadmap Principle

Every milestone should produce something testable.

Every feature should preserve the core philosophy:

```text
Standard C++ in.

Standard C++ out.
```
