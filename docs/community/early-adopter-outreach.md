# Early-Adopter Outreach

CJM is ready for a focused v0.5 early-adopter outreach round.

The goal is not broad marketing. The goal is to find a few experienced Modern
C++ developers who will try CJM on practical model headers and report real
friction around parsing, CMake integration, generated C++ code, and generated
JSON Schema artifacts.

## Target Audience

Look for developers who:

- use Modern C++;
- use CMake;
- write JSON serialization code by hand today;
- maintain model-heavy code;
- care about generated code being readable;
- may benefit from JSON Schema artifacts for DTO review or downstream tooling;
- are willing to report small reproducible cases.

The target is 5 to 10 serious early adopters, not a large star spike.

## Core Message

```text
CJM is a build-time metadata compiler for Modern C++.

It reads ordinary C++ model declarations with Go-style JSON field metadata,
builds a Metadata IR, and generates ordinary nlohmann/json integration code
during the build.

v0.5 also adds opt-in JSON Schema Draft 2020-12 artifacts from the same
validated Metadata IR.

No macros. No runtime reflection. Standard C++ in, standard C++ out.
```

## What To Ask For

Ask early adopters for:

- practical model headers;
- CMake integration feedback;
- parser failure cases;
- confusing diagnostics;
- generated-code readability feedback;
- generated JSON Schema shape feedback;
- missing type mappings needed before v1.0.

## What Not To Claim

Do not claim:

- production stability;
- full C++ grammar support;
- that CJM is a JSON library;
- automatic header discovery;
- broad runtime backend support;
- benchmark wins;
- OpenAPI route generation;
- runtime JSON Schema validation;
- cross-language code generation owned by CJM;
- that CJM replaces existing JSON libraries.

## Short Announcement

````md
CJM v0.5.0 is ready for early adopters.

CJM is a build-time metadata compiler for Modern C++. It reads ordinary C++
models annotated with Go-style JSON field metadata, builds a Metadata IR, and
generates ordinary nlohmann/json integration code during the build.

v0.5.0 adds an opt-in JSON Schema backend. The same validated Metadata IR can now
produce JSON Schema Draft 2020-12 artifacts through:

```sh
cjm generate-schema --input user.hpp --output user.schema.json
```

or CMake:

```cmake
cjm_generate(
  TARGET app
  HEADERS user.hpp
  GENERATED_TARGET app_cjm_generated
  GENERATE_SCHEMAS
  GENERATED_SCHEMAS_VAR app_cjm_schemas
)
```

I am looking for a few Modern C++ developers to try CJM on practical model
headers and report parser, CMake, diagnostics, generated-code, or schema-output
friction.

GitHub: https://github.com/cjmworks/cxx-json-codegen
Release: https://github.com/cjmworks/cxx-json-codegen/releases/tag/v0.5.0
Feedback thread: https://github.com/cjmworks/cxx-json-codegen/discussions/163
Dogfood report: https://github.com/cjmworks/cxx-json-codegen/blob/main/docs/dogfood/ull-md-engine-v0.3.0.md
````

## Reddit r/cpp Draft

Title:

```text
CJM v0.5.0: C++ metadata compiler with nlohmann/json and JSON Schema output
```

Post:

````md
Hi r/cpp,

I have been building CJM, a build-time metadata compiler for Modern C++.

The idea is not to create another JSON library. CJM reads ordinary C++ model
headers with Go-style JSON field metadata, builds a Metadata IR, and generates
ordinary nlohmann/json integration code during the build.

v0.5.0 adds a second backend: JSON Schema Draft 2020-12 artifacts generated from
the same validated Metadata IR.

Example:

```cpp
struct User {
    std::string name; // json:"name"
    std::uint64_t id; // json:"id"
};
```

CJM can generate normal C++ `to_json` / `from_json` code and, when requested, a
schema artifact for the supported DTO mapping surface. There are no macros,
compiler plugins, or runtime reflection systems in user code.

The current release supports a practical subset:

- scalar fields and strings
- fixed-width integers
- enums as JSON strings
- nested generated structs
- `std::vector<T>`
- `std::array<T, N>`
- `std::optional<T>`
- `std::map<std::string, T>`
- `std::unordered_map<std::string, T>`
- `json:"-"` ignored fields
- `omitempty`
- opt-in JSON Schema artifacts for supported mappings

This is still an early-adopter project, not a production-stability claim. The
most useful feedback right now is not stars, but real failure cases:

- parser limitations on ordinary model headers
- CMake integration friction
- confusing diagnostics
- generated-code readability issues
- generated schema shapes that do not match practical downstream needs
- missing type mappings needed before v1.0

Repo:
https://github.com/cjmworks/cxx-json-codegen

Release:
https://github.com/cjmworks/cxx-json-codegen/releases/tag/v0.5.0

Discussions:
https://github.com/cjmworks/cxx-json-codegen/discussions/163

If you maintain C++ code with model structs and JSON integration, I would love
to hear where this approach works or fails for you.
````

## Direct Message

```md
Hi, I am looking for early feedback on CJM v0.5.0, a build-time metadata
compiler for Modern C++ JSON code generation.

It reads ordinary C++ model headers with Go-style JSON metadata, builds a
Metadata IR, and generates ordinary nlohmann/json integration during the build.
v0.5 also adds opt-in JSON Schema artifacts from that same IR.

I am looking for a few C++ developers to try it on practical model headers.

Repo: https://github.com/cjmworks/cxx-json-codegen
Release: https://github.com/cjmworks/cxx-json-codegen/releases/tag/v0.5.0
Feedback thread: https://github.com/cjmworks/cxx-json-codegen/discussions/163

If you have a small model header that would be a good test case, feedback would
be very helpful.
```

## Response Checklist

When someone gives feedback:

1. Thank them.
2. Ask for the smallest model header if missing.
3. Ask whether they used CLI or CMake integration.
4. Ask whether schema output was involved.
5. Classify the feedback as parser, semantic, generator, schema, CMake,
   diagnostics, or docs.
6. Create a focused issue if it is actionable.
7. Avoid promising broad features before they fit the roadmap.
