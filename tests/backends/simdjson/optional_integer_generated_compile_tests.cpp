#include <cassert>
#include <cstdint>
#include <optional>

struct OptionalIntegerValues {
    std::optional<std::int64_t> maybe_count;
};

#include "tests/golden/simdjson_optional_integer.expected.cjm.hpp"

int main() {
    cjm::simdjson::DecodeError missing_error;
    const auto missing_result =
        cjm::simdjson::from_json<OptionalIntegerValues>("{}", missing_error);

    assert(missing_result.has_value());
    assert(!missing_result->maybe_count.has_value());
    assert(missing_error.code == cjm::simdjson::DecodeErrorCode::none);
    assert(missing_error.path.empty());
    assert(missing_error.runtime_error == simdjson::SUCCESS);

    cjm::simdjson::DecodeError null_error;
    const auto null_result = cjm::simdjson::from_json<OptionalIntegerValues>(
        R"({"maybe_count":null})", null_error);

    assert(null_result.has_value());
    assert(!null_result->maybe_count.has_value());
    assert(null_error.code == cjm::simdjson::DecodeErrorCode::none);
    assert(null_error.path.empty());
    assert(null_error.runtime_error == simdjson::SUCCESS);

    cjm::simdjson::DecodeError value_error;
    const auto value_result = cjm::simdjson::from_json<OptionalIntegerValues>(
        R"({"maybe_count":-42})", value_error);

    assert(value_result.has_value());
    assert(value_result->maybe_count.has_value());
    assert(*value_result->maybe_count == -42);
    assert(value_error.code == cjm::simdjson::DecodeErrorCode::none);
    assert(value_error.path.empty());
    assert(value_error.runtime_error == simdjson::SUCCESS);

    cjm::simdjson::DecodeError type_mismatch_error;
    const auto type_mismatch_result =
        cjm::simdjson::from_json<OptionalIntegerValues>(
            R"({"maybe_count":"nope"})", type_mismatch_error);

    assert(!type_mismatch_result.has_value());
    assert(type_mismatch_error.code ==
           cjm::simdjson::DecodeErrorCode::expected_integer);
    assert(type_mismatch_error.path.size() == 1);
    assert(type_mismatch_error.path[0].kind ==
           cjm::simdjson::DecodePathSegmentKind::field);
    assert(type_mismatch_error.path[0].field_name == "maybe_count");
    assert(type_mismatch_error.runtime_error == simdjson::INCORRECT_TYPE);

    return 0;
}
