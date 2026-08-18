# simdjson Experimental Backend MVP

Status: design contract for epic #203.

This document defines the release boundary for promoting simdjson from internal
research evidence to an opt-in experimental backend MVP.

The target is practical mapping parity with the current `nlohmann/json` backend,
subject only to explicitly documented parity blockers. A blocker does not permit
silent partial generation. Unsupported cases must fail at generation time with
actionable diagnostics.

## Goal

The experimental simdjson backend should let a user choose simdjson explicitly
for the same practical CJM model surface that the current nlohmann backend
supports.

The backend remains experimental because:

- it is newly exposed;
- it has not yet accumulated downstream adoption;
- it has no performance conclusion;
- its encode strategy still needs a backend-specific design decision;
- its conformance suite must prove parity before any stronger status.

The default backend remains `nlohmann/json`.

## Product Contract

The first public simdjson backend is:

```text
experimental
opt-in
not default
parity-targeted
fail-closed
```

Meaning:

- users must explicitly select simdjson through CLI or CMake;
- existing nlohmann behavior remains unchanged;
- simdjson dependency use is isolated to selected backend builds and tests;
- supported mappings should match the current nlohmann practical mapping
  surface;
- unsupported or blocked mappings fail during generation instead of producing
  incomplete generated C++;
- docs must name every known blocker before release.

## Ownership

Parser owns syntax extraction.

Semantic Analysis owns backend-independent validation, type resolution,
dependency analysis, and Metadata IR construction.

Metadata IR remains the shared contract across backends.

The simdjson backend owns:

- simdjson-specific capability checks;
- generated simdjson decode code;
- generated simdjson encode code;
- simdjson dependency assumptions;
- generated-code lifetime rules;
- backend-local runtime error translation;
- fail-closed diagnostics for unsupported backend capabilities.

CLI and CMake own public backend selection.

## Backend Selection

The CLI should eventually support this conceptual shape:

```bash
cjm generate --backend simdjson --input model.hpp --output model.cjm.hpp
```

The exact spelling is owned by the CLI child issue, but the contract is:

- omitted backend means the existing default nlohmann backend;
- `nlohmann` explicitly selects the existing backend;
- `simdjson` explicitly selects the experimental backend;
- unknown backend names fail before generation;
- backend selection does not change parser or semantic behavior.

CMake should eventually support explicit runtime backend selection through the
static backend selection design:

```cmake
cjm_generate(
    TARGET app
    HEADERS model.hpp
    JSON_BACKEND simdjson
)
```

The exact implementation belongs to the CMake child issue. The contract is that
simdjson is not found, fetched, configured, linked, exported, or required for a
default nlohmann-only consumer.

## Capability Matrix

The MVP target is parity with the current nlohmann practical mapping surface.

`required` means the capability must be implemented before release unless a
dedicated parity-blocker decision moves it out of the MVP. A deferred blocker
must have generation-time diagnostics and documentation.

| Metadata IR / feature | Decode MVP | Encode MVP | Status |
| --- | --- | --- | --- |
| `Bool` | required | required | nlohmann parity |
| `SignedInteger` | required | required | includes fixed-width targets and overflow checks |
| `UnsignedInteger` | required | required | includes fixed-width targets and overflow checks |
| `FloatingPoint` | required | required | nlohmann parity target; numeric policy must be documented |
| `String` | required | required | decoded strings are model-owned |
| `Enum` | required | required | JSON string enum mapping |
| `Optional<T>` | required | required | for every supported `T` |
| `Vector<T>` | required | required | for every supported `T` |
| `Array<T, N>` | required | required | fixed extent diagnostics required for decode |
| `Map<std::string, T>` | required | required | string-keyed object mapping |
| `std::unordered_map<std::string, T>` | required | required | output order must be documented |
| `UserDefined` | required | required | generated model composition |
| Recursive `UserDefined` | required | required | parity target; blocker review required before deferral |
| Renamed JSON fields | required | required | effective JSON name contract |
| Same-name default fields | required | required | no tag required |
| Ignored fields | required | required | skipped in decode and encode |
| `omitempty` | decode-compatible | required | encode omission behavior must match nlohmann practical semantics |
| Unknown object fields | required | n/a | ignored |
| Trailing content | required | n/a | raw-text decode rejects trailing non-whitespace |

## Parity Blockers

A parity blocker is a concrete reason that a current nlohmann-supported mapping
cannot safely be supported by the simdjson experimental backend in this release.

Valid blockers include:

- simdjson On-Demand lifetime constraints that would make generated code unsafe;
- forward-only traversal constraints that conflict with a required semantic;
- missing encode writer strategy for a required JSON output shape;
- inability to produce portable diagnostics for a supported failure mode;
- excessive implementation risk that would make the experimental backend less
  reliable than fail-closed behavior.

Invalid blockers include:

- implementation inconvenience alone;
- lack of tests;
- a desire to keep the backend small after choosing parity as the release goal;
- silently relying on downstream compiler errors.

If a blocker is accepted, the backend must:

- document the blocked mapping;
- reject it during generation;
- report the field name and C++ type spelling when applicable;
- preserve all other supported mappings.

## Decode Contract

Generated decode must preserve the runtime semantic profile.

Required decode properties:

- root decode owns padded input, parser, document, and root object lifetime;
- each generated object decoder consumes one object with one forward field
  iteration;
- nested model decode composes generated object decoders;
- child-relative error paths are prepended by the caller;
- vector and array element failures include index path segments;
- map value failures include the string key as a field path segment;
- missing required fields are decode failures;
- missing optional fields produce `std::nullopt`;
- explicit JSON `null` for optional fields produces `std::nullopt`;
- explicit JSON `null` for non-optional fields is a decode failure;
- fixed-width integer overflow is a decode failure;
- invalid enum strings are decode failures;
- fixed-size array extent mismatch is a decode failure;
- unknown model object fields are ignored;
- trailing non-whitespace content is rejected when decoding raw JSON text;
- public decode failure does not expose a partially decoded model.

The exact generated helper function names remain backend implementation detail,
but the generated code must remain readable, deterministic C++.

## Encode Contract

Generated encode must produce deterministic JSON for deterministic model inputs.

Required encode properties:

- generated model fields are emitted in deterministic generated order;
- ignored fields are omitted;
- renamed fields use effective JSON names;
- enum values encode as strings;
- strings are escaped as valid JSON strings;
- optionals follow the selected nlohmann-parity omission/null policy;
- nested models compose generated encode functions;
- vectors and fixed arrays emit JSON arrays;
- string-keyed maps emit JSON objects;
- ordered maps preserve their C++ iteration order;
- unordered maps must either document backend output order limits or use a
  deterministic ordering strategy selected by a child issue.

The encode strategy is intentionally left to #211. The strategy may use a
simdjson-provided writer surface if it is suitable, or a CJM-owned deterministic
writer helper if that better preserves the product contract.

The MVP must not claim performance benefits from either strategy without a
dedicated benchmark issue.

## Diagnostics Contract

Generation-time diagnostics must distinguish:

```text
invalid CJM model
unsupported simdjson backend capability
```

Semantic Analysis owns invalid CJM models. The simdjson backend owns unsupported
simdjson capabilities for otherwise-valid Metadata IR.

Unsupported backend diagnostics should include:

- C++ field name;
- effective JSON field name when relevant;
- C++ type spelling;
- unsupported capability;
- suggested supported shape when obvious.

Decode diagnostics should preserve the portable error and path model:

- portable code;
- structured path;
- backend-local detail when useful;
- no backend-only error as the sole public failure signal.

## Dependency And Standard Isolation

The simdjson backend must not affect default users.

Rules:

- default `cjm_generate(...)` remains nlohmann;
- default CLI generation remains nlohmann;
- selecting nlohmann must not find, fetch, configure, link, or include simdjson;
- selecting simdjson may require simdjson;
- simdjson headers may appear in simdjson generated headers only;
- simdjson must not appear in exported nlohmann targets;
- Catch2 remains test-only;
- backend-specific C++ standard requirements attach only to selected backend
  targets.

The target remains generated C++17 unless a child issue proves and documents a
stricter simdjson-only requirement. Raising CJM core or nlohmann users is out of
scope.

## Test Strategy

The implementation epic should build tests in layers:

1. generator shape and golden tests;
2. generated compile/run tests for focused behaviors;
3. CLI backend-selection tests;
4. CMake backend-selection tests;
5. parity conformance tests against the current practical mapping surface;
6. round-trip tests for representative models;
7. default build isolation tests.

CTest remains the repository-level entry point.

Each supported behavior should have at least one independently runnable test
case before release readiness review.

Golden mismatch diagnostics must identify useful context. Tests should avoid
only comparing generated output with a raw assertion failure.

## Release Readiness Gate

The experimental backend is release-ready only when:

- public CLI selection works;
- public CMake selection works;
- the capability matrix is implemented or blocked with documented
  generation-time diagnostics;
- generated decode tests pass;
- generated encode tests pass;
- round-trip tests pass;
- default nlohmann behavior remains unchanged;
- default simdjson-disabled build remains free of simdjson dependency use;
- docs clearly label the backend experimental;
- docs state dependency, C++ standard, and unsupported-case behavior;
- no performance claim is made without benchmark evidence.

## Out Of Scope

- making simdjson the default backend;
- replacing `nlohmann/json`;
- Glaze implementation;
- benchmark claims;
- C++ static reflection;
- parser redesign;
- backend-independent Metadata IR redesign;
- universal runtime polymorphism;
- supporting arbitrary dynamic JSON documents outside CJM's model surface.
