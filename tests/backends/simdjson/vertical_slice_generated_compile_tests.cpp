#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

struct SliceAddress {
    std::string city;
};

struct SliceUser {
    std::int64_t id = 0;
    std::string name;
    std::optional<std::int64_t> maybe_count;
    SliceAddress address;
};

#include "tests/golden/simdjson_vertical_slice.expected.cjm.hpp"

void expect_field_path(const cjm::simdjson::DecodeError& error,
                       std::size_t index, std::string_view field_name) {
    assert(error.path.size() > index);
    assert(error.path[index].kind ==
           cjm::simdjson::DecodePathSegmentKind::field);
    assert(error.path[index].field_name == field_name);
}

void run_success_case() {
    cjm::simdjson::DecodeError error;
    const auto result = cjm::simdjson::from_json<SliceUser>(
        R"({"address":{"ignored":true,"city":"Paris"},"maybe_count":42,"name":"Ada","id":7})",
        error);

    assert(result.has_value());
    assert(result->id == 7);
    assert(result->name == "Ada");
    assert(result->maybe_count.has_value());
    assert(*result->maybe_count == 42);
    assert(result->address.city == "Paris");
    assert(error.code == cjm::simdjson::DecodeErrorCode::none);
    assert(error.path.empty());
    assert(error.runtime_error == simdjson::SUCCESS);
}

void run_optional_type_mismatch_case() {
    cjm::simdjson::DecodeError error;
    const auto result = cjm::simdjson::from_json<SliceUser>(
        R"({"id":7,"name":"Ada","maybe_count":"many","address":{"city":"Paris"}})",
        error);

    assert(!result.has_value());
    assert(error.code == cjm::simdjson::DecodeErrorCode::expected_integer);
    assert(error.path.size() == 1);
    expect_field_path(error, 0, "maybe_count");
    assert(error.runtime_error == simdjson::INCORRECT_TYPE);
}

void run_nested_non_object_case() {
    cjm::simdjson::DecodeError error;
    const auto result = cjm::simdjson::from_json<SliceUser>(
        R"({"id":7,"name":"Ada","address":false})", error);

    assert(!result.has_value());
    assert(error.code == cjm::simdjson::DecodeErrorCode::expected_object);
    assert(error.path.size() == 1);
    expect_field_path(error, 0, "address");
    assert(error.runtime_error == simdjson::INCORRECT_TYPE);
}

void run_nested_child_missing_case() {
    cjm::simdjson::DecodeError error;
    const auto result = cjm::simdjson::from_json<SliceUser>(
        R"({"id":7,"name":"Ada","address":{}})", error);

    assert(!result.has_value());
    assert(error.code ==
           cjm::simdjson::DecodeErrorCode::missing_required_field);
    assert(error.path.size() == 2);
    expect_field_path(error, 0, "address");
    expect_field_path(error, 1, "city");
    assert(error.runtime_error == simdjson::SUCCESS);
}

int main(int argc, char** argv) {
    assert(argc == 2);
    const std::string_view scenario = argv[1];

    if (scenario == "success") {
        run_success_case();
        return 0;
    }
    if (scenario == "optional_type_mismatch") {
        run_optional_type_mismatch_case();
        return 0;
    }
    if (scenario == "nested_non_object") {
        run_nested_non_object_case();
        return 0;
    }
    if (scenario == "nested_child_missing") {
        run_nested_child_missing_case();
        return 0;
    }

    assert(false);
    return 1;
}
