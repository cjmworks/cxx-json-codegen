#include <cassert>
#include <cstdint>
#include <string>

struct Address {
    std::string city;
};

struct NestedUser {
    std::int64_t id = 0;
    Address address;
};

#include "tests/golden/simdjson_nested.expected.cjm.hpp"

int main() {
    cjm::simdjson::DecodeError success_error;
    const auto success_result = cjm::simdjson::from_json<NestedUser>(
        R"({"address":{"ignored":true,"city":"Paris"},"id":7})",
        success_error);

    assert(success_result.has_value());
    assert(success_result->id == 7);
    assert(success_result->address.city == "Paris");
    assert(success_error.code == cjm::simdjson::DecodeErrorCode::none);
    assert(success_error.path.empty());
    assert(success_error.runtime_error == simdjson::SUCCESS);

    cjm::simdjson::DecodeError missing_nested_error;
    const auto missing_nested_result = cjm::simdjson::from_json<NestedUser>(
        R"({"id":7})", missing_nested_error);

    assert(!missing_nested_result.has_value());
    assert(missing_nested_error.code ==
           cjm::simdjson::DecodeErrorCode::missing_required_field);
    assert(missing_nested_error.path.size() == 1);
    assert(missing_nested_error.path[0].kind ==
           cjm::simdjson::DecodePathSegmentKind::field);
    assert(missing_nested_error.path[0].field_name == "address");
    assert(missing_nested_error.runtime_error == simdjson::SUCCESS);

    cjm::simdjson::DecodeError nested_type_error;
    const auto nested_type_result = cjm::simdjson::from_json<NestedUser>(
        R"({"id":7,"address":"not an object"})", nested_type_error);

    assert(!nested_type_result.has_value());
    assert(nested_type_error.code ==
           cjm::simdjson::DecodeErrorCode::expected_object);
    assert(nested_type_error.path.size() == 1);
    assert(nested_type_error.path[0].kind ==
           cjm::simdjson::DecodePathSegmentKind::field);
    assert(nested_type_error.path[0].field_name == "address");
    assert(nested_type_error.runtime_error == simdjson::INCORRECT_TYPE);

    cjm::simdjson::DecodeError child_missing_error;
    const auto child_missing_result = cjm::simdjson::from_json<NestedUser>(
        R"({"id":7,"address":{}})", child_missing_error);

    assert(!child_missing_result.has_value());
    assert(child_missing_error.code ==
           cjm::simdjson::DecodeErrorCode::missing_required_field);
    assert(child_missing_error.path.size() == 2);
    assert(child_missing_error.path[0].kind ==
           cjm::simdjson::DecodePathSegmentKind::field);
    assert(child_missing_error.path[0].field_name == "address");
    assert(child_missing_error.path[1].kind ==
           cjm::simdjson::DecodePathSegmentKind::field);
    assert(child_missing_error.path[1].field_name == "city");
    assert(child_missing_error.runtime_error == simdjson::SUCCESS);

    return 0;
}
