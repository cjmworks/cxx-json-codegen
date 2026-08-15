# simdjson Generated Decode Vertical Slice

Status: design in progress under epic #196. The root/object decode boundary is
approved for child issue #197. Required owned string decoding is defined by #198.
Optional integer presence behavior is defined by #199.
Required nested generated model composition is defined by #200.

## Goal

Extend the experimental generated C++17 simdjson decoder through small,
independently verified slices while preserving the runtime semantic profile and
portable decode errors.

The first slice separates decoding raw JSON text from decoding one already-open
simdjson object. It adds no new supported Metadata IR type.

## Layer Ownership

The simdjson backend generator owns this boundary because it generates
simdjson-specific traversal and manages simdjson input, parser, document, and
value lifetimes.

Semantic Analysis continues to validate and normalize models into Metadata IR.
The backend consumes that validated IR and does not inspect parser syntax or
repeat semantic validation.

## Generated Decode Boundaries

Generated decode has two internal stages.

### Raw-Text Root Wrapper

The existing `from_json<T>` specialization:

1. clears the output error;
2. creates padded input, parser, and document storage;
3. extracts the root object;
4. creates a temporary `T`;
5. delegates model decoding to `detail::decode_object`;
6. checks for trailing content after successful object decoding;
7. returns the completed temporary object.

The wrapper owns every object whose lifetime must cover On-Demand traversal.
The wrapper does not emit model field dispatch or required-field checks.

### Model Object Decoder

Each generated model receives an internal overload with this conceptual shape:

```cpp
bool decode_object(
    ::simdjson::ondemand::object& object,
    Model& value,
    DecodeError& error);
```

The overload:

1. initializes required-field presence state;
2. iterates the supplied object once;
3. dispatches recognized fields to generated value decoders;
4. ignores unknown fields;
5. verifies required-field presence;
6. returns `true` only for a complete model.

The object and error are borrowed from the caller. The decoder does not reset
the error and does not retain any simdjson value after returning.

The decoder may partially modify `value` before returning `false`. Every
generated caller must therefore pass storage owned by the current root decode
operation and discard the root temporary after any failure.

This internal basic mutation contract preserves the public new-object guarantee:

```text
success: return one complete model
failure: return an error and no model
```

Using `Model&` also lets ordinary C++ overload resolution select the generated
decoder. The generated path does not require template specialization, a type
tag, reflection, or a higher C++ language standard for object composition.

## Error And Path Contract

The root wrapper clears `DecodeError` once. Object decoders only report the
failure they observe.

An object decoder reports paths relative to the object it receives. Root-field
paths therefore remain unchanged in #197. A later nested-model slice will make
the parent prepend its own field segment to a child-relative path.

Inputs containing more than one independent error do not have a portable error
priority. In particular, after this boundary is introduced, a missing required
field may be reported before trailing content when both defects exist. Each
single-error scenario retains its existing portable result.

## #197 Invariants

- generated code remains deterministic, readable C++17;
- padded input, parser, and document lifetime stay in `from_json<T>`;
- `detail::decode_object` receives an already-open object by reference;
- each model object has one forward field iteration;
- unknown fields remain ignored;
- existing bool and integer conversions, errors, and root-field paths remain
  unchanged;
- trailing content remains a raw-text root-wrapper responsibility;
- no new Metadata IR kind becomes supported.

## #197 Verification

The generator golden tests must show that field traversal moved into
`detail::decode_object` and that `from_json<T>` delegates through a temporary
model.

The existing generated bool runtime test continues to prove success, missing
required, type mismatch, and trailing-content behavior. The existing generated
integer runtime test continues to prove arbitrary field order and narrow-integer
overflow behavior.

The simdjson-enabled CTest subset is the integration boundary for this child
issue.

## #198 Required Owned String Contract

Required `std::string` fields are decoded inside the generated object decoder.
The decoder reads a simdjson string result into a local `std::string_view` and
immediately copies that view into the model-owned `std::string` field.

This establishes the ownership boundary:

```text
simdjson view: valid only during current On-Demand traversal
model string: owned by the returned C++ object
```

The generated assignment uses the existing model field storage:

```cpp
value.name.assign(decoded_name.begin(), decoded_name.end());
```

A present non-string value reports `DecodeErrorCode::expected_string` with the
field path and simdjson runtime error. A missing required string uses the same
`missing_required_field` path contract as existing required scalar fields.

Borrowed string fields, `std::string_view` model fields, optional strings, and
container strings remain outside #198.

The generated string compile test proves escaped-string success, missing
required diagnostics, type mismatch diagnostics, and that decoded text can be
copied out after the public decode call returns.

## #199 Optional Integer Contract

Optional `std::optional<std::int64_t>` fields are decoded inside the generated
object decoder. The decoder initializes the model field to `std::nullopt` when
the JSON field is seen, keeps it disengaged for JSON `null`, and assigns the
decoded integer when the JSON value is a signed integer.

Missing optional fields are successful and leave the model field disengaged.
Optional fields do not create required-field presence flags and do not
participate in missing-required-field checks.

A present non-integer non-null value reports `DecodeErrorCode::expected_integer`
with the field path and simdjson runtime error.

Only `std::optional<std::int64_t>` is supported by #199. Optional strings,
optional unsigned integers, optional narrow integers, optional nested models,
containers, and encode remain outside this slice.

The generated optional integer compile test proves missing-field success,
explicit-null success, present signed-integer success, and present wrong-type
diagnostics through the public `from_json` entry point.

## #200 Required Nested Model Contract

Required user-defined fields are decoded by opening the child JSON value as a
simdjson object and passing that already-open object to the generated child
model object decoder.

The root wrapper remains the only generated code that owns padded input, parser,
and document storage. Nested decoding does not create a second parser and does
not restart traversal through the root document.

Each object decoder keeps its own required-field presence state and consumes its
own object with one forward field iteration. Unknown fields remain ignored at
both root and child levels.

A present non-object nested value reports `DecodeErrorCode::expected_object`
with the parent field path. A missing required nested field reports the existing
`missing_required_field` code with the parent field path.

Child object decoders report paths relative to the child object they receive. If
a child decoder fails, the parent prepends its own field segment so the public
root error path identifies the full location, such as `address.city`.

Only required, non-recursive generated model fields are supported by #200.
Optional nested models, nested containers, arrays, recursive models, and encode
remain outside this slice.

The generated nested compile test proves nested success with root and child
fields in non-declaration order, missing required nested field diagnostics,
non-object nested value diagnostics, child missing-field path composition, and
unknown child-field ignoring through the public `from_json` entry point.

## Deferred Epic Work

Later child issues define and implement:

- combined integration, golden, coexistence, and documentation verification.

Floating point, enums, containers, encode, borrowed strings, optional nested
models, recursive models, public backend selection, and performance claims
remain outside epic #196.
