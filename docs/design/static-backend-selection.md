# Static Backend Selection

This document defines how CJM should select JSON runtime backends as the v0.6
runtime backend program begins.

It is a design contract. It does not implement CLI, CMake, or generated-code
changes by itself.

---

# Goal

CJM should choose runtime backends statically:

```text
generation time or build time
```

not dynamically:

```text
runtime plugin lookup or virtual JsonRuntime dispatch
```

Each generated C++ artifact should target one concrete backend. Backend-specific
dependencies, C++ standard requirements, filenames, and unsupported capability
diagnostics must be resolved before the user's compiler reaches generated code
whenever practical.

---

# Ownership

Static backend selection belongs to the CLI and CMake design layer.

The selected backend changes:

- which backend generator is invoked
- which generated files are produced
- which backend dependency the consuming target needs
- which C++ standard requirement may apply to that generated target
- which backend capability checks run before generation succeeds

The selected backend must not change:

- parser behavior
- Semantic Analysis ownership
- Metadata IR shape
- JSON field-name normalization
- ignored-field semantics
- runtime semantic profile definitions
- conformance case meanings

The Metadata IR remains the contract between frontend/semantic work and all
backends.

---

# Backend Classes

CJM should distinguish backend classes instead of treating every output as the
same kind of option.

## Runtime JSON Backends

Runtime JSON backends produce C++ code that reads or writes JSON through a
specific JSON library or runtime.

Examples:

```text
nlohmann
simdjson
glaze
yyjson
```

Only one runtime JSON backend should be selected for a single generated C++
artifact.

## Artifact Backends

Artifact backends produce non-runtime artifacts from Metadata IR.

Examples:

```text
contract
schema
documentation
```

Artifact backends are not runtime JSON backends. JSON Schema output, for
example, should remain opt-in through schema-specific CLI/CMake behavior rather
than becoming `JSON_BACKEND schema`.

---

# Conceptual CLI Shape

The current C++ generator defaults to the official compatibility backend.

Conceptual future form:

```bash
cjm generate \
  --backend nlohmann \
  --input include/models.hpp \
  --output build/generated/cjm/models.cjm.hpp
```

A high-performance backend would use the same Metadata IR semantics but produce
backend-specific generated code:

```bash
cjm generate \
  --backend simdjson \
  --input include/models.hpp \
  --output build/generated/cjm/models.simdjson.cjm.hpp
```

The exact option spelling is not frozen. The policy is:

- backend selection is explicit when the user wants a non-default runtime
  backend
- unsupported backend names fail before generation
- unsupported backend/type combinations fail before generated code is compiled
- artifact commands such as `generate-schema` remain separate from runtime JSON
  backend selection unless a later design deliberately unifies them

---

# Conceptual CMake Shape

The default workflow should stay simple:

```cmake
cjm_generate(
    TARGET app
    HEADERS include/models.hpp
)
```

Future explicit runtime backend selection should use a runtime-specific option:

```cmake
cjm_generate(
    TARGET app
    HEADERS include/models.hpp
    JSON_BACKEND nlohmann
)
```

or:

```cmake
cjm_generate(
    TARGET app
    HEADERS include/models.hpp
    JSON_BACKEND simdjson
)
```

`JSON_BACKEND` is preferred over a generic `BACKEND` option because CJM also has
artifact backends such as JSON Schema and generated model contracts.

Schema generation remains an artifact request:

```cmake
cjm_generate(
    TARGET app
    HEADERS include/models.hpp
    JSON_BACKEND nlohmann
    GENERATE_SCHEMAS
)
```

This means:

- `JSON_BACKEND` selects generated runtime C++ integration
- `GENERATE_SCHEMAS` requests JSON Schema artifacts
- the two options can coexist because they answer different questions

---

# Generated Artifact Identity

Runtime generated headers must include the selected JSON backend in their
artifact identity. The CMake layout is a backend directory under the generated
C++ output root:

```text
<build-dir>/generated/cjm/nlohmann/user.cjm.hpp
<build-dir>/generated/cjm/simdjson/user.cjm.hpp
```

CMake should add the generated output root, not the backend directory itself, to
the consuming target include path:

```text
<build-dir>/generated
```

Consumers should include runtime generated headers through the same canonical
path regardless of whether the file was produced by `cjm_generate(...)` or by a
manual CLI command:

```cpp
#include "cjm/nlohmann/user.cjm.hpp"
#include "cjm/simdjson/user.cjm.hpp"
```

This keeps the C++ include spelling explicit about both the CJM-generated domain
and the selected runtime backend. `generated` is a physical build output root,
not part of the user-facing include identity.

For direct CLI use, users control the output path. The recommended layout is to
write outputs under an include root using the same canonical path:

```text
<output-root>/cjm/nlohmann/user.cjm.hpp
<output-root>/cjm/simdjson/user.cjm.hpp
```

Artifact backend outputs keep their existing artifact-specific conventions:

```text
<build-dir>/generated/schemas/user.schema.json
```

---

# Dependency Isolation

Backend dependencies must be optional and selected only when needed.

Rules:

- selecting `nlohmann` should not require simdjson, Glaze, yyjson, or RapidJSON
- selecting `simdjson` should not require Glaze or yyjson
- selecting `GENERATE_SCHEMAS` should not add a runtime JSON library dependency
- CJM core libraries and frontends must not include backend runtime headers
- generated backend code may include the selected backend's public headers
- backend tests may depend on their selected runtime library

If a selected backend dependency is unavailable, the failure should occur during
CMake configuration or CJM generation, not as a confusing downstream include
error when possible.

---

# C++ Standard Isolation

Backend C++ requirements must not leak into unrelated targets.

Examples:

- `nlohmann` should remain compatible with CJM's current C++17 workflow
- a future simdjson generated-codec backend should declare its own requirement
- a future Glaze adapter may require a newer C++ standard for the selected
  generated target only

Rules:

- CJM core should not raise its language requirement merely because an optional
  backend exists
- default nlohmann users should not inherit requirements from experimental
  backends
- CMake should attach backend requirements to the generated/backend target that
  needs them
- if the consuming target cannot satisfy the selected backend requirement, CMake
  or CJM should fail before the user sees generated-code compiler noise

---

# Capability Diagnostics

Each runtime backend should declare a static capability surface.

Conceptual fields:

```text
backend name
integration model
supported Metadata IR type combinations
supported directions: encode, decode, round_trip
supported strict capabilities
C++ standard requirement
required dependency target or package
experimental or official status
```

This declaration supports:

- generation-time diagnostics
- conformance fixture selection
- documentation tables
- promotion reviews

It is not a runtime plugin registry.

Unsupported combinations should fail with source-aware diagnostics when CJM can
know the relevant source location:

```text
include/user.hpp:12:18: backend simdjson does not support std::map<std::string, Address> yet
```

When source location is unavailable, diagnostics should still identify:

- selected backend
- field name or model name
- Metadata IR type category
- unsupported capability or type combination

---

# Default Backend

Until a later milestone deliberately changes this, the default runtime JSON
backend remains:

```text
nlohmann
```

Reasons:

- it is the official compatibility backend
- it is readable and easy to inspect
- it has broad ecosystem familiarity
- it keeps early adopter workflows stable while experimental backends mature

Experimental backends must be selected explicitly.

---

# Invariants

Static backend selection must preserve these invariants:

- all backends consume normalized Metadata IR
- runtime semantics are shared through the runtime semantic profile
- conformance fixtures define behavior before backend-specific claims
- backend dependencies stay optional
- backend C++ standard requirements stay isolated
- unsupported backend capabilities fail before user compiler errors when
  practical
- no universal runtime facade is introduced before real backend evidence proves
  a shared code-level abstraction is necessary

---

# Relationship To Other Documents

This design depends on:

- [Backend Strategy](backend-strategy.md)
- [High-Performance JSON Strategy](high-performance-json-strategy.md)
- [Binary Format Strategy](binary-format-strategy.md)
- [Runtime Backend Program](runtime-backend-program.md)
- [Runtime JSON Semantic Profile](runtime-json-semantic-profile.md)
- [Runtime Decode Error Model](runtime-decode-error-model.md)
- [Runtime Conformance Fixture Layout](runtime-conformance-fixtures.md)

It feeds:

- simdjson On-Demand decode spike
- future backend capability matrices
- future CMake backend option implementation
- future backend promotion reviews
