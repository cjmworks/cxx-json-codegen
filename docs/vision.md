# CJM Vision

## Why CJM Exists

Modern C++ is one of the most powerful programming languages ever created.

It provides excellent performance, expressive abstractions, and complete control over system resources.

However, many everyday development tasks remain unnecessarily repetitive.

Serialization.

Configuration.

Metadata.

Reflection.

Schema generation.

Developers often spend significant effort writing code that machines could generate automatically.

CJM exists to eliminate that unnecessary work.

---

# A Better Developer Experience

The goal of CJM is not to replace C++.

The goal is to make modern C++ development feel simpler.

Developers should focus on solving business problems rather than writing repetitive boilerplate.

Code generation should become a natural part of the build process instead of something developers actively think about.

---

# Inspired by Simplicity

Many modern languages provide elegant metadata systems.

Go has struct tags.

Rust has derive macros.

C# has attributes.

Java has annotations.

These ideas dramatically improve developer productivity.

CJM aims to bring a similarly lightweight experience to C++ while preserving the language's strengths.

---

# Build-Time Instead of Runtime

Instead of introducing runtime reflection or runtime registration systems, CJM performs all analysis during the build.

The result is ordinary C++ code.

No hidden runtime.

No additional runtime cost.

No dynamic reflection engine.

The compiler remains responsible for optimization.

---

# Ordinary C++ Should Be Enough

One of CJM's core beliefs is that developers should not need to learn another language in order to use a code generator.

Users write ordinary C++.

The generated output is also ordinary C++.

Everything in between is the responsibility of CJM.

---

# Engineering Before Magic

CJM intentionally avoids "magic."

Generated code should be visible.

Behavior should be predictable.

Errors should be understandable.

Developers should always be able to inspect and debug the generated output.

Transparent systems are easier to trust.

---

# Build Tools Should Feel Native

A build-time tool should integrate naturally into existing workflows.

CJM is designed to work with existing CMake projects instead of replacing them.

Adding CJM to an existing project should feel like enabling another compiler tool rather than adopting a new framework.

---

# Long-Term Vision

The product focus remains model-first JSON code generation and metadata
compilation, including JSON Schema and Model Contract. Reusable infrastructure
does not imply a general-purpose multi-format serialization framework.

Current users write C++ models. A bounded pre-v1.0 C frontend research spike
will stress-test canonical IR with real C DTOs. Freeze a bounded C MVP from that
evidence and complete it before v1.0, alongside stable C++ JSON codegen. Keep
the Glaze JSON metadata adapter integration in its planned sequence; it must
not wait for C. Broader C capabilities remain v1.x work. Do not redesign IR
before the experiment demonstrates concrete gaps.

Glaze is a future optional JSON metadata adapter, not a mandatory dependency or
second IR. Verified extra formats remain ecosystem capabilities, not native
format commitments. See the [extension strategy](design/json-first-extension-strategy.md).

The first goal of CJM is build-time JSON code generation.

That is only the beginning.

More importantly, CJM establishes a reusable foundation for modern build-time code generation in C++.

As the project evolves, every new capability should continue to follow the same principles:

- Standard C++ in.
- Standard C++ out.
- Zero runtime overhead introduced by CJM.
- Simple user experience.
- Production-quality engineering.

---

# Success

CJM succeeds when developers forget that code generation is happening.

They simply write standard C++.

Run their normal build.

And everything works.

That is the experience CJM strives to deliver.
