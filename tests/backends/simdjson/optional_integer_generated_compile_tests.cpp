#include <cassert>
#include <cstdint>
#include <optional>
#include <string>

enum class Status {
    Active,
    Disabled,
};

struct OptionalScalarValues {
    std::optional<bool> maybe_enabled;
    std::optional<std::int64_t> maybe_count;
    std::optional<std::uint64_t> maybe_limit;
    std::optional<std::string> maybe_name;
    std::optional<Status> maybe_status;
};

#include "tests/golden/simdjson_optional_integer.expected.cjm.hpp"

int main() {
    cjm::simdjson::DecodeError missing_error;
    const auto missing_result =
        cjm::simdjson::from_json<OptionalScalarValues>("{}", missing_error);

    assert(missing_result.has_value());
    assert(!missing_result->maybe_enabled.has_value());
    assert(!missing_result->maybe_count.has_value());
    assert(!missing_result->maybe_limit.has_value());
    assert(!missing_result->maybe_name.has_value());
    assert(!missing_result->maybe_status.has_value());
    assert(missing_error.code == cjm::simdjson::DecodeErrorCode::none);
    assert(missing_error.path.empty());
    assert(missing_error.runtime_error == simdjson::SUCCESS);

    cjm::simdjson::DecodeError null_error;
    const auto null_result = cjm::simdjson::from_json<OptionalScalarValues>(
        R"({"maybe_enabled":null,"maybe_count":null,"maybe_limit":null,"maybe_name":null,"maybe_status":null})",
        null_error);

    assert(null_result.has_value());
    assert(!null_result->maybe_enabled.has_value());
    assert(!null_result->maybe_count.has_value());
    assert(!null_result->maybe_limit.has_value());
    assert(!null_result->maybe_name.has_value());
    assert(!null_result->maybe_status.has_value());
    assert(null_error.code == cjm::simdjson::DecodeErrorCode::none);
    assert(null_error.path.empty());
    assert(null_error.runtime_error == simdjson::SUCCESS);

    cjm::simdjson::DecodeError value_error;
    const auto value_result = cjm::simdjson::from_json<OptionalScalarValues>(
        R"({"maybe_count":-42})", value_error);

    assert(value_result.has_value());
    assert(value_result->maybe_count.has_value());
    assert(*value_result->maybe_count == -42);

    const auto scalar_result = cjm::simdjson::from_json<OptionalScalarValues>(
        R"({"maybe_enabled":true,"maybe_count":-42,"maybe_limit":42,"maybe_name":"Ada","maybe_status":"Active"})",
        value_error);
    assert(scalar_result.has_value());
    assert(value_error.code == cjm::simdjson::DecodeErrorCode::none);
    assert(value_error.path.empty());
    assert(value_error.runtime_error == simdjson::SUCCESS);

    cjm::simdjson::DecodeError type_mismatch_error;
    const auto type_mismatch_result =
        cjm::simdjson::from_json<OptionalScalarValues>(
            R"({"maybe_count":"nope"})", type_mismatch_error);

    assert(!type_mismatch_result.has_value());
    assert(scalar_result->maybe_enabled.has_value());
    assert(*scalar_result->maybe_enabled);
    assert(scalar_result->maybe_count.has_value());
    assert(*scalar_result->maybe_count == -42);
    assert(scalar_result->maybe_limit.has_value());
    assert(*scalar_result->maybe_limit == 42);
    assert(scalar_result->maybe_name.has_value());
    assert(*scalar_result->maybe_name == "Ada");
    assert(scalar_result->maybe_status.has_value());
    assert(*scalar_result->maybe_status == Status::Active);

    assert(type_mismatch_error.code ==
           cjm::simdjson::DecodeErrorCode::expected_integer);
    assert(type_mismatch_error.path.size() == 1);
    assert(type_mismatch_error.path[0].kind ==
           cjm::simdjson::DecodePathSegmentKind::field);
    assert(type_mismatch_error.path[0].field_name == "maybe_count");
    assert(type_mismatch_error.runtime_error == simdjson::INCORRECT_TYPE);

    cjm::simdjson::DecodeError invalid_enum_error;
    const auto invalid_enum_result =
        cjm::simdjson::from_json<OptionalScalarValues>(
            R"({"maybe_status":"Paused"})", invalid_enum_error);

    assert(!invalid_enum_result.has_value());
    assert(invalid_enum_error.code ==
           cjm::simdjson::DecodeErrorCode::invalid_enum_string);
    assert(invalid_enum_error.path.size() == 1);
    assert(invalid_enum_error.path[0].kind ==
           cjm::simdjson::DecodePathSegmentKind::field);
    assert(invalid_enum_error.path[0].field_name == "maybe_status");
    assert(invalid_enum_error.runtime_error == simdjson::SUCCESS);

    return 0;
}
