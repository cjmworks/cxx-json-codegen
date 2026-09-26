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

The current slice supports direct and optional `float` and `double` fields.
`long double` (including optional wrapping) and floating-point values in vector,
array, or map fields remain generation-time unsupported capabilities.
Encoding is described below.

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

Models whose participating fields are bool, supported integers, `float`,
`double`, owned `std::string`, or enums with Metadata IR definitions, including
optional fields wrapping these types, now have generated encoders. Required
nested generated objects are also supported when their participating fields
are encodable. Container support and optional-object acceptance remain tracked
separately. Directly nested optionals are excluded by the decision below.
`long double` remains
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

Owned string values are checked with the official `simdjson::validate_utf8`
before their string value is written, then emitted with
`escape_and_append_with_quotes`. Explicit lengths preserve embedded NUL;
the JSON output represents it as `\u0000`. Invalid or truncated UTF-8 reports
`invalid_utf8_string`, runtime `UTF8_ERROR`, and the effective JSON field name.
Failure returns no partial JSON. Runtime tests cover empty/ASCII/Chinese text,
newline, quote, backslash, embedded NUL, invalid/truncated UTF-8, and invalid
bytes after NUL. Generator tests separately check renamed-field error paths.
This does not claim dynamic map-key validation or complete emitted-key handling;
those remain part of the pending encoder scope.

Enum fields use the matching Metadata IR enum definition, selected by qualified
type name at generation time. Generated comparisons map known values to their
enumerator-name strings and write them with the official escaping function.
An unmapped value reports `invalid_enum_value`, the effective JSON field name,
and runtime `SUCCESS`: this is a CJM mapping error, not a writer error. No partial
JSON is returned. Runtime tests cover both named values of the test enum,
an unmapped value, and success after failure. Generator tests cover renamed
error paths, qualified-name lookup, the public generation entry point, and the
empty-enum helper branch. The empty-enum test checks generated text only;
it does not establish end-to-end empty-enum support. Optional enums are also
covered by generated runtime tests; enum containers and custom enum rename
policies are outside this completed slice.

Each encode call creates a new builder and resets the caller's error object.
Recovery tests reuse that error object after a non-finite-value, invalid-UTF-8,
or unmapped-enum failure, then verify that a valid input produces complete output with no stale error
or path. Recovery means independence of successive calls, not resuming a
partially written document or repairing the invalid input.

### Experimental simdjson nested object status

As of 2026-09-23, required nested objects append directly to the root's official
builder through generated `encode_object` overloads. They do not return
intermediate JSON strings. Semantic Analysis supplies dependency-ordered models;
the generator records successfully emitted encoders and enables a parent only
when each participating field has a supported mapping. An unsupported child
prevents parent encoder emission without removing supported decoder output.
Complete unsupported-encode diagnostics remain tracked in #226.

Child data errors retain their code and runtime error. Each parent prepends
its effective JSON field name exactly once; tests verify `home → city` and
`owner → home → city`. The root returns no partial output, and a subsequent
successful call clears the previous error and path.

Generated C++17 runtime tests cover two- and three-level round trips, sibling
field separators, nested invalid UTF-8, and recovery. An all-omitted child
still emits an object, for example `{"home":{}}`. This empty-child test uses
a model with an omitted optional scalar, not a fieldless C++ struct: the
current frontend omits fieldless models from its generated model set.

Required-object support does not imply completed optional-object acceptance
(#224), container support (#213), recursive-model support, or complete
resource-failure conformance (#227/#214). No performance claim is made.

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

### Optional field compatibility decision — 2026-09-25

CJM's optional-field contract follows ordinary pointer-valued struct fields
in Go's traditional `encoding/json` (v1 semantics), not Go's pointer storage
or allocation model. This is a bounded compatibility target, not a claim of
complete Go JSON compatibility or verified cross-backend parity.

| State / input | CJM behavior |
| --- | --- |
| Disengaged optional, with `omitempty` | Omit the member |
| Disengaged optional, without `omitempty` | Emit `null` |
| Engaged optional | Encode the contained value, retaining zero, false, empty strings and empty objects |
| Missing field when decoding a fresh model | `std::nullopt` |
| Explicit `null` | `std::nullopt` |
| Present non-null field | Decode `T`; propagate failures |

Go leaves a missing field unchanged when decoding into an existing object.
CJM's simdjson root API returns a fresh model, not an in-place merge/PATCH
operation. Do not infer equivalent update behavior for existing destinations.
Ordinary non-optional Go fields, nil slices/maps, and Go JSON v2 omission
rules are not included in this compatibility target.

References: [Go Marshal](https://pkg.go.dev/encoding/json#Marshal),
[Go Unmarshal](https://pkg.go.dev/encoding/json#Unmarshal). In particular,
a non-nil pointer to zero is not omitted by v1 `omitempty`; similarly, CJM
checks optional engagement rather than recursively testing the contained value.

Direct nesting such as `optional<optional<T>>` (including deeper chains) is
outside the current approved mapping scope and must receive generation-time
rejection, not silently collapse distinct C++ states. This restriction applies
wherever that type shape occurs in a participating field. It does not prohibit
an optional object containing optional members, or otherwise supported
`optional<vector<optional<T>>>` compositions. Ignored fields remain ignored.

Three-state Missing/Null/Value or PATCH behavior requires a separate explicit
design if a real use case arises. It is not an implementation task in #224.
Existing backend behavior must be audited against this contract before claiming
parity: #214 owns shared input/output and Go-reference cases; #226 owns remaining
simdjson generation-time diagnostics. Do not silently change other backends
as part of this scope decision.

### Experimental simdjson optional scalar status

As of 2026-09-20, generated encoding supports optional bool, supported signed
and unsigned integers, float/double, owned strings, and mapped enums. A
disengaged field is omitted with `omitempty` and emitted as `null` otherwise.
Engaged zero, false, and empty strings are retained. Commas follow actually
emitted fields; an all-omitted model produces `{}`. Ignored fields are not
validated or emitted.

Contained-value failures retain the effective JSON field path without an
extra optional segment. Runtime tests cover invalid UTF-8, unmapped enums,
non-finite floats, and success after failure. Optional floating round trips
cover signed zero, finite extrema, minimum normal/subnormal values, and
representative decimal rounding. Decode tests distinguish wrong types,
target-float overflow after a successful double read, and runtime number
errors; recovery clears the previous error and path.

This completes the current optional scalar slice, not all `optional<T>`
combinations. Optional-object acceptance and container combinations still
need their respective capability and integration coverage. Directly nested
optionals are excluded by the 2026-09-25 decision, rather than pending support.

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
