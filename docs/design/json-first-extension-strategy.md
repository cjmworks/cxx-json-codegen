# JSON-First Extension Strategy

Status: accepted documentation direction, 2026-09-16. No IR or runtime API
change is authorized by this document.

## Product Boundary

CJM is a model-first JSON code generator / metadata compiler. The current
product consumes supported C++ models. Future C support must prove that the
same canonical JSON semantics can be reused, not turn CJM into a general
serialization framework or schema-first wire platform.

CJM owns model understanding, semantic normalization, Metadata IR, JSON
mapping, JSON Schema, Model Contract, generated integration, diagnostics, and
conformance. Runtime libraries own their generic parsing/writing machinery.
Prefer a small set of backends with distinct, demonstrated value:

| Runtime | Intended role | Status boundary |
| --- | --- | --- |
| nlohmann/json | Compatibility/reference JSON integration | Existing backend |
| simdjson | Generated JSON codec; performance assessed by measurements | Active experimental work |
| Glaze | Optional JSON metadata adapter | Future evaluation, not a core dependency |
| yyjson | C-oriented compact DOM/document binding candidate | Future product research, not no-DOM |

These roles are not a capability matrix. Populate feature support from pinned
version experiments and conformance tests, not from runtime reputation.

## Glaze: Output Adapter, Not a Second IR

The intended dependency flow is:

```text
C++ source -> semantic normalization -> CJM Metadata IR
                                           |
                                           v
                                  generated glz::meta<T>
                                           |
                                           v
                                    optional Glaze runtime
```

Core, Semantic Analysis, and other backends must not consume Glaze metadata.
Glaze API upgrades should be contained in the adapter, its artifacts, and its
tests. Its compiler/standard requirements must remain isolated to opt-in
targets. CJM IR remains the semantic authority; adapt differences, reject
unsupported mappings, or document limitations rather than silently adopting
Glaze defaults.

First learn Glaze directly: JSON read/write, automatic reflection, explicit
metadata, rename/ignore, enum, optional, nested values, errors, compile-time
cost, and the selected version's requirements. Then compare handwritten
metadata with CJM IR and implement one minimal JSON adapter experiment.
Expand only after the mapping is understood and useful. A generated Glaze
custom codec is not part of the planned adapter: it would require a separate
evidence-backed product decision.

Additional formats may be exposed through an optional ecosystem adapter only
when the selected runtime version and mapping have been verified. YAML, TOML,
BEVE, CBOR, and MessagePack are not promised Glaze capabilities here and are
not CJM-native backend commitments. CJM does not take ownership of their
parsing, canonicalization, wire compatibility, or conformance by generating
metadata. No universal runtime facade or format expansion is needed now.

## C: Validate, Freeze the MVP, Then Release v1.0

Schedule a bounded C frontend research spike around v0.9, before stabilizing
v1.0 contracts. Its goal is architectural evidence, not full C support.
After the spike, freeze and implement a bounded C MVP before releasing v1.0.
The release gate is stable C++ JSON codegen plus that agreed C MVP, not full C
language support or resolution of every C-specific finding. This deliberately
allows the release date to move rather than calling an unfinished C MVP done.
Keep the planned Glaze JSON metadata adapter integration before the C spike;
C work must not displace it or require Glaze to wait for a redesigned IR.

The target architecture is language-specific semantic normalization into one
shared backend-facing IR, not permanent CMetadataIR and CppMetadataIR systems.
Temporary C-specific semantic facts are justified only when real source
relationships require them. C frontend work must not depend on yyjson APIs.

Before starting, agree a time budget, fixture list, success criteria, and stop
conditions. The experiment should:

1. Select 5-10 representative C DTOs and handwrite their expected canonical IR.
2. Start with records, enums, typedefs, numeric fields, fixed arrays, and nested
   records; use explicit rename/ignore metadata where needed.
3. Identify reusable IR concepts and concrete lowering gaps.
4. Implement the smallest source-to-IR path and reuse an artifact backend
   where possible, or a minimal experimental C JSON generator. No yyjson
   implementation is required to establish the frontend boundary.
5. Deliver fixtures, mapping tests, one end-to-end path, and a findings report.
   Stop at the agreed boundary rather than building a complete ownership system.

`char[N]` is not automatically a UTF-8 string: termination, length, and capacity
need an explicit contract. Likewise, `char*` and pointer/length pairs do not
imply strings, bytes, sequences, or ownership. Analyze representative pointer
cases as stress tests without requiring their implementation. Arbitrary
pointers, unions, bitfields, flexible array members, complex macros, function
pointers, and ownership inference are outside the initial implementation.

Classify findings as current C++ correctness problems, compatibility risks to
review before stabilization, or deferred C-only requirements. Only real
expression gaps justify a separately approved minimal IR cleanup, with tests
and migration impact. Renaming Vector to Sequence or UserDefined to Record,
splitting source identity, or moving json.ignored are hypotheses, not tasks
automatically triggered by this plan.

The post-spike C MVP must deliver an end-to-end usable path: a documented C
model subset, one JSON runtime binding, encode/decode, explicit errors and
failure cleanup, CMake integration, conformance tests, and examples. yyjson is
the preferred C-oriented candidate, not a dependency of semantic normalization;
confirm the backend choice from spike evidence before implementation.

Freeze one explicit string representation and ownership profile rather than
implementing borrowed views, caller storage, and arenas simultaneously. Define
length/capacity rules and cleanup before coding. Share JSON semantics and error
categories with C++, not necessarily APIs. Broader pointers, ownership profiles,
and other C capabilities outside the agreed MVP remain v1.x work.

If the C spike proves a shared-IR change necessary, review migration impact and
update affected backends, including Glaze, with regression tests. Do not fork
the IR or make Glaze metadata a bridge to C. Publish the MVP scope and acceptance
criteria after the spike; do not interpret this plan as full C support.

## Sequence

| Stage | Focus |
| --- | --- |
| v0.6 | simdjson JSON MVP, runtime semantics, conformance |
| v0.7 | Reliability, diagnostics, stabilization |
| v0.8 | Glaze JSON metadata adapter evaluation and integration; shared IR remains authoritative |
| Around v0.9 | Time-boxed C frontend/IR stress test, then freeze and implement the C MVP |
| v1.0 | Stable C++ JSON codegen plus the completed C MVP; jointly validate existing backends and Glaze adapter |
| v1.x | C capabilities and ownership profiles beyond the frozen MVP |

Dependency order matters more than release labels. Do not interrupt simdjson
work or rewrite IR for hypothetical future frontends.

## Repository Inspection Baseline

The current [IR](../../src/core/ir/model.hpp) already separates parser facts
from backend consumption. It contains `SourceLocation`, `JsonFieldMetadata`,
`FieldTypeKind`, `FieldType`, `FieldModel`, `TypeModel`, `EnumModel`, and
`ProjectModel`. This is useful groundwork, not proof of complete C readiness.

- Canonical candidates: numeric/string/enum categories, array extent, recursive
  type arguments, records, field ordering, presence and container concepts.
- Source identity: field/type names, spelling, qualified names, namespace paths,
  enumerator identifiers, and source locations. Namespaces and emitted C++ type
  expressions require particular care in a C experiment.
- JSON mapping: effective name, omit_empty, and the currently JSON-owned ignored
  flag. Whether participation should be separated requires evidence.
- Vector and UserDefined mix useful model concepts with C++-oriented naming;
  neither naming alone nor prospective C support justifies replacing them.

Current backend implementation consumers of FieldModel/FieldType are:

| Source file under src/backends | Dependency |
| --- | --- |
| nlohmann/cpp_generator.cpp | Field mapping, type dispatch, C++ identities, generated conversion code |
| simdjson/cpp_generator.cpp | Project orchestration and encoder type filtering |
| simdjson/decoder.cpp | Capability checks, recursive decoding, source type expressions, paths |
| simdjson/encoder.cpp | Model/field identities, participation, scalar dispatch, emitted writes |
| schema/schema_generator.cpp | Recursive categories, references, JSON names and required/omission rules |
| contract/contract_generator.cpp | Source identity and JSON/type facts emitted as C++ model metadata |

Thus canonical facts are already reused, but C++ syntax and identity still
matter to several consumers. Backend-independent IR does not imply that every
backend can emit C. Existing CJM-C exclusions in parser-spike documents remain
valid for those historical tasks. The bounded pre-v1.0 study changes future
planning, not the scope of completed spikes.
