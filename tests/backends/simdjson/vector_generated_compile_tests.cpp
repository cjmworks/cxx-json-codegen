#include <cassert>
#include <string>
#include <vector>

struct VectorValues {
    std::vector<std::string> tags;
};

#include "tests/golden/simdjson_vector.expected.cjm.hpp"

int main() {
    cjm::simdjson::DecodeError error;

    const auto result = cjm::simdjson::from_json<VectorValues>(
        R"({"tags":["cpp","json","simdjson"]})", error);

    assert(result.has_value());
    assert(
        (result->tags == std::vector<std::string>{"cpp", "json", "simdjson"}));
    assert(error.code == cjm::simdjson::DecodeErrorCode::none);
    assert(error.path.empty());
    assert(error.runtime_error == simdjson::SUCCESS);

    cjm::simdjson::DecodeError non_array_error;
    const auto non_array_result = cjm::simdjson::from_json<VectorValues>(
        R"({"tags":"not an array"})", non_array_error);

    assert(!non_array_result.has_value());
    assert(non_array_error.code ==
           cjm::simdjson::DecodeErrorCode::expected_array);
    assert(non_array_error.path.size() == 1);
    assert(non_array_error.path[0].kind ==
           cjm::simdjson::DecodePathSegmentKind::field);
    assert(non_array_error.path[0].field_name == "tags");
    assert(non_array_error.runtime_error == simdjson::INCORRECT_TYPE);

    cjm::simdjson::DecodeError element_error;
    const auto element_result = cjm::simdjson::from_json<VectorValues>(
        R"({"tags":["ok",123]})", element_error);

    assert(!element_result.has_value());
    assert(element_error.code ==
           cjm::simdjson::DecodeErrorCode::expected_string);
    assert(element_error.path.size() == 2);
    assert(element_error.path[0].kind ==
           cjm::simdjson::DecodePathSegmentKind::field);
    assert(element_error.path[0].field_name == "tags");
    assert(element_error.path[1].kind ==
           cjm::simdjson::DecodePathSegmentKind::index);
    assert(element_error.path[1].index == 1);
    assert(element_error.runtime_error == simdjson::INCORRECT_TYPE);

    cjm::simdjson::DecodeError missing_error;
    const auto missing_result =
        cjm::simdjson::from_json<VectorValues>("{}", missing_error);

    assert(!missing_result.has_value());
    assert(missing_error.code ==
           cjm::simdjson::DecodeErrorCode::missing_required_field);
    assert(missing_error.path.size() == 1);
    assert(missing_error.path[0].kind ==
           cjm::simdjson::DecodePathSegmentKind::field);
    assert(missing_error.path[0].field_name == "tags");
    assert(missing_error.runtime_error == simdjson::SUCCESS);

    return 0;
}
