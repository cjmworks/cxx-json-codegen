# Runtime JSON Semantic Profile

This document defines the v0.6 foundation semantic profile for CJM runtime JSON
backends.

It describes what CJM means when decoding and encoding supported Metadata IR
types. It does not define a universal runtime interface, a public C++ ABI, or a
specific backend implementation.

---

# Goal

Runtime backends may use very different implementation models:

```text
nlohmann/json
    DOM binding

simdjson On-Demand
    forward-only generated codec

Glaze
    direct typed adapter

yyjson
    compact document / DOM binding
```

CJM must still own the user-visible semantics:

```text
same supported model
same JSON input
same success meaning
same failure category
same field participation rules
same effective JSON names
```

Backend libraries may differ in API shape and capability, but they must not
silently redefine CJM model semantics.

---

# Ownership Boundary

CJM owns:

- field participation
- effective JSON field names
- ignored-field behavior
- required, optional, and future defaulted field policy
- nullability policy
- enum string policy
- numeric range policy
- fixed-array extent policy
- partial-output guarantee classification
- backend capability classification

Runtime backends own:

- JSON syntax parsing
- JSON formatting
- buffer ownership
- parser/document lifetime constraints
- runtime-specific performance strategy
- runtime-specific low-level error detail

The runtime backend consumes normalized Metadata IR. It must not inspect parser
syntax, comments, or Tree-sitter nodes.

---

# Current Type Surface

The profile applies to the existing Metadata IR type model:

```text
Bool
SignedInteger
UnsignedInteger
FloatingPoint
String
Enum
Array
Vector
Map
Optional
UserDefined
```

and recursive `arguments`.

This profile does not introduce a second runtime type algebra.

---

# Core Decode Profile

The core profile is the minimum behavior expected from a backend that claims
support for a type combination.

If a backend cannot implement a core behavior for a type combination, that
combination must be rejected at generation time or marked unsupported in the
capability matrix.

## Field Presence

Ordinary non-optional field:

```text
presence: required
missing: decode failure
```

`std::optional<T>` field:

```text
presence: optional
missing: std::nullopt
```

Future default metadata:

```text
presence: defaulted
missing: use default value
status: not implemented in v0.6 foundation
```

Explicit required/optional metadata:

```text
status: future feature
```

Presence and nullability are separate concepts. The current policy maps
`std::optional<T>` to both missing-allowed and null-allowed, but CJM must not
treat that as a permanent type-system rule.

## Nullability

Ordinary non-optional field:

```text
nullability: null forbidden
null input: decode failure
```

`std::optional<T>` field:

```text
nullability: null allowed
null input: std::nullopt
```

Containers:

```text
null container value: decode failure unless the field itself is optional
null container element: decode failure unless the element type explicitly allows null
```

Current CJM has no independent nullable metadata for non-optional fields.

## Unknown Fields

Initial core policy:

```text
unknown object fields are ignored
```

Strict unknown-field rejection is an optional capability, not a v0.6 core
requirement.

## Duplicate Object Keys

Initial core policy:

```text
duplicate key detection is backend-defined unless the backend declares strict
duplicate-key detection
```

Rationale:

- DOM backends may lose duplicate-key information during parsing.
- Generated On-Demand or event-based backends may be able to detect duplicates.
- CJM should not claim strict duplicate detection for a backend path that cannot
  observe duplicates.

Strict duplicate-key rejection is an optional strict capability.

## Trailing Content

A backend that decodes from raw JSON text should reject trailing non-whitespace
content after the root value.

A backend that receives an already parsed runtime value may not own trailing
content detection. That difference must be documented in the capability matrix.

---

# Value Semantics

## Booleans

JSON value must be a boolean.

Type mismatch is a decode failure.

## Signed Integers

JSON value must be an integer representable by the target C++ type.

Failure cases:

- non-integer JSON value
- value below target minimum
- value above target maximum

## Unsigned Integers

JSON value must be an integer representable by the target C++ type.

Failure cases:

- negative JSON integer
- non-integer JSON value
- value above target maximum

## Floating Point

JSON value must be a number representable by the target C++ type under the
selected backend policy.

Malformed numbers are decode failures.

Non-finite values are not part of standard JSON and are not accepted by the core
profile.

### Experimental simdjson decode policy

The current direct-field slice supports `float` and `double`. `long double`
and floating-point values wrapped in optional, vector, array, or map fields
remain generation-time unsupported capabilities. Direct-field encoding is
described below.

Generated code first reads a temporary `double` using `get_double()`. A failed
read reports `expected_number`, preserves the simdjson error code, and records
the effective JSON field path. In particular, `NUMBER_ERROR` is not relabeled
as overflow: it can also represent malformed number text.

Before assigning the temporary to the target field, generated code checks
against plus/minus `numeric_limits<target_type>::max()`. A value outside that
interval reports `floating_point_overflow` with runtime `SUCCESS`, because the
underlying read succeeded. No partially decoded root object is returned.

Ordinary binary floating-point rounding is accepted; exact decimal
representability is not required. Under the supported default floating-point
environment, sufficiently small magnitudes may round to zero, preserving the
sign. Tests separately cover JSON-to-double underflow and double-to-float
underflow, as well as representable subnormal values. These tests exercise the
usual IEEE binary32/binary64 environment; they do not establish portability
under fast-math, flush-to-zero modes, or altered rounding modes.

Each root call resets the error before decoding; a successful call after a
failed call leaves no stale code, runtime error, or path.

### Experimental simdjson encode policy

Models whose participating fields are bool, supported integers, `float`, or
`double` now have generated encoders. This is a direct-field capability, not
support for floating-point optionals or containers. `long double` remains
unsupported; the generator must not silently narrow it to `double`.

Before writing each floating-point value, generated code checks
`std::isfinite`. NaN and positive/negative infinity report `non_finite_number`
with the effective JSON field name in the path and runtime `SUCCESS`: the
value was rejected by CJM before calling the writer. Failure returns no JSON
string, even if earlier fields were already written to the private builder.
Resource and writer failures retain the precedence defined in the
[encode strategy](simdjson-encode-strategy.md#error-contract).

Finite values use the official builder's formatting. Tests cover ordinary
values and round trips for positive/negative maxima, the smallest positive
subnormals, negative zero, and representative decimal values. Round-trip
assertions compare the original stored floating-point values and zero signs,
not a universal decimal spelling or exact mathematical decimal value. This
is sampled coverage of the current runtime/compiler environment, not proof
for every floating-point value or cross-backend equivalence.

Each encode call creates a new builder and resets the caller's error object.
The recovery test reuses that error object after a non-finite-value failure,
then verifies that a valid input produces complete output with no stale error
or path. Recovery means independence of successive calls, not resuming a
partially written document or repairing the invalid input.

## Strings

JSON value must be a string.

The runtime owns UTF-8 and escape validation according to its parser contract.
The capability matrix must record material differences between backend string
validation behavior.

## Enums

JSON value must be a string matching a known Metadata IR enum value.

Failure cases:

- JSON value is not a string
- string does not match a known enum mapping

Custom enum rename policies are not part of v0.6 foundation.

## User-Defined Objects

JSON value must be an object.

Decode recursively applies the same profile to the referenced generated type.

Unknown-field, duplicate-key, and partial-output policies apply independently
inside nested objects.

## Vectors

JSON value must be an array.

Each element must satisfy the element type profile.

A failed element decode fails the whole vector decode.

## Fixed Arrays

JSON value must be an array with exactly `array_extent` elements.

Failure cases:

- JSON value is not an array
- too few elements
- too many elements
- any element fails the element type profile

## Maps

JSON value must be an object.

Current supported map keys are strings only:

```text
std::map<std::string, T>
std::unordered_map<std::string, T>
```

Each JSON object property value must satisfy the mapped value type profile.

Non-string C++ map keys are unsupported by Semantic Analysis.

## Optionals

For `std::optional<T>`:

```text
missing field: std::nullopt
null field: std::nullopt
present non-null field: decode T
```

If decoding `T` fails, decoding the optional field fails.

---

# Encode Profile

Encoding should emit deterministic JSON for supported models.

Object field order:

```text
Metadata IR field order
```

Ignored fields:

```text
not emitted
```

Optional fields with `omit_empty`:

```text
disengaged optional is not emitted
engaged optional is emitted as its contained value
```

Optional fields without `omit_empty`:

```text
disengaged optional is emitted as null
engaged optional is emitted as its contained value
```

Enum fields:

```text
emit the Metadata IR enum string
```

Backend-specific formatting differences such as whitespace are not semantic
unless a backend claims byte-for-byte deterministic output.

---

# Decode Output Guarantee

The default public decode shape should prefer producing a new object:

```cpp
auto result = cjm::decode<User>(json_input);
```

This profile classifies that as:

```text
new-object guarantee
```

Meaning:

```text
success: returns a complete User
failure: returns no User
```

An in-place API may be added later:

```cpp
cjm::decode_into(json_input, existing_user);
```

but it must explicitly choose one guarantee:

```text
strong
    failure leaves existing_user unchanged

basic
    failure leaves existing_user valid but possibly partially updated
```

The completed simdjson spike follows this new-object guarantee and does not
define an accidental in-place partial-update contract.

---

# Capability Classes

Core capabilities:

- supported type combinations either decode according to this profile or fail at
  generation time
- missing required fields fail
- null for non-nullable fields fails
- integer overflow fails
- invalid enum strings fail
- fixed array extent mismatch fails
- unsupported map key types fail during Semantic Analysis

Optional strict capabilities:

- strict duplicate-key detection
- strict unknown-field rejection
- transactional in-place decode
- byte-for-byte deterministic encoding
- backend-owned trailing-content detection when decoding from raw text

Backends must not silently claim a strict behavior that their integration path
cannot observe or enforce.

---

# Non-Goals

This profile does not add:

- a universal `JsonRuntime` facade
- runtime dynamic backend selection
- a second type algebra
- custom converters
- custom enum string mappings
- default value metadata
- independent nullable metadata
- arbitrary dynamic JSON values
- new STL container mappings
- native JSON parser or formatter implementation

---

# Relationship To Later Work

This profile is the input to:

- the [decode error and path model](runtime-decode-error-model.md)
- the [conformance fixture skeleton](runtime-conformance-fixtures.md)
- [static backend selection design](static-backend-selection.md)
- simdjson On-Demand decode spike

Future milestones may expand the profile, but they should do so by updating CJM
semantics first and backend implementations second.
