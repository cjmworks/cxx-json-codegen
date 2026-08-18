# CJM

> Build-time Metadata Compiler for Modern C++

**CJM** is a build-time metadata compiler for Modern C++.

It extracts source-level metadata from ordinary C++ declarations, builds a
stable Metadata IR, and generates backend-specific build artifacts.

The first official C++ backend generates `nlohmann/json` integration from
Go-style field metadata. CJM can also emit JSON Schema artifacts for the
supported Metadata IR surface.

Instead of writing repetitive integration code or relying on macros and runtime
reflection, CJM generates ordinary C++ code while keeping your source files
valid, standard C++.

CJM keeps your C++ models as the source of truth. It generates the JSON integration around them, not the models themselves.

> **Standard C++ in. Standard C++ out.**

CJM targets **strongly typed, structured, modelable JSON**. It intentionally
does not try to support arbitrary dynamic JSON documents or arbitrary C++
object graphs.
Dynamic inputs should be validated and normalized by user code into concrete
C++ model types before CJM-generated integration is used.

---

## Early Adopters Welcome

CJM v0.5.2 is the latest release for early adopters who want to try build-time
JSON integration, default field mapping, recursive generated JSON Schema, and
the documented runtime-semantics foundation for ordinary Modern C++ models.

The v0.4.0 workflow has been dogfooded through the public `FetchContent` +
`cjm_generate` workflow in
[ull-md-engine](https://github.com/lmingzhi618/ull-md-engine), covering
optionals, vectors, ordered and unordered string-keyed maps, nested generated
structs, enums, ignored fields, `omitempty`, fixed-width integers, fixed-size
arrays, enum string output, and generated model-contract metadata for downstream
tools. v0.5 adds an opt-in JSON Schema backend through the CLI and CMake
workflow, v0.5.1 makes same-name JSON field tags optional, and v0.5.2 completes
recursive schema coverage and the documented v0.6 runtime foundation.

If CJM fails on a practical model you expected to work, or if the CMake,
diagnostics, or documentation feel confusing, please open an issue with the
smallest reproducible header and build command.

For open-ended usage or design feedback, use
[GitHub Discussions](https://github.com/cjm-labs/cxx-json-codegen/discussions).

See [CONTRIBUTING.md](CONTRIBUTING.md) for bug reports, questions, feature
requests, and pull request guidance.

See the [ull-md-engine dogfood report](docs/dogfood/ull-md-engine-v0.3.0.md)
for the current downstream validation record.

---

## Why CJM?

Modern C++ still lacks a simple and ergonomic way to associate metadata with user-defined types.

Other languages provide elegant solutions:

- **Go** → Struct Tags
- **Rust** → Derive Macros
- **C#** → Attributes
- **Java** → Annotations

C++ developers often have to choose between:

- intrusive macros
- handwritten serialization code
- runtime reflection libraries
- compiler-specific extensions

CJM aims to provide a compiler-style metadata pipeline while preserving a
Go-like developer experience for its first JSON backend:

- Build-time only
- Standard C++
- Compiler independent from the user's perspective
- Easy to integrate with existing CMake projects

---

## Usage Example

Write ordinary C++ models. Same-name JSON fields do not need metadata:

```cpp
#pragma once

#include <string>

struct User {
    std::string name;
    int age = 0;
};
```

After building the CLI, run CJM:

```sh
./build/cjm generate \
  --input user.hpp \
  --output user.cjm.hpp
```

CJM generates ordinary C++ integration code:

```cpp
inline void to_json(nlohmann::json& j, const User& value) {
    j["name"] = value.name;
    j["age"] = value.age;
}

inline void from_json(const nlohmann::json& j, User& value) {
    j.at("name").get_to(value.name);
    j.at("age").get_to(value.age);
}
```

Use it like normal `nlohmann/json` code:

```cpp
#include "user.hpp"
#include "user.cjm.hpp"

#include <nlohmann/json.hpp>

int main() {
    User user;
    user.name = "Ada";
    user.age = 42;

    nlohmann::json json = user;
    User round_trip = json.get<User>();
}
```

No macros. No compiler plugins. No runtime reflection.

---

## Quickstart

The fastest way to try CJM today is from this repository checkout.

Requirements:

- CMake
- a C++17 compiler
- `nlohmann/json` available locally or downloadable by CMake

### Run the Example

```sh
cmake -S . -B build
cmake --build build --target cjm_basic_example
./build/examples/basic/cjm_basic_example
```

That builds the CJM CLI, generates `user.cjm.hpp`, compiles the example, and
runs a tiny JSON round trip.

The example model is ordinary C++:

```cpp
#pragma once

#include <string>

struct User {
    std::string name;
    int age = 0;
};
```

### Generate Code Manually

You can also run the local CLI directly:

```sh
cmake --build build --target cjm
./build/cjm generate \
  --input examples/basic/user.hpp \
  --output /tmp/user.cjm.hpp
```

The generated file contains ordinary C++ `to_json` and `from_json` functions
for `nlohmann/json`.

For multiple related headers, pass them after `--input` in the same command:

```sh
./build/cjm generate \
  --input address.hpp user.hpp \
  --output model.cjm.hpp
```

Repeating `--input` is still supported for compatibility.

CJM does not automatically discover `#include` dependencies yet, so pass every
related model header explicitly.

A less repetitive file-list or manifest-based input workflow is planned for a
future adoption milestone.

### Use the Generated Header

Include your model first, then the generated CJM header:

```cpp
#include "user.hpp"
#include "user.cjm.hpp"

#include <nlohmann/json.hpp>

int main() {
    User user;
    user.name = "Ada";
    user.age = 42;

    nlohmann::json json = user;
    User round_trip = json.get<User>();
}
```

### Use CJM From CMake

Inside this source tree, the example uses `cjm_generate`:

```cmake
add_executable(app main.cpp)

target_link_libraries(app PRIVATE nlohmann_json::nlohmann_json)

cjm_generate(
  TARGET app
  HEADERS user.hpp
)
```

In this snippet:

- `app` is your normal C++ executable.
- `nlohmann_json::nlohmann_json` provides the JSON library used by the
  generated backend code.
- `cjm_generate` wires CJM into the build: it reads `user.hpp`, generates
  `user.cjm.hpp`, and makes the generated header available to `app`.

During the build, CJM generates:

```text
generated/cjm/user.cjm.hpp
```

and adds the generated directory to the target include path.

If another target needs to consume the generated CJM headers, ask
`cjm_generate` to expose the generated artifact contract:

```cmake
cjm_generate(
  TARGET app
  HEADERS user.hpp
  GENERATED_TARGET app_cjm_generated
  GENERATED_HEADERS_VAR app_cjm_headers
  GENERATED_INCLUDE_DIR_VAR app_cjm_include_dir
)

add_executable(tool tool.cpp)
add_dependencies(tool app_cjm_generated)
target_sources(tool PRIVATE ${app_cjm_headers})
target_include_directories(tool PRIVATE ${app_cjm_include_dir})
```

This keeps generated files in the build directory while giving downstream
targets a stable dependency and include path.

### Generate JSON Schema

CJM can also generate JSON Schema Draft 2020-12 artifacts from the same
validated Metadata IR used by the C++ backend:

```sh
cjm generate-schema --input user.hpp --output user.schema.json
```

Schema generation is opt-in for CMake builds. Add `GENERATE_SCHEMAS` when
calling `cjm_generate`:

```cmake
cjm_generate(
  TARGET app
  HEADERS user.hpp
  GENERATED_TARGET app_cjm_generated
  GENERATE_SCHEMAS
  GENERATED_SCHEMAS_VAR app_cjm_schemas
)
```

During the build, CJM keeps generated C++ headers and schema artifacts in
separate output directories:

```text
generated/cjm/user.cjm.hpp
generated/schemas/user.schema.json
```

The generated schema files are build artifacts. They are tracked by the
generated target, but they are not C++ sources and are not added to the target
include path.

CJM schema generation describes supported DTO mappings. It does not generate
OpenAPI routes, HTTP endpoint policy, cross-language model code, or a runtime
JSON Schema validation engine.

### Use CJM From A Downstream Project

Early adopters can consume CJM by pinning a release tag with CMake
`FetchContent`:

```cmake
include(FetchContent)

FetchContent_Declare(
  cxx_json_codegen
  GIT_REPOSITORY https://github.com/cjm-labs/cxx-json-codegen.git
  GIT_TAG v0.5.2
)

FetchContent_MakeAvailable(cxx_json_codegen)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE nlohmann_json::nlohmann_json)

cjm_generate(
  TARGET app
  HEADERS user.hpp
)
```

This is the workflow used by the downstream `ull-md-engine` dogfood run.
Packaged installation through `find_package(CJM REQUIRED)` is planned later.

### Generated Model Contract

Generated `*.cjm.hpp` headers also include experimental model-contract metadata
through `cjm::contract::model_traits<T>`.

Downstream tools can inspect supported model facts without depending on CJM
parser internals:

```cpp
#include "user.hpp"
#include "user.cjm.hpp"

#include <cstdint>

const auto& model = cjm::contract::model_traits<User>::model;

for (std::uint32_t i = 0; i < model.field_count; ++i) {
    const auto& field = model.fields[i];
    // field.cpp_name, field.json_name, field.type, field.location
}
```

The current experimental contract exposes field names, JSON names, ignored
fields, `omitempty`, source locations, type categories, container arguments, and
enum string values. It is intended for downstream experiments and may still
change before v1.0.

### Try the Full Test Suite

```sh
ctest --test-dir build --output-on-failure
```

### Developer Test Workflow

Configure a Debug test build before doing focused C++ test work:

```sh
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug
ctest --test-dir build/debug --output-on-failure
```

New C++ unit tests use Catch2 and are discovered as individual CTest cases.
Run one named case through CTest with its target-prefixed name:

```sh
ctest --test-dir build/debug \
  -R '^cjm_metadata_named_tests\.field_type\.records_array_extent$' \
  --output-on-failure
```

You can also run the Catch2 executable directly with the shorter case name:

```sh
./build/debug/tests/cjm_metadata_named_tests \
  "field_type.records_array_extent"
```

CTest remains the repository-wide runner. Existing standalone `main()` and
`assert`-based tests continue to run beside Catch2-discovered cases.

To verify a production-style build without the test framework:

```sh
cmake -S . -B build/no-tests -DCJM_BUILD_TESTS=OFF
cmake --build build/no-tests --target cjm
```

### Keep the First Try Smooth

Current parser notes:

- CJM uses a Tree-sitter-backed C++ frontend internally
- write supported managed field declarations in the documented practical subset
- put `json:"..."` metadata in a same-line `//` comment only when renaming,
  adding `omitempty`, or ignoring a field
- use qualified standard types such as `std::string`
- pass every related header explicitly with `--input`

---

## Design Philosophy

CJM follows a few core principles:

- Standard C++ in.
- Standard C++ out.
- Build-time code generation.
- CMake-first workflow.
- Keep implementation details hidden from users.
- Make generated code readable and debuggable.

---

## Architecture

```
User C++ Source
        |
        v
 C++ Frontend
        |
        v
 Metadata IR
    |          |
    v          v
 nlohmann     JSON Schema
 Backend      Backend
    |          |
    v          v
 Generated    Generated
 C++          Schema
    |
    v
 Normal Compiler
    |
    v
 Executable
```

The Metadata IR is the stable boundary between source-language understanding
and backend-specific code generation.

Users only interact with standard C++ source code, CMake, generated C++ files,
and optional generated schema artifacts.

---

## Project Status

Current status:

- v0.5.0 completed the first JSON Schema backend
- v0.5.1 completed default field mapping and canonical field semantics
- v0.5.2 completed recursive schema coverage and the v0.6 runtime foundation
- the first v0.6 simdjson On-Demand scalar decode spike is complete and proved
  that Metadata IR can drive an optional, one-pass generated decoder
- the C++17 simdjson native custom-type baseline is complete and remains a
  test-only comparison for API and error-shape differences
- the simdjson generated decode vertical slice is complete internally and
  verifies owned strings, optional integer presence, and one required nested
  generated model through generated C++17
- CJM has a parser -> semantic analysis -> Metadata IR pipeline with
  `nlohmann/json`, generated model-contract, and JSON Schema backends
- First official C++ runtime integration backend: `nlohmann/json`
- First schema artifact backend: JSON Schema Draft 2020-12
- CJM v0.3.0 has been dogfooded in a real downstream CMake project:
  [ull-md-engine](https://github.com/lmingzhi618/ull-md-engine)
- The supported model surface is still a documented practical subset, not full
  C++ grammar support

Next planned line:

- v0.6 continues with decode MVP scoping, encode spike planning, and backend
  promotion evidence review
- the first public v0.6 tag will be assigned only when the result is a
  user-consumable capability snapshot

No simdjson, Glaze, or yyjson backend is available in the v0.5.2 release.
The merged simdjson code remains internal research evidence: it has no public
backend-selection path and makes no performance claim.

---

## Current Mapping Surface

CJM currently supports a practical JSON mapping subset.

Supported:

- explicit CMake header registration with `cjm_generate`
- one or more explicit input headers
- public `struct` declarations in the supported parser subset
- supported multiline field declarations in the Tree-sitter frontend subset
- fields default to their exact C++ field names
- fields with Go-style `json:"name"` comments when an explicit JSON name is
  needed
- generated `to_json` / `from_json` for `nlohmann/json`
- `bool`
- signed and unsigned integer types
- common fixed-width integer spellings from `<cstdint>`
- floating-point types
- `std::string`
- enums
- nested generated structs
- namespaces
- supported type aliases
- `std::vector<T>`
- `std::optional<T>`
- `std::map<std::string, T>`
- `std::unordered_map<std::string, T>`
- `std::array<T, N>`
- enum and enum class fields as JSON strings
- `json:"-"` ignored fields
- `json:",omitempty"` and explicit-name `omitempty` for supported optional fields

JSON Schema output currently covers:

- JSON Schema Draft 2020-12 object schemas for supported generated structs
- scalar and string fields
- unsigned integers with `minimum: 0`
- `std::vector<T>` and `std::array<T, N>` using recursive `schema(T)` for
  supported `T`
- `std::optional<T>` using `anyOf` with recursive `schema(T)` and `null`
- `std::map<std::string, T>` and `std::unordered_map<std::string, T>` when `T`
  has a supported recursive schema mapping
- enum and enum class fields as JSON string enums
- nested generated structs as `$ref` entries with `$defs`
- `json:"-"` fields omitted from schema properties
- non-optional supported fields listed in `required`
- default field names, explicit rename tags, `omitempty`, and ignored fields
  based on normalized Metadata IR

## Current Limitations

CJM intentionally remains a practical subset.

Not yet supported:

- full C++ parsing 
- automatic header discovery 
- arbitrary dynamic JSON values
- `std::variant`
- `std::any`
- pointer fields
- polymorphism
- custom converters
- custom enum string mapping policies
- time and datetime mappings
- OpenAPI route generation
- runtime JSON Schema validation
- private fields 
- native JSON backend 
- install/package distribution

---

## Roadmap

See [ROADMAP.md](ROADMAP.md) for the current product roadmap.

---

## Documentation

- [Project Vision](docs/vision.md)
- [Architecture](ARCHITECTURE.md)
- [Philosophy](docs/philosophy.md)
- [Contributing](CONTRIBUTING.md)
- [Discussions](DISCUSSIONS.md)
- [Roadmap](ROADMAP.md)
- [JSON Mapping Scope](docs/design/json-mapping-scope.md)
- [Default Field Mapping](docs/design/default-field-mapping.md)
- [Generated Model Contract](docs/design/generated-model-contract.md)
- [Custom Converter Boundaries](docs/design/custom-converters.md)
- [Backend Strategy](docs/design/backend-strategy.md)
- [High-Performance JSON Strategy](docs/design/high-performance-json-strategy.md)
- [Binary Format Strategy](docs/design/binary-format-strategy.md)
- [Runtime Backend Program](docs/design/runtime-backend-program.md)
- [Runtime JSON Semantic Profile](docs/design/runtime-json-semantic-profile.md)
- [Runtime Decode Error Model](docs/design/runtime-decode-error-model.md)
- [Runtime Conformance Fixture Layout](docs/design/runtime-conformance-fixtures.md)
- [Static Backend Selection](docs/design/static-backend-selection.md)
- [simdjson On-Demand Decode Spike](docs/design/simdjson-ondemand-decode-spike.md)
- [simdjson Generated Decode Vertical Slice](docs/design/simdjson-generated-decode-vertical-slice.md)
- [simdjson Experimental Backend MVP](docs/design/simdjson-experimental-backend-mvp.md)
- [Named C++ Test Infrastructure](docs/design/cpp-test-infrastructure.md)
- [ull-md-engine Dogfood Report](docs/dogfood/ull-md-engine-v0.3.0.md)
- [Early-Adopter Outreach](docs/community/early-adopter-outreach.md)
- [Early-Adopter Launch Posts](docs/community/early-adopter-launch-posts.md)
- [v0.5.2 Release Notes](docs/releases/v0.5.2.md)
- [v0.5.1 Release Notes](docs/releases/v0.5.1.md)
- [v0.5.0 Release Notes](docs/releases/v0.5.0.md)
- [v0.4.0 Release Notes](docs/releases/v0.4.0.md)
- [v0.3.6 Release Notes](docs/releases/v0.3.6.md)
- [v0.3.0 Release Notes](docs/releases/v0.3.0.md)
- [Competitive Landscape](docs/design/competitive-landscape.md)
- [Third-Party Notices](THIRD_PARTY_NOTICES.md)
- [Design Notes](docs/design/)

---

## About

CJM is an open-source project developed by **CJM Labs**.

🌐 https://cjm-labs.org

GitHub Organization:

https://github.com/cjm-labs

---

## License

CJM is licensed under the MIT License. See [LICENSE](LICENSE).

## Golden Tests

CJM uses golden tests to protect generated C++ output.

Golden files live under:

```text
tests/golden/
```

Expected files are committed:

```text
*.expected.cjm.hpp
```

Actual files are local failure artifacts:

```text
*.actual.cjm.hpp
```

When a golden test fails, the test writes an actual file and prints both paths:

```text
golden mismatch
expected: tests/golden/example.expected.cjm.hpp
actual: tests/golden/example.actual.cjm.hpp
```

To inspect the change:

```bash
diff -u tests/golden/example.expected.cjm.hpp tests/golden/example.actual.cjm.hpp
```

If the generated output change is intentional, update the expected file after review:

```bash
cp tests/golden/example.actual.cjm.hpp tests/golden/example.expected.cjm.hpp
```

Do not commit `*.actual.*` files. They are ignored by git and exist only to help inspect local failures.

New Catch2-based golden tests should use the test-only diagnostics helper under
`tests/support/golden_diff.hpp`. It reports the first mismatch line and column,
including missing or extra trailing content, without exposing test support
through production headers.
