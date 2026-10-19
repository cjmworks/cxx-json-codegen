#include <catch2/catch_test_macros.hpp>

#include "root_fault.cjm.hpp"

namespace cjm::simdjson::detail {

bool encode_object(::simdjson::builder::string_builder& builder,
                   const ::app::RootEncodeFault& value, EncodeError& error) {
    if (value.throw_length_error) {
        error.code = EncodeErrorCode::invalid_utf8_string;
        error.path = {{EncodePathSegmentKind::field, "name", 0}};
        error.runtime_error = ::simdjson::UTF8_ERROR;
        throw std::length_error("injected length failure");
    }

    builder.start_object();
    builder.end_object();
    return true;
}

} // namespace cjm::simdjson::detail
