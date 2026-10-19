#pragma once

#include <simdjson.h>

namespace app {

// Control the test-only object encoder.
struct RootEncodeFault {
    bool throw_length_error = false;
};

} // namespace app

namespace cjm::simdjson {
struct EncodeError;

namespace detail {

// Encode an empty object or inject a length exception for root-contract tests.
bool encode_object(::simdjson::builder::string_builder& builder,
                   const ::app::RootEncodeFault& value,
                   EncodeError& error);
} // namespace detail
} // namespace cjm::simdjson
