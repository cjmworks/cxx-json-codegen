# CJM Discussions

CJM is entering an early-adopter phase.

The project is looking for practical feedback from developers who use Modern
C++ and CMake on real model headers.

## Best Discussion Topics

Good topics include:

- whether CJM fits your current C++ model workflow;
- CMake integration friction;
- parser limitations you hit with ordinary headers;
- diagnostics that were hard to understand;
- practical type mappings needed before v1.0;
- how generated code should look and feel;
- whether generated JSON Schema artifacts match your downstream tooling needs;
- where schema output should stop before becoming OpenAPI or runtime validation;
- downstream adoption stories.

## What To Share

The most helpful discussion includes:

- a small C++ model snippet;
- the JSON shape you want;
- the JSON Schema shape you expected, if schema generation is involved;
- whether you tried CLI or CMake integration;
- what worked;
- what felt awkward;
- what blocked you.

## Where To Post

Use [GitHub Discussions](https://github.com/cjmworks/cxx-json-codegen/discussions)
for open-ended design, adoption, and usage feedback.

The current v0.5 early-adopter feedback thread is:

https://github.com/cjmworks/cxx-json-codegen/discussions/163

Use GitHub Issues for concrete bugs, reproducible failures, and scoped feature
requests.

If you are not sure which one to use, open a question issue. The maintainers can
move or split the topic later.

## Current Positioning

CJM is not a JSON library.

CJM is a build-time metadata compiler for Modern C++. The first C++ backend
targets `nlohmann/json`, and v0.5 adds a JSON Schema artifact backend for the
supported Metadata IR surface.

The current release is intended for early adopters who want to try build-time
JSON integration, generated schema artifacts, and help shape the v1.0 path.
