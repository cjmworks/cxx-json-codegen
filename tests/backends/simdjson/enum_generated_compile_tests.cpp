#include <cassert>

enum class Status {
    Active,
    Disabled,
};

struct EnumValues {
    Status status;
};

#include "tests/golden/simdjson_enum.expected.cjm.hpp"

int main() {
    cjm::simdjson::DecodeError success_error;
    const auto success_result = cjm::simdjson::from_json<EnumValues>(
        R"({"status":"Active"})", success_error);

    assert(success_result.has_value());
    assert(success_result->status == Status::Active);
    assert(success_error.code == cjm::simdjson::DecodeErrorCode::none);
    assert(success_error.path.empty());
    assert(success_error.runtime_error == simdjson::SUCCESS);

    cjm::simdjson::DecodeError type_mismatch_error;
    const auto type_mismatch_result = cjm::simdjson::from_json<EnumValues>(
        R"({"status":123})", type_mismatch_error);

    assert(!type_mismatch_result.has_value());
    assert(type_mismatch_error.code ==
           cjm::simdjson::DecodeErrorCode::expected_string);
    assert(type_mismatch_error.path.size() == 1);
    assert(type_mismatch_error.path[0].kind ==
           cjm::simdjson::DecodePathSegmentKind::field);
    assert(type_mismatch_error.path[0].field_name == "status");
    assert(type_mismatch_error.runtime_error == simdjson::INCORRECT_TYPE);

    cjm::simdjson::DecodeError invalid_enum_error;
    const auto invalid_enum_result = cjm::simdjson::from_json<EnumValues>(
        R"({"status":"Paused"})", invalid_enum_error);

    assert(!invalid_enum_result.has_value());
    assert(invalid_enum_error.code ==
           cjm::simdjson::DecodeErrorCode::invalid_enum_string);
    assert(invalid_enum_error.path.size() == 1);
    assert(invalid_enum_error.path[0].kind ==
           cjm::simdjson::DecodePathSegmentKind::field);
    assert(invalid_enum_error.path[0].field_name == "status");
    assert(invalid_enum_error.runtime_error == simdjson::SUCCESS);

    return 0;
}
