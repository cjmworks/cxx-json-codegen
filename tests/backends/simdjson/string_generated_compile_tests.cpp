#include <cassert>
#include <string>

struct StringValues {
    std::string name;
};

#include "tests/golden/simdjson_string.expected.cjm.hpp"

int main() {
    cjm::simdjson::DecodeError error;

    const auto result = cjm::simdjson::from_json<StringValues>(
        R"({"name": "Ada\nLovelace"})", error);

    assert(result.has_value());
    assert(result->name == std::string("Ada\nLovelace"));
    assert(error.code == cjm::simdjson::DecodeErrorCode::none);
    assert(error.path.empty());
    assert(error.runtime_error == simdjson::SUCCESS);

    cjm::simdjson::DecodeError missing_error;
    const auto missing_result =
        cjm::simdjson::from_json<StringValues>("{}", missing_error);

    assert(!missing_result.has_value());
    assert(missing_error.code ==
           cjm::simdjson::DecodeErrorCode::missing_required_field);
    assert(missing_error.path.size() == 1);
    assert(missing_error.path[0].kind ==
           cjm::simdjson::DecodePathSegmentKind::field);
    assert(missing_error.path[0].field_name == "name");
    assert(missing_error.runtime_error == simdjson::SUCCESS);

    cjm::simdjson::DecodeError type_mismatch_error;

    const auto type_mismatch_result = cjm::simdjson::from_json<StringValues>(
        R"({"name":123})", type_mismatch_error);

    assert(!type_mismatch_result.has_value());
    assert(type_mismatch_error.code ==
           cjm::simdjson::DecodeErrorCode::expected_string);
    assert(type_mismatch_error.path.size() == 1);
    assert(type_mismatch_error.path[0].kind ==
           cjm::simdjson::DecodePathSegmentKind::field);
    assert(type_mismatch_error.path[0].field_name == "name");
    assert(type_mismatch_error.runtime_error == simdjson::INCORRECT_TYPE);

    std::string owned_name;
    {
        cjm::simdjson::DecodeError ownership_error;

        const auto ownership_result = cjm::simdjson::from_json<StringValues>(
            R"({"name":"owned after decode"})", ownership_error);
        assert(ownership_result.has_value());
        assert(ownership_error.code == cjm::simdjson::DecodeErrorCode::none);
        assert(ownership_error.path.empty());
        assert(ownership_error.runtime_error == simdjson::SUCCESS);
        owned_name = ownership_result->name;
    }
    assert(owned_name == "owned after decode");
    return 0;
}
