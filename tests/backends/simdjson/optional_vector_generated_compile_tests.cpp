#include <cassert>
#include <optional>
#include <string>
#include <vector>

struct OptionalVectorValues {
    std::optional<std::vector<std::string>> maybe_tags;
};

#include "tests/golden/simdjson_optional_vector.expected.cjm.hpp"

int main() {
    {
        cjm::simdjson::DecodeError error;
        const auto result =
            cjm::simdjson::from_json<OptionalVectorValues>("{}", error);

        assert(result.has_value());
        assert(!result->maybe_tags.has_value());
        assert(error.code == cjm::simdjson::DecodeErrorCode::none);
        assert(error.path.empty());
        assert(error.runtime_error == simdjson::SUCCESS);
    }
    {
        cjm::simdjson::DecodeError error;
        const auto result = cjm::simdjson::from_json<OptionalVectorValues>(
            R"({"maybe_tags":null})", error);
        assert(result.has_value());
        assert(!result->maybe_tags.has_value());
        assert(error.code == cjm::simdjson::DecodeErrorCode::none);
        assert(error.path.empty());
        assert(error.runtime_error == simdjson::SUCCESS);
    }
    {
        cjm::simdjson::DecodeError error;
        const auto result = cjm::simdjson::from_json<OptionalVectorValues>(
            R"({"maybe_tags":[]})", error);

        assert(result.has_value());
        const auto& value = result.value();
        assert(value.maybe_tags.has_value());
        assert(value.maybe_tags->empty());
        assert(error.code == cjm::simdjson::DecodeErrorCode::none);
        assert(error.path.empty());
        assert(error.runtime_error == simdjson::SUCCESS);
    }
    {
        cjm::simdjson::DecodeError error;
        const auto result = cjm::simdjson::from_json<OptionalVectorValues>(
            R"({"maybe_tags":["a","b"]})", error);

        assert(result.has_value());
        const auto& value = result.value();
        assert(value.maybe_tags.has_value());
        const auto& tags = value.maybe_tags.value();
        assert(tags.size() == 2);
        assert(tags[0] == "a");
        assert(tags[1] == "b");
        assert(error.code == cjm::simdjson::DecodeErrorCode::none);
        assert(error.path.empty());
        assert(error.runtime_error == simdjson::SUCCESS);
    }
    {
        cjm::simdjson::DecodeError error;
        const auto result = cjm::simdjson::from_json<OptionalVectorValues>(
            R"({"maybe_tags":"not-an-array"})", error);

        assert(!result.has_value());
        assert(error.code == cjm::simdjson::DecodeErrorCode::expected_array);
        assert(error.path.size() == 1);
        assert(error.path[0].kind ==
               cjm::simdjson::DecodePathSegmentKind::field);
        assert(error.path[0].field_name == "maybe_tags");
        assert(error.runtime_error == simdjson::INCORRECT_TYPE);
    }
    {
        cjm::simdjson::DecodeError error;
        const auto result = cjm::simdjson::from_json<OptionalVectorValues>(
            R"({"maybe_tags":["ok",42]})", error);

        assert(!result.has_value());
        assert(error.code == cjm::simdjson::DecodeErrorCode::expected_string);
        assert(error.path.size() == 2);
        assert(error.path[0].kind ==
               cjm::simdjson::DecodePathSegmentKind::field);
        assert(error.path[0].field_name == "maybe_tags");
        assert(error.path[1].kind ==
               cjm::simdjson::DecodePathSegmentKind::index);
        assert(error.path[1].index == 1);
        assert(error.runtime_error == simdjson::INCORRECT_TYPE);
    }
    return 0;
}
