# Early-Adopter Launch Posts

These drafts are for CJM's v0.5 early-adopter outreach round.

The goal is to recruit practical feedback, not to run a broad marketing launch.
Keep the tone technical, honest, and specific.

## Positioning

Use this core positioning everywhere:

```text
CJM is a build-time metadata compiler for Modern C++.
It keeps C++ models as the source of truth and generates backend artifacts from
validated Metadata IR.
```

Current v0.5 artifact outputs:

- ordinary `nlohmann/json` C++ integration;
- opt-in JSON Schema Draft 2020-12 artifacts.

Avoid:

- "production ready";
- "new JSON library";
- "full C++ support";
- "best/fastest";
- benchmark claims;
- OpenAPI framework claims;
- runtime validation claims;
- broad backend promises.

Ask for:

- practical C++ model headers;
- CMake integration feedback;
- parser failure cases;
- confusing diagnostics;
- generated-code readability feedback;
- generated JSON Schema shape feedback.

## Links

- Repo: https://github.com/cjmworks/cxx-json-codegen
- Release: https://github.com/cjmworks/cxx-json-codegen/releases/tag/v0.5.0
- v0.5 release notes: https://github.com/cjmworks/cxx-json-codegen/blob/main/docs/releases/v0.5.0.md
- Dogfood report: https://github.com/cjmworks/cxx-json-codegen/blob/main/docs/dogfood/ull-md-engine-v0.3.0.md
- v0.5 feedback thread: https://github.com/cjmworks/cxx-json-codegen/discussions/163
- Discussions: https://github.com/cjmworks/cxx-json-codegen/discussions
- Issues: https://github.com/cjmworks/cxx-json-codegen/issues
- Website: https://cjmworks.org

## Reddit r/cpp

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
backend artifacts during the build.

The first C++ backend generates ordinary `nlohmann/json` `to_json` / `from_json`
integration. v0.5.0 adds an opt-in JSON Schema Draft 2020-12 backend that
consumes the same validated Metadata IR.

Example:

```cpp
struct User {
    std::string name;       // json:"name"
    std::uint64_t id = 0;   // json:"id"
};
```

CJM can generate normal C++ integration code and, when requested, a schema
artifact for the supported DTO mapping surface. There are no macros, compiler
plugins, or runtime reflection systems involved in user code.

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

It has been dogfooded in a real downstream CMake project through the public
`FetchContent + cjm_generate` workflow.

This is still an early-adopter project, not a production-stability claim. The
most useful feedback right now would be:

- parser limitations on ordinary headers
- CMake integration friction
- confusing diagnostics
- generated-code readability issues
- generated schema shapes that do not match practical downstream needs
- type mappings needed before v1.0

Repo:
https://github.com/cjmworks/cxx-json-codegen

Release:
https://github.com/cjmworks/cxx-json-codegen/releases/tag/v0.5.0

Discussions:
https://github.com/cjmworks/cxx-json-codegen/discussions/163

If you maintain C++ code with model structs and JSON integration, I would love
to hear where this approach works or fails for you.
````

## Reddit r/cpp Shorter Variant

Title:

```text
Looking for feedback on CJM, a build-time C++ metadata compiler
```

Post:

````md
I am looking for early feedback on CJM v0.5.0:

https://github.com/cjmworks/cxx-json-codegen

CJM is a build-time metadata compiler for Modern C++. It reads ordinary C++
model headers with Go-style JSON field metadata, builds a Metadata IR, and
generates ordinary `nlohmann/json` `to_json` / `from_json` code.

v0.5.0 also adds opt-in JSON Schema Draft 2020-12 artifacts from the same
validated Metadata IR.

Example:

```cpp
struct Event {
    std::string name; // json:"name"
    std::optional<std::string> note; // json:"note,omitempty"
    std::map<std::string, std::uint64_t> counters; // json:"counters"
};
```

The current release supports optionals, vectors, arrays, string-keyed maps,
nested generated structs, enums, ignored fields, `omitempty`, fixed-width
integers, and schema artifacts for the supported mapping surface.

This is still an early-adopter project. I am especially interested in practical
model headers that break the parser, confusing diagnostics, CMake integration
friction, and schema output that does not match downstream needs.
````

## LinkedIn Post

```md
I have started the v0.5 early-adopter round for CJM.

CJM is a build-time metadata compiler for Modern C++. It reads ordinary C++
model declarations with Go-style JSON field metadata, builds a Metadata IR, and
generates backend artifacts during the build.

The first C++ backend targets nlohmann/json. v0.5 adds opt-in JSON Schema Draft
2020-12 artifacts from the same validated Metadata IR.

The design goal is simple:

Standard C++ in. Standard C++ out.

No macros in user models.
No runtime reflection.
No schema-first model duplication.

The current practical mapping surface includes scalar fields, strings,
fixed-width integers, enums, nested generated structs, vectors, arrays,
optionals, string-keyed maps, ignored fields, and omitempty.

I am looking for a small number of Modern C++ developers to try CJM on practical
model headers and tell me what breaks:

- parser limitations
- CMake integration friction
- confusing diagnostics
- generated-code readability
- generated schema shape
- missing type mappings before v1.0

Repo:
https://github.com/cjmworks/cxx-json-codegen

Release:
https://github.com/cjmworks/cxx-json-codegen/releases/tag/v0.5.0

Discussions:
https://github.com/cjmworks/cxx-json-codegen/discussions/163
```

## LinkedIn Short Variant

```md
CJM v0.5.0 is ready for early-adopter feedback.

CJM is a build-time metadata compiler for Modern C++. It keeps ordinary C++
models as the source of truth, builds a Metadata IR, and generates backend
artifacts during the build.

Current outputs:

- ordinary nlohmann/json integration code
- opt-in JSON Schema Draft 2020-12 artifacts

The goal is not to build another JSON library or schema-first model generator.
The goal is a compiler-style pipeline:

C++ source -> Metadata IR -> generated C++ / generated schema

I am looking for a few C++ developers to try it on practical model headers and
share feedback before v1.0.

Repo:
https://github.com/cjmworks/cxx-json-codegen
```

## Chinese Short Post

```md
CJM v0.5.0 现在进入新一轮 early-adopter feedback。

CJM 是一个面向 Modern C++ 的 build-time metadata compiler。它从普通 C++ model
声明中读取 Go-style JSON metadata，构建 Metadata IR，然后在构建期生成普通
C++ 的 nlohmann/json 集成代码。

v0.5.0 新增了 opt-in JSON Schema Draft 2020-12 输出，同样来自已经验证过的
Metadata IR。

它不是新的 JSON library，也不是 runtime reflection，也不是 OpenAPI framework。

目标是：

Standard C++ in. Standard C++ out.

当前 practical mapping surface 覆盖 scalar/string、fixed-width integer、enum、
nested struct、vector、array、optional、string-keyed map、ignore、omitempty，以及
这些受支持映射的 schema artifact。

现在想找少量真正写 Modern C++ / CMake 的开发者试用，重点不是 Star，而是真实反馈：

- parser 哪里不够用；
- CMake 接入哪里别扭；
- diagnostics 哪里看不懂；
- generated code 是否可读；
- generated schema shape 是否符合下游工具需要；
- v1.0 前还缺哪些 practical mapping。

Repo:
https://github.com/cjmworks/cxx-json-codegen

Release:
https://github.com/cjmworks/cxx-json-codegen/releases/tag/v0.5.0

Discussions:
https://github.com/cjmworks/cxx-json-codegen/discussions/163
```

## GitHub Discussions Feedback Thread

Published thread:

https://github.com/cjmworks/cxx-json-codegen/discussions/163

Original title:

`CJM v0.5.0 early-adopter feedback thread`

Original body:

```md
CJM v0.5.0 is ready for early-adopter feedback.

CJM is a build-time metadata compiler for Modern C++. It reads ordinary C++
model declarations with Go-style JSON field metadata, builds a Metadata IR, and
generates backend artifacts during the build.

Current outputs:

- ordinary `nlohmann/json` C++ integration
- opt-in JSON Schema Draft 2020-12 artifacts

This thread is for open-ended usage and adoption feedback:

- Would CJM fit any model-heavy C++ code you maintain?
- Does the `FetchContent + cjm_generate` workflow feel reasonable?
- Would generated JSON Schema artifacts help your downstream tooling?
- What parser limitations would block your real headers?
- Are the current diagnostics understandable?
- What type mappings are missing before v1.0?
- Does the generated code look readable enough to trust?
- Does the generated schema shape match what you would expect?

Useful links:

- Repo: https://github.com/cjmworks/cxx-json-codegen
- v0.5.0 release: https://github.com/cjmworks/cxx-json-codegen/releases/tag/v0.5.0
- v0.5.0 release notes: https://github.com/cjmworks/cxx-json-codegen/blob/main/docs/releases/v0.5.0.md
- Dogfood report: https://github.com/cjmworks/cxx-json-codegen/blob/main/docs/dogfood/ull-md-engine-v0.3.0.md

If you try CJM, please share the smallest practical model that worked or failed.
```

## Short Direct Message

```md
Hi, I am looking for early feedback on CJM v0.5.0, a build-time metadata
compiler for Modern C++ JSON code generation.

It reads ordinary C++ model headers with Go-style JSON metadata, builds a
Metadata IR, and generates ordinary nlohmann/json integration during the build.
v0.5 also adds opt-in JSON Schema artifacts from that same IR.

Repo: https://github.com/cjmworks/cxx-json-codegen
Release: https://github.com/cjmworks/cxx-json-codegen/releases/tag/v0.5.0
Feedback thread: https://github.com/cjmworks/cxx-json-codegen/discussions/163

If you have a small model header that would be a good test case, feedback would
be very helpful.
```

## Posting Order

Suggested order:

1. Share the GitHub Discussions thread.
2. Share the LinkedIn post.
3. Post to Reddit only after the repo homepage and feedback thread are ready.
4. Do not post to Hacker News yet.

## Follow-Up Rule

When someone responds, optimize for learning:

1. Ask for the smallest model header.
2. Ask how they tried to integrate CJM.
3. Ask whether generated schema output was involved.
4. Move concrete failures into GitHub Issues.
5. Keep broad design discussion in GitHub Discussions.
6. Do not promise features outside the roadmap.
