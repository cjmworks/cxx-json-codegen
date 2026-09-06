#include <cassert>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

struct MapVectorValues {
    std::map<std::string, std::vector<std::int32_t>> groups;
};

#include "tests/golden/simdjson_map_vector.expected.cjm.hpp"

int main() {
    {
        cjm::simdjson::DecodeError error;
        const auto result = cjm::simdjson::from_json<MapVectorValues>(
            R"({"groups":{"admins":[1,2],"guests":[3]}})", error);

        assert(result.has_value());
        assert((result->groups.at("admins") ==
                std::vector<std::int32_t>{1, 2}));
        assert((result->groups.at("guests") ==
                std::vector<std::int32_t>{3}));
        assert(error.code == cjm::simdjson::DecodeErrorCode::none);
        assert(error.path.empty());
    }
    {
        cjm::simdjson::DecodeError error;
        const auto result = cjm::simdjson::from_json<MapVectorValues>(
            R"({"groups":{"admins":"bad"}})", error);

        assert(!result.has_value());
        assert(error.code == cjm::simdjson::DecodeErrorCode::expected_array);
        assert(error.path.size() == 2);
        assert(error.path[0].field_name == "groups");
        assert(error.path[1].field_name == "admins");
        assert(error.runtime_error == simdjson::INCORRECT_TYPE);
    }
    {
        cjm::simdjson::DecodeError error;
        const auto result = cjm::simdjson::from_json<MapVectorValues>(
            R"({"groups":{"admins":[1,"bad"]}})", error);

        assert(!result.has_value());
        assert(error.code ==
               cjm::simdjson::DecodeErrorCode::expected_integer);
        assert(error.path.size() == 3);
        assert(error.path[0].field_name == "groups");
        assert(error.path[1].field_name == "admins");
        assert(error.path[2].kind ==
               cjm::simdjson::DecodePathSegmentKind::index);
        assert(error.path[2].index == 1);
        assert(error.runtime_error == simdjson::INCORRECT_TYPE);
    }
    {
        cjm::simdjson::DecodeError error;
        const auto result = cjm::simdjson::from_json<MapVectorValues>(
            R"({"groups":{"admins":[]}})", error);

        assert(result.has_value());
        assert(result->groups.at("admins").empty());
    }
    {
        cjm::simdjson::DecodeError error;
        const auto result = cjm::simdjson::from_json<MapVectorValues>(
            R"({"groups":{}})", error);

        assert(result.has_value());
        assert(result->groups.empty());
    }

    return 0;
}
