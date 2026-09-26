# simdjson Encode Strategy

Status: official writer and API/error contract selected under #211.
Direct-field bool, integer, float/double, owned-string, and enum encoding is
implemented with generated-code regression coverage, including optional fields
wrapping those scalar types, and required nested generated objects using the
same root builder. The broader #212/#213 encoder scope and
#214 conformance work remain incomplete.

Parent contract: [simdjson Experimental Backend MVP](simdjson-experimental-backend-mvp.md),
epic #203. Evidence recorded on 2026-09-08 against simdjson v4.6.4.

Implementation status updated on 2026-09-23. See the
[encode runtime policy](runtime-json-semantic-profile.md#experimental-simdjson-encode-policy)
for current scope, optional omission/null behavior, finite-value and UTF-8 checks, enum mapping, escaping coverage,
and the meaning of success-after-failure recovery. The complete contract below includes planned
capabilities; it is not a claim that every listed shape is implemented.

## Decision And Rationale

Use the documented `simdjson::builder::string_builder`, included through
`<simdjson.h>`, for the experimental backend's generated encoder. Do not build
a separate CJM JSON writer, copy simdjson's escaping/number-formatting code, or
introduce a universal writer facade.

The official builder supplies the primitives needed for the approved model
surface subject to the explicit `long double` limitation below. Its manual
object/array example already follows the intended division:
the caller controls structure; the runtime writes JSON bytes. A custom writer
would duplicate escaping, formatting, allocation, and their maintenance without
a demonstrated requirement. This is a compatibility and maintenance decision,
not a measured performance claim.

This API is distinct from `simdjson::internal::string_builder` used by DOM
serialization. The earlier assumption that simdjson lacked a usable documented
writer was incorrect. The public builder is low-level, not a complete validating
serializer, and its documentation is not a guarantee of future API stability.

## Architectural Ownership

Semantic Analysis still validates the model and builds backend-independent
Metadata IR. The simdjson generator consumes that IR without parser access.
Runtime value checks belong in generated encode code, not Semantic Analysis.

| Responsibility | Owner |
| --- | --- |
| Field participation, effective JSON names, enum mappings | Validated Metadata IR, applied by generated code |
| Field order, omission, separators, container traversal, nested composition | CJM-generated encoder |
| String escaping, finite-number formatting, buffer capacity checks/growth | Official simdjson builder |
| Runtime input validation, error context, returning owned output | CJM-generated encoder using official runtime primitives |

Runtime flow (public signature below; leaf helper decomposition is reviewed
one implementation slice at a time):

```text
cjm::simdjson::to_json(value, error)
    create one local official builder
    record model_valid = detail::encode_object(builder, value, error)
        write participating fields in generated order
            scalar / enum / optional value
            array or vector elements
            map keys and values
            nested model using the same builder
    check builder result even when model_valid is false
        writer failure -> root output_failure, discard any data-error path
        otherwise model_valid is false -> retain the data error
        otherwise -> copy the view into an owning string
    catch standard allocation/length exceptions -> path-free resource error
```

Review concrete helper contracts and the orchestration skeleton before
implementation. Implement leaves bottom-up, with focused tests, before
container composition and root integration. Do not create generic runtime
abstractions merely to express this graph.

## Public Encode API

Follow the existing generated decoder's `std::optional<T>` plus error-reference
style. There is no existing `cjm::result` abstraction to extend. The selected
interface is:

```cpp
namespace cjm::simdjson {

using EncodePathSegmentKind = DecodePathSegmentKind;
using EncodePathSegment = DecodePathSegment;

enum class EncodeErrorCode {
    none,
    invalid_utf8_string,
    invalid_utf8_key,
    non_finite_number,
    invalid_enum_value,
    output_failure,
    allocation_failure,
    size_limit_exceeded
};

struct EncodeError {
    EncodeErrorCode code = EncodeErrorCode::none;
    std::vector<EncodePathSegment> path;
    ::simdjson::error_code runtime_error = ::simdjson::SUCCESS;
};

template <typename T>
std::optional<std::string> to_json(const T& value, EncodeError& error);

} // namespace cjm::simdjson
```

Generate inline specializations for supported root model types, mirroring
`from_json<T>`. Template argument deduction permits `to_json(user, error)`.
Root scalar/container convenience overloads are not added by this MVP.

The path aliases reuse the existing field/index storage without copying the
path model or renaming decoder types in an encoder feature. Encode has distinct
error codes because malformed JSON and missing input fields are decode concepts.
Emit encode declarations under a separate multiple-header guard; do not make an
older decode-only header suppress them through the existing decode guard.

The function resets `error` on entry. Success returns an owning string and
leaves `code == none`, an empty path, and `runtime_error == SUCCESS`. Failure
returns `std::nullopt` with a non-`none` error. Input models are never mutated.
No caller-owned output buffer or borrowed output overload is introduced.

This is an experimental backend API, not a stable cross-backend ABI. The
internal composition entry point is an overload of:

```cpp
bool encode_object(::simdjson::builder::string_builder& builder,
                   const Model& value, EncodeError& error);
```

Its `bool` reports model validation/traversal only: `true` means all participating
values were valid and their write operations were issued; it does not guarantee
that the builder retained those writes. Builder append operations return `void`
and can record a capacity failure for later inspection. `false` means a data
error was detected and its child-relative path is available, unless constructing
that diagnostic threw a standard resource exception.

After any normal helper return, the root must check `builder.view()` regardless
of the boolean result, then apply the error precedence below. Only a valid
builder plus `model_valid == true` permits copying output. The private builder
may contain an incomplete prefix on failure; none of it is returned. Checking
every primitive write is not required by this contract.

## Official API And C++17 Boundary

The selected primitive surface is:

- `start_object()` / `end_object()` and `start_array()` / `end_array()`;
- `append_comma()` / `append_colon()`;
- `append()` for explicitly supported arithmetic types, and `append_null()`;
- `escape_and_append_with_quotes(std::string_view)` for strings and keys;
- `view().get(...)` for explicit result checking;
- `validate_unicode()` for whole-buffer UTF-8 checking, or the public
  `simdjson::validate_utf8(...)` for input-local checks when needed.

All selected builder calls were compiled and exercised as C++17. Although the
v4.6.4 builder guide labels `view()` as C++20, its declaration is not guarded by
concept support and the C++17 probe succeeds. Preserve a compilation test for
this exact dependency boundary rather than raising the project's standard.

Do not depend on C++20 automatic container/custom-type serialization,
compile-time string-key templates, or C++26 reflection. Explicit traversal is
compatible with the official low-level API and keeps CJM's mapping rules visible.
Do not use internal helpers, architecture-specific namespaces, or patch the
dependency. Use the official `simdjson::simdjson` CMake target.

`append(char)` writes a raw byte, not a JSON number or quoted string. Select
numeric versus string operations from the approved Metadata IR contract; do not
blindly forward arbitrary C++ types to an overload. User strings and keys must
never pass through `append_raw()`. Pass explicit string lengths so embedded NUL
bytes are preserved and escaped.

## Ownership And Failure Boundaries

The builder owns its buffer. `view()` borrows it; destruction, reuse, or
reallocation can invalidate that view. The owning root result must be copied
before the builder goes out of scope. Nested encoders share the root builder
instead of constructing intermediate JSON strings. Do not expose a view into a
local builder, or share a mutable builder between concurrent encode calls.

The ordinary growable-buffer path checks capacity before writing. Allocation
failure invalidates the builder and `view()` reports `OUT_OF_CAPACITY`.
This is not an all-purpose encode error: it carries no model field/index path.
The final `std::string` allocation is a separate failure point and can throw.
The root translates standard allocation/length exceptions as specified below.
The public function is not `noexcept`; it does not mask unrelated exceptions
from user-provided container behavior or programming errors. A complete
`-fno-exceptions` encoder is not promised by this MVP: the primitive API probe
alone does not establish recoverable allocation failure for such a build.

`view()` success does not validate JSON syntax or UTF-8. For example, manually
writing `[,]` succeeds as a buffer operation, and it is valid UTF-8 but invalid
JSON. CJM must generate correct delimiters and verify that behavior in tests.

String escaping does not validate UTF-8. Validate every emitted string and key
with the official `simdjson::validate_utf8(...)` before escaping, while its
model path is known. Whole-buffer `validate_unicode()` is useful in tests but
is not a second mandatory runtime pass. Ignored fields are not inspected.
Do not infer a model path from a final buffer failure, and do not duplicate
Unicode validation internally.

### Error Contract

| Condition | `EncodeErrorCode` | Path / runtime detail |
| --- | --- | --- |
| Invalid UTF-8 string value | `invalid_utf8_string` | Full field/index path; runtime `UTF8_ERROR` |
| Invalid UTF-8 object key | `invalid_utf8_key` | Containing path plus the offending key; runtime `UTF8_ERROR` |
| NaN or positive/negative infinity | `non_finite_number` | Full value path; runtime `SUCCESS` because rejected before the builder |
| Enum value absent from the Metadata IR mapping | `invalid_enum_value` | Full value path; runtime `SUCCESS` |
| Builder reports failure | `output_failure` | Empty/root path; preserve returned runtime code, normally `OUT_OF_CAPACITY` |
| `std::bad_alloc`, including output-copy or error-path allocation | `allocation_failure` | Empty path; runtime `SUCCESS` |
| `std::length_error` from output or diagnostic storage | `size_limit_exceeded` | Empty path; runtime `SUCCESS` |

Runtime `SUCCESS` on a CJM failure means no underlying simdjson error was
reported; callers use the CJM code as authoritative. `output_failure` does not
pretend to distinguish allocation exhaustion from other builder capacity limits.

Paths use effective JSON names and zero-based indices. Nested helpers return
child-relative paths, and each caller prepends its own context exactly once.
Optional wrapping adds no path segment. For example, a failure inside a map of
vectors can report `field("groups"), field("admin"), index(2), field("name")`.
Stop traversal at the first detected data error; this does not claim temporal
ordering across data errors and delayed builder failures. Unordered-map traversal
does not promise which of several bad entries is detected first.

Select the final result using this precedence:

1. A caught `std::bad_alloc` or `std::length_error` becomes its corresponding
   path-free resource error, even if another failure had already been recorded.
2. After a normal helper return, inspect the builder even when the helper
   returned `false`. A builder failure becomes root `output_failure` and clears
   any existing data-error path.
3. If the builder is valid but the helper returned `false`, retain its data error
   and full path.
4. Only when both checks succeed, copy and return the owning string. Copy
   allocation/length exceptions are handled by rule 1.

For example, a failed writer allocation followed by invalid UTF-8 reports
`output_failure`, not the later string error. If constructing that string error's
path throws first, the caught resource exception wins instead. No error fallback
may allocate, and no failed outcome exposes partially written JSON.

An invalid key is preserved as owned raw bytes in the final key segment.
The `invalid_utf8_key` code distinguishes it from a value failure at the same
path. Display code must escape those bytes; the path is not promised to be a
valid UTF-8 display string.

Catch allocation/length failures at the root, clear any existing diagnostic
path, and set only fixed-size error fields before returning `std::nullopt`.
That fallback must not allocate or format a message. If collecting a data-error
path itself exhausts memory, the resource error takes precedence and the
original detailed path is lost by design. Do not promise recovery from process
termination, stack exhaustion, or invalid concurrent access to the input.

## Numeric And Mapping Compatibility

### Floating-Point Type Boundary

The selected encoder supports `float` and `double`, subject to the finite-value
rule below. For this MVP, an emitted `long double` field is an explicit
generation-time unsupported-capability error. Apply that check recursively to
optional, vector/array, map-value, and nested-model types; ignored fields are
not part of the emitted encode surface. Report the model, field, effective JSON
name, and unsupported C++ type without emitting a partial codec.

The distinction from upstream must be precise: v4.6.4's arithmetic `append`
template accepts `long double`, but the floating-point branch unconditionally
converts it to `double` before formatting. Its `to_json` convenience wrappers
also delegate to `append`; they do not provide a full-precision alternative.
It is therefore incorrect to say the official API rejects `long double`.
It accepts the type without preserving its additional precision or range on
platforms where it is wider than `double`.

CJM's stricter generation-time rejection is deliberate: do not silently narrow
user values, emulate an unsupported extended-precision formatter, or introduce
a custom writer. The restriction is uniform even on platforms where the two
floating-point types happen to have equal precision. Revisit it in a separate
capability decision if an appropriate official API becomes available.

This is an accepted encoder parity blocker, not a change to Semantic Analysis's
backend-independent type model or to the nlohmann backend. It neither implements
nor decides the separate `long double` decoder capability. Checking that the
original `long double` is finite would not prevent precision loss or a
conversion outside the representable `double` range.

### Floating-Point Values And Mapping Rules

Never pass NaN or infinity directly to `append()`. In v4.6.4 its floating-point
path calls `internal::to_chars`, whose precondition requires finite input.
The local probe returned success with unrelated large numeric text for NaN and
infinity; that is an observation outside the supported precondition, not an
output contract to preserve. Check finiteness before invoking the builder.

The nlohmann comparison emits `null` for NaN and infinity. The
[runtime semantic profile](runtime-json-semantic-profile.md#floating-point)
excludes non-finite numbers. This encoder therefore rejects all three cases
with `non_finite_number`, rather than silently turning a non-nullable numeric
field into JSON `null`. This is an explicit failure-policy difference from the
existing nlohmann backend, not full behavior parity for out-of-profile inputs.
It does not change nlohmann code or behavior.

Finite `float` values are formatted through `double` in this version. For
example, `0.1f` produced `0.10000000149011612`; a longer representation alone is
not evidence of a round-trip failure. Test numeric value preservation separately
from output spelling; do not promise byte-for-byte equality with nlohmann.

Preserve the [existing encode profile](runtime-json-semantic-profile.md#encode-profile),
including its already-defined optional field rules:

| Position / state | Encoded result |
| --- | --- |
| Ignored field | No output and no value validation |
| Disengaged optional field with `omitempty` | Omit the member |
| Disengaged optional field without `omitempty` | Emit the member with `null` |
| Engaged optional | Encode its contained value, including zero/empty values |
| Disengaged optional array/vector element | Emit `null`; retain index and length |
| Disengaged optional map value | Retain the key and emit `null` |
| Directly nested optional, such as `optional<optional<T>>` | Unsupported mapping; reject at generation time |

Only the field's own outer optional presence controls `omitempty`; it does not
propagate to container elements or map values. These rules apply to approved
type combinations and do not bypass generation-time capability checks.

The 2026-09-25 [optional-field compatibility decision](runtime-json-semantic-profile.md#optional-field-compatibility-decision--2026-09-25)
aligns supported optional fields with Go `encoding/json` v1 pointer-field
semantics for fresh-model decoding. It supersedes the earlier nested-optional
encode proposal. Three-state/PATCH support is not part of #224; supported
optional objects and container combinations remain separate commitments.

Generated model fields use Metadata IR order. Vectors/arrays preserve element
order, and ordered maps preserve their comparator's iteration order.
Unordered maps use their actual iteration order without added sorting: semantic
JSON member equality is promised, not byte-order stability across runs,
implementations, or mutation histories. Record that exception to deterministic
ordering explicitly in #213/#214; no canonical-JSON mode is introduced.

An object with no emitted fields is `{}`, an empty array is `[]`, and an empty
map is `{}`. The current nlohmann generator does not explicitly initialize a
model object before assignments, so an all-omitted model can remain `null`.
That implementation artifact is not copied into this encoder; #214 must record
the difference against the model-as-object contract. Other formatting differences
are compared semantically unless a golden test specifically asserts spelling.

Do not change the default nlohmann backend's semantics as part of this decision.

## Verification Evidence And Limits

Local temporary probes used Linux x86_64, GCC 15.2.0, `-std=c++17`, the pinned
v4.6.4 headers, and the existing simdjson static library. They were not added to
the repository and are not substitutes for committed regression tests.

| Probe | Observed result |
| --- | --- |
| C++17 manual object/array construction and `view().get()` | Compiled and ran |
| Same primitive probe with `-fno-exceptions -DSIMDJSON_EXCEPTIONS=0` | Compiled and ran against the existing library; not a full no-exceptions build audit |
| Every signed/unsigned 8-bit value; 16/32/64-bit signed extrema and unsigned maxima | Expected decimal text |
| Double +/-zero, representative fractions, minimum normal, maximum finite, minimum subnormal | Parse-back equality, including zero sign |
| `append(long double)` using `nextafter(1.0L, 2.0L)` on the local wider-precision platform | Compiled and succeeded, but emitted `1.0`, losing the extra precision |
| Chinese text, quotes, backslash, all bytes 0x00-0x1f | Escaped output parsed back to the original string |
| Initial capacity of one byte, growing past 4 KiB | Correct output; owned copy survived builder reuse |
| Malformed UTF-8 | `view()` succeeded; `validate_unicode()` returned false |
| Malformed structure `[,]` | `view()` succeeded; UTF-8 validation did not detect JSON syntax error |
| Injected initial/growth allocation failures | `OUT_OF_CAPACITY`; successful reuse after `clear()` and allocation recovery |
| Direct NaN/infinity append | Did not reliably reject unsupported input; caller guard required |
| nlohmann comparison | NaN/infinity became `null`; malformed UTF-8 threw on default `dump()` |
| Proposed API compiled beside an existing generated decoder | C++17 template deduction, distinct encode errors, and shared path aliases compiled and ran |
| Handwritten API probe with one vector of strings and one double | Owned success, string index path, non-finite field path, and error reset passed |
| Explicitly injected `bad_alloc` / `length_error` at contract boundaries | Root returned no output and a path-free resource error; not a process-wide allocator exhaustion test |
| Failed builder allocation plus invalid string in the handwritten API probe | Builder checked despite helper `false`; root `output_failure` replaced the data error and cleared its path |

These checks do not prove all floating-point values, every platform/compiler,
generated encoder correctness, or performance. Failure injection covered builder
array allocations, not every possible process allocation.

## Implementation Gates And Issue Allocation

- **#211:** review this writer/API/error contract with the maintainer before
  closing. The semantic choices above are concrete, not deferred alternatives.
- **#212:** implement reviewed scalar/string/enum/optional/nested encode contracts
  on the official builder, including owned output, helper/root success separation,
  resource-error precedence, and recursive generation-time rejection of
  `long double` in emitted values.
- **#213:** add approved containers/maps using the same builder; preserve null
  positions, key escaping, error paths, and the ordering guarantees above.
- **#214:** add independently runnable compatibility/conformance tests, generated
  C++17 compile/run and golden coverage, and dependency-isolation checks. Include
  multiple generated headers in both include orders to verify declaration guards.
  Test `long double` rejection in direct/composed fields and mixed builder/data
  failures, as well as allocation failure while collecting error paths.

The dependency helper currently accepts an already-provided
`simdjson::simdjson` target before its exact-version lookup. An injected target
must therefore also pass the selected API compatibility checks; its name alone
does not establish v4.6.4 compatibility. Track verification in #214 without
silently changing the dependency policy here.

On dependency upgrades, rerun the primitive and generated compatibility suite.
Keep simdjson out of default nlohmann builds. Do not promise compatibility with
untested versions or platforms. No encoder issue is complete merely because
the writer has been selected.

## Official References

- [v4.6.4 builder guide](https://github.com/simdjson/simdjson/blob/v4.6.4/doc/builder.md)
- [v4.6.4 builder declaration](https://github.com/simdjson/simdjson/blob/v4.6.4/include/simdjson/generic/builder/json_string_builder.h)
- [v4.6.4 builder implementation](https://github.com/simdjson/simdjson/blob/v4.6.4/include/simdjson/generic/builder/json_string_builder-inl.h)
- [v4.6.4 finite-number formatting precondition](https://github.com/simdjson/simdjson/blob/v4.6.4/src/to_chars.cpp#L908-L915)
- [nlohmann number handling](https://json.nlohmann.me/features/types/number_handling/)
