#include <cassert>
#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>

struct MapValues {
    std::map<std::string, std::int32_t> counts;
    std::unordered_map<std::string, std::string> labels;
};

#include "tests/golden/simdjson_map.expected.cjm.hpp"

int main() {
    {
        cjm::simdjson::DecodeError error;
        const auto result = cjm::simdjson::from_json<MapValues>(
            R"({"counts":{"a\u006cice":10,"bob":20},"labels":{"a":"one","b":"two"}})",
            error);

        assert(result.has_value());
        assert(result->counts.at("alice") == 10);
        assert(result->counts.at("bob") == 20);
        assert(result->labels.at("a") == "one");
        assert(result->labels.at("b") == "two");
        assert(error.code == cjm::simdjson::DecodeErrorCode::none);
        assert(error.path.empty());
    }
    {
        cjm::simdjson::DecodeError error;
        const auto result = cjm::simdjson::from_json<MapValues>(
            R"({"counts":[],"labels":{}})", error);

        assert(!result.has_value());
        assert(error.code == cjm::simdjson::DecodeErrorCode::expected_object);
        assert(error.path.size() == 1);
        assert(error.path[0].field_name == "counts");
        assert(error.runtime_error == simdjson::INCORRECT_TYPE);
    }
    {
        cjm::simdjson::DecodeError error;
        const auto result = cjm::simdjson::from_json<MapValues>(
            R"({"counts":{"alice":10,"bob":"bad"},"labels":{}})",
            error);

        assert(!result.has_value());
        assert(error.code ==
               cjm::simdjson::DecodeErrorCode::expected_integer);
        assert(error.path.size() == 2);
        assert(error.path[0].field_name == "counts");
        assert(error.path[1].kind ==
               cjm::simdjson::DecodePathSegmentKind::field);
        assert(error.path[1].field_name == "bob");
        assert(error.runtime_error == simdjson::INCORRECT_TYPE);
    }
    {
        cjm::simdjson::DecodeError error;
        const auto result = cjm::simdjson::from_json<MapValues>(
            R"({"counts":{},"labels":{}})", error);

        assert(result.has_value());
        assert(result->counts.empty());
        assert(result->labels.empty());
    }

    return 0;
}
