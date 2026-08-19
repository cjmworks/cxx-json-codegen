# CMake Integration

CMake is the primary build-system integration target for CJM.

The goal is to make CJM feel like a native part of an existing CMake-based C++ project.

Users should not need to manually invoke the `cjm` executable during normal development.

---

# Goals

The CMake integration should provide:

- simple user-facing API
- automatic invocation of the CJM generator
- generated include directory management
- dependency tracking
- incremental rebuild support
- clear diagnostics
- minimal user configuration

---

# Basic Usage

A typical project should look like this:

```cmake
find_package(CJM REQUIRED)

add_executable(app
    main.cpp
)

cjm_generate(
    TARGET app
    HEADERS
        user.hpp
)
```

This is the target installed-package workflow. Until packaged installation
lands, early adopters should consume CJM through a pinned `FetchContent`
checkout and then call the same `cjm_generate(...)` function.

---

# User-Facing API

The primary CMake function is:

```cmake
cjm_generate(...)
```

Current signature:

```cmake
cjm_generate(
    TARGET <target>
    HEADERS <header>...
    [GENERATED_TARGET <target>]
    [GENERATED_HEADERS_VAR <var>]
    [GENERATED_INCLUDE_DIR_VAR <var>]
    [GENERATE_SCHEMAS]
    [GENERATED_SCHEMAS_VAR <var>]
)
```

Example:

```cmake
cjm_generate(
    TARGET app
    HEADERS
        include/user.hpp
        include/order.hpp
)
```

Downstream tooling targets may request generated artifact handles:

```cmake
cjm_generate(
    TARGET app
    HEADERS include/user.hpp
    GENERATED_TARGET app_cjm_generated
    GENERATED_HEADERS_VAR app_cjm_headers
    GENERATED_INCLUDE_DIR_VAR app_cjm_include_dir
)
```

- `GENERATED_TARGET` creates a custom target that depends on generated files.
- `GENERATED_HEADERS_VAR` stores the generated header list in the caller scope.
- `GENERATED_INCLUDE_DIR_VAR` stores the generated include directory in the
  caller scope.
- `GENERATE_SCHEMAS` requests JSON Schema artifacts for the listed headers.
- `GENERATED_SCHEMAS_VAR` stores the generated schema file list in the caller
  scope.

Schema output is opt-in. Existing calls that omit `GENERATE_SCHEMAS` continue to
generate only C++ headers.

---

# Header Input Modes

CJM currently supports explicit header input only.

## Explicit Header Mode

Users list headers manually.

This is the default and most predictable mode.

```cmake
cjm_generate(
    TARGET app
    HEADERS
        include/user.hpp
        include/order.hpp
)
```

Each listed header is treated as a CJM model input. The current CMake workflow
does not scan directories, recursively glob headers, or infer model inputs from
ordinary `#include` dependencies.

## Future Discovery Mode

Future versions may support automatic header discovery.

```cmake
cjm_generate(
    TARGET app
    DISCOVER
    ROOTS
        include
)
```

Discovery mode scans selected roots for CJM metadata.

It should never scan the entire project implicitly.

Explicit boundaries prevent accidental processing of third-party or generated headers.
---

# Generated Output

Generated files are placed under the build directory.

Example:

```text
<build-dir>/generated/cjm/user.cjm.hpp
<build-dir>/generated/cjm/order.cjm.hpp
```

The generated include directory is automatically added to the target.

Users should not need to manually include generated paths.

When `GENERATE_SCHEMAS` is requested, JSON Schema artifacts are placed in a
separate build output directory:

```text
<build-dir>/generated/schemas/user.schema.json
<build-dir>/generated/schemas/order.schema.json
```

Schema files are build artifacts. They are not C++ sources and are not added to
the generated include directory.

When requested, `cjm_generate()` also exposes the generated include directory to
the caller through `GENERATED_INCLUDE_DIR_VAR` so another target can consume the
same generated artifacts explicitly.

---

# Dependency Tracking

Each input header should map to one or more generated files.

CMake should regenerate output when:

- an input header changes
- CJM executable changes
- generator options change
- relevant dependencies change

The build should avoid unnecessary regeneration.

---

# Generated File Naming

Generated C++ headers use the convention:

```text
*.cjm.hpp
```

Example:

```text
user.hpp
    -> user.cjm.hpp
```

Generated JSON Schema artifacts use the convention:

```text
*.schema.json
```

Example:

```text
user.hpp
    -> user.schema.json
```

If multiple source files share the same base name, CJM should avoid collisions by preserving relative paths or using a deterministic output layout.

---

# Target Integration

`cjm_generate()` should attach generated files to the specified target.

Conceptually:

```cmake
target_include_directories(app PRIVATE <generated-dir>)
target_sources(app PRIVATE <generated-headers>)
```

When `GENERATED_TARGET` is provided, `cjm_generate()` should also create a
custom target that depends on the generated files:

```cmake
add_custom_target(app_cjm_generated DEPENDS <generated-artifacts>)
```

Another target can then depend on that target and include the generated
directory without relying on repository internals:

```cmake
add_dependencies(tool app_cjm_generated)
target_sources(tool PRIVATE ${app_cjm_headers})
target_include_directories(tool PRIVATE ${app_cjm_include_dir})
```

When schema generation is enabled, the generated target should also depend on
the schema files. Consumers can inspect those file paths through
`GENERATED_SCHEMAS_VAR`, but schema files should not be added through
`target_sources()` for normal C++ compilation.

---

# Build-Time Execution

CJM runs before the target is compiled.

Simplified flow:

```text
header changed

    ↓

run cjm generate

    ↓

produce *.cjm.hpp

    ↓

compile target

    ↓

link target
```

When `GENERATE_SCHEMAS` is enabled, the generated target also runs
`cjm generate-schema` and produces `*.schema.json` artifacts before downstream
targets that depend on the generated target are considered up to date.

---

# Compiler Independence

CJM may use a parser internally.

However, the user's target compiler remains independent.

The generated code should compile with any supported C++ compiler.

CMake integration should not require users to change their compiler.

---

# CLI Invocation

The CMake function invokes the CLI internally.

Example conceptual command:

```bash
cjm generate \
    --input include/user.hpp \
    --output <build-dir>/generated/cjm/user.cjm.hpp
```

Schema generation uses the sibling schema command:

```bash
cjm generate-schema \
    --input include/user.hpp \
    --output <build-dir>/generated/schemas/user.schema.json
```

Users normally should not run this manually.

---

# Build Directory Only

Generated files should live in the build directory by default.

They should not be written into the source tree.

This keeps the source tree clean and prevents accidental manual edits.

---

# Debug Mode

Future versions may support a debug option:

```cmake
cjm_generate(
    TARGET app
    HEADERS user.hpp
    DEBUG
)
```

Debug mode may:

- print the generator command
- preserve intermediate metadata
- emit verbose diagnostics
- make generated files easier to inspect

---

# Native Backend Direction

The first implementation may generate code targeting an existing JSON library.

However, the long-term goal is to support a CJM-native JSON backend.

This would reduce third-party dependency risk and improve product stability.

Possible long-term generated APIs:

```cpp
cjm::json_writer writer;
cjm::to_json(writer, value);
```

or:

```cpp
std::string json = cjm::to_json(value);
```

The MVP should not block on this.

The CMake API should remain stable as backends evolve.

---

# Future Options

Potential future options:

```cmake
cjm_generate(
    TARGET app
    HEADERS user.hpp
    JSON_BACKEND nlohmann
    OUTPUT_DIR <dir>
    NAMESPACE my_project
    STRICT
)
```

Options should be added conservatively.

The default experience should remain simple.

## Future Runtime Backend Selection

Runtime JSON backend selection should be static and target one generated C++
backend at a time:

```cmake
cjm_generate(
    TARGET app
    HEADERS user.hpp
    JSON_BACKEND simdjson
)
```

`JSON_BACKEND` selects generated runtime C++ integration. Artifact requests such
as `GENERATE_SCHEMAS` remain separate:

```cmake
cjm_generate(
    TARGET app
    HEADERS user.hpp
    JSON_BACKEND nlohmann
    GENERATE_SCHEMAS
)
```

Backend dependencies and C++ standard requirements should attach only to the
targets that select that backend. Selecting an experimental backend must not
raise the language standard or dependency set for default nlohmann users.

When multiple runtime backend outputs are requested for the same input, the
generated file layout must disambiguate by backend name:

```text
<build-dir>/generated/cjm/nlohmann/user.cjm.hpp
<build-dir>/generated/cjm/simdjson/user.cjm.hpp
```

CMake should add `<build-dir>/generated` to the consuming target include path.
Consumers should include runtime generated headers through the canonical
backend-qualified path:

```cpp
#include "cjm/nlohmann/user.cjm.hpp"
#include "cjm/simdjson/user.cjm.hpp"
```

The `generated` directory is a build output root. It should not appear in the
source-level include spelling.

See [Static Backend Selection](static-backend-selection.md) for the full design
contract.

---

# Error Reporting

Errors from CJM should appear as build errors.

Diagnostics should reference the original user source file.

Example:

```text
include/user.hpp:12:18: unsupported CJM field type: std::map<std::string, int>
```

Users should not need to inspect generated files to understand most errors.

---

# Summary

CMake integration is the primary user interface of CJM.

The user writes C++.

CMake invokes CJM.

CJM generates ordinary C++.

The user's compiler compiles the result.

The experience should feel native, predictable, and portable.
