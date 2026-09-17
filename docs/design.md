# cxx-json-map Design

Historical design notes. For current architecture and scope, see
[ARCHITECTURE.md](../ARCHITECTURE.md) and the
[JSON-first extension strategy](design/json-first-extension-strategy.md).
Early multi-format ideas below are not current product commitments.

## Problem

C++ does not currently provide a standard Go-style struct tag mechanism for
JSON mapping.

In Go, JSON field mapping can be expressed directly on a struct field:

```go
type Result struct {
    ElapsedNs uint64 `json:"elapsed_ns"`
}
```

In C++, the common alternatives are:

- handwritten `to_json` / `from_json`
- macro-based field registration
- library-specific reflection tricks
- intrusive serialization frameworks

These approaches work, but they either become repetitive or hide source-level
structure behind macro magic.

`cxx-json-map` explores a compiler-tooling approach:

```text
C++ source
-> Clang AST discovery
-> metadata model
-> generated JSON mapping code
```

## Product Goal

The first product goal is intentionally narrow:

```text
Generate explicit JSON mapping code for annotated C++ structs.
```

The generated code should be readable, reviewable, and compilable by a normal
C++ compiler.

The first backend is:

```text
nlohmann/json
```

## Non-Goals For MVP

The MVP does not attempt to implement:

- a full C++ reflection system
- a general serialization framework
- XML/YAML/TOML emitters
- LLVM IR transformation
- Clang compiler plugin integration
- runtime reflection
- private field access
- inheritance-aware object mapping

## Why Clang LibTooling

JSON mapping needs source-level information:

- struct names
- namespaces
- field names
- field types
- attributes
- source locations
- access control

This information is naturally available in Clang AST.

LLVM IR is not the right first layer because many C++ concepts have already
been lowered or erased by that point. Field names may only exist in debug
metadata, which is optional and fragile.

A standalone Clang LibTooling executable also keeps integration simple:

```text
headers
-> codegen tool
-> generated header/source
-> normal build
```

This is similar in spirit to protobuf, flatbuffers, and capnproto.

## Initial User Model

Example input:

```cpp
#include <cstdint>
#include <string>

#include "cxx_json_map/annotations.hpp"

struct [[cjm::json]] ConsumerLatency {
  std::uint64_t consumer_id;
  std::uint64_t count;
  std::uint64_t p50_ns;
};
```

Example generated output:

```cpp
inline void to_json(nlohmann::json &j, const ConsumerLatency &v) {
  j = nlohmann::json{
      {"consumer_id", v.consumer_id},
      {"count", v.count},
      {"p50_ns", v.p50_ns},
  };
}
```

Future field rename support may look like:

```cpp
struct [[cjm::json]] ConsumerLatency {
  [[cjm::name("consumer")]]
  std::uint64_t consumer_id;
};
```

Generated output:

```cpp
{"consumer", v.consumer_id}
```

## Architecture

The desired internal architecture is:

```text
Clang AST frontend
-> StructModel / FieldModel / TypeModel
-> nlohmann/json emitter
```

Possible future architecture:

```text
frontends:
  clang_ast
  future_standard_reflection

intermediate model:
  StructModel
  FieldModel
  TypeModel
  AnnotationModel

backends:
  nlohmann_json
  rapidjson
  glaze
  custom
```

The MVP should implement only:

```text
clang_ast frontend
+
nlohmann_json backend
```

## JSON vs Other Formats

The repository name is intentionally JSON-specific because JSON is the first
real product target.

The internal model should not unnecessarily hard-code JSON concepts, but the
project should not add XML/YAML/TOML support before JSON code generation is
useful, tested, and documented.

XML in particular has a different data model:

- elements
- attributes
- text nodes
- namespaces
- ordering
- mixed content

That would require a richer annotation system and should not shape the MVP.

## First Implementation Milestones

1. Project skeleton and documentation
2. Minimal annotations header
3. CLI that accepts input and output paths
4. Clang LibTooling setup
5. Discover annotated structs
6. Extract public fields
7. Build an internal metadata model
8. Emit `to_json` for primitive fields
9. Add golden-file tests
10. Add nested struct and `std::vector<T>` support
