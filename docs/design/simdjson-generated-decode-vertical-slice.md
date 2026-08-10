# simdjson Generated Decode Vertical Slice

Status: design in progress under epic #196. The root/object decode boundary is
approved for child issue #197; additional type behavior remains pending.

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

## Deferred Epic Work

Later child issues define and implement:

- required owned strings;
- optional integer missing, null, and present behavior;
- one nested generated model and parent-path prepending;
- combined integration, golden, coexistence, and documentation verification.

Floating point, enums, containers, encode, borrowed strings, optional nested
models, recursive models, public backend selection, and performance claims
remain outside epic #196.
