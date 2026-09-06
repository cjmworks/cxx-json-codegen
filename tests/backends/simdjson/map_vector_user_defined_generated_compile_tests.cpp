#include <cassert>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

struct GroupedItem {
    std::int64_t id = 0;
};

struct GroupedCatalog {
    std::map<std::string, std::vector<GroupedItem>> groups;
};

#include "tests/golden/simdjson_map_vector_user_defined.expected.cjm.hpp"

int main() {
    {
        cjm::simdjson::DecodeError error;
        const auto result = cjm::simdjson::from_json<GroupedCatalog>(
            R"({"groups":{"admins":[{"id":7},{"id":9}]}})", error);

        assert(result.has_value());
        assert(result->groups.at("admins").size() == 2);
        assert(result->groups.at("admins")[0].id == 7);
        assert(result->groups.at("admins")[1].id == 9);
        assert(error.code == cjm::simdjson::DecodeErrorCode::none);
        assert(error.path.empty());
    }
    {
        cjm::simdjson::DecodeError error;
        const auto result = cjm::simdjson::from_json<GroupedCatalog>(
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
        const auto result = cjm::simdjson::from_json<GroupedCatalog>(
            R"({"groups":{"admins":[{"id":7},false]}})", error);

        assert(!result.has_value());
        assert(error.code == cjm::simdjson::DecodeErrorCode::expected_object);
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
        const auto result = cjm::simdjson::from_json<GroupedCatalog>(
            R"({"groups":{"admins":[{"id":7},{}]}})", error);

        assert(!result.has_value());
        assert(error.code ==
               cjm::simdjson::DecodeErrorCode::missing_required_field);
        assert(error.path.size() == 4);
        assert(error.path[0].field_name == "groups");
        assert(error.path[1].field_name == "admins");
        assert(error.path[2].kind ==
               cjm::simdjson::DecodePathSegmentKind::index);
        assert(error.path[2].index == 1);
        assert(error.path[3].field_name == "id");
        assert(error.runtime_error == simdjson::SUCCESS);
    }

    return 0;
}
