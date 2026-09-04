# CJM Branding

This document defines the official branding and naming conventions of the CJM project.

The goal is to keep all public-facing components consistent across repositories, documentation, websites, releases, and future tooling.

---

# Product

**Official Product Name**

> CJM

CJM is the user-facing product brand.

Users should say:

> "We use CJM."

rather than referring to the repository name.

---

# Organization

**GitHub Organization**

```
cjmworks
```

Official GitHub Organization:

https://github.com/cjmworks

---

# Repository

Current repository:

```
cxx-json-codegen
```

The repository name describes the implementation.

The product name remains **CJM**.

---

# Website

Official website:

```
https://cjmworks.org
```

The website represents the entire CJM Works organization rather than an individual project.

---

# Command Line Interface

Executable:

```bash
cjm
```

Example:

```bash
cjm generate
```

Future commands may include:

```bash
cjm doctor
cjm version
cjm check
```

---

# C++ Namespace

```cpp
namespace cjm
```

---

# CMake Integration

```cmake
cjm_generate(...)
```

Example:

```cmake
find_package(CJM REQUIRED)

cjm_generate(
    TARGET app
    HEADERS
        user.hpp
)
```

---

# Generated Files

Generated files follow the naming convention:

```
*.cjm.hpp
```

Examples:

```
user.cjm.hpp
order.cjm.hpp
message.cjm.hpp
```

This makes generated code immediately recognizable while remaining valid C++.

---

# Brand Identity

CJM represents:

- Modern C++
- Build-time code generation
- Developer tooling
- Simplicity
- Portability
- Production-quality engineering

The implementation technology is intentionally hidden from users.

Users interact with:

- Standard C++
- CMake
- Generated C++

Implementation details (parsers, ASTs, code generators, etc.) are not part of the public product identity.

---

# Product Philosophy

CJM follows one fundamental principle:

> **Standard C++ in. Standard C++ out.**

Everything else is an implementation detail.

---

# Long-Term Vision

CJM aims to become a production-quality developer tool that integrates naturally into existing C++ projects while maintaining an intuitive, Go-inspired developer experience.

CJM Works serves as the home for this project and future open-source engineering tools.
