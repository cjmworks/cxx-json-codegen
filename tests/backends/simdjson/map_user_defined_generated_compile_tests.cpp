#include <cassert>
#include <cstdint>
#include <map>
#include <string>

struct MapItem {
    std::int64_t id = 0;
};

struct Catalog {
    std::map<std::string, MapItem> items;
};

#include "tests/golden/simdjson_map_user_defined.expected.cjm.hpp"

int main() {
    {
        cjm::simdjson::DecodeError error;
        const auto result = cjm::simdjson::from_json<Catalog>(
            R"({"items":{"alice":{"id":7},"bob":{"id":9}}})", error);

        assert(result.has_value());
        assert(result->items.size() == 2);
        assert(result->items.at("alice").id == 7);
        assert(result->items.at("bob").id == 9);
        assert(error.code == cjm::simdjson::DecodeErrorCode::none);
        assert(error.path.empty());
        assert(error.runtime_error == simdjson::SUCCESS);
    }
    {
        cjm::simdjson::DecodeError error;
        const auto result = cjm::simdjson::from_json<Catalog>(
            R"({"items":{"alice":{"id":7},"bob":false}})", error);

        assert(!result.has_value());
        assert(error.code == cjm::simdjson::DecodeErrorCode::expected_object);
        assert(error.path.size() == 2);
        assert(error.path[0].field_name == "items");
        assert(error.path[1].kind ==
               cjm::simdjson::DecodePathSegmentKind::field);
        assert(error.path[1].field_name == "bob");
        assert(error.runtime_error == simdjson::INCORRECT_TYPE);
    }
    {
        cjm::simdjson::DecodeError error;
        const auto result = cjm::simdjson::from_json<Catalog>(
            R"({"items":{"alice":{"id":7},"bob":{}}})", error);

        assert(!result.has_value());
        assert(error.code ==
               cjm::simdjson::DecodeErrorCode::missing_required_field);
        assert(error.path.size() == 3);
        assert(error.path[0].field_name == "items");
        assert(error.path[1].kind ==
               cjm::simdjson::DecodePathSegmentKind::field);
        assert(error.path[1].field_name == "bob");
        assert(error.path[2].kind ==
               cjm::simdjson::DecodePathSegmentKind::field);
        assert(error.path[2].field_name == "id");
        assert(error.runtime_error == simdjson::SUCCESS);
    }

    return 0;
}
