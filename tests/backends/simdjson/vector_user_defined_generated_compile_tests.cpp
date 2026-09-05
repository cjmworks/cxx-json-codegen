#include <cassert>
#include <cstdint>
#include <vector>

struct Item {
    std::int64_t id = 0;
};

struct Order {
    std::vector<Item> items;
};

#include "tests/golden/simdjson_vector_user_defined.expected.cjm.hpp"

int main() {
    {
        cjm::simdjson::DecodeError error;
        const auto result = cjm::simdjson::from_json<Order>(
            R"({"items":[{"id":7},{"id":9}]})", error);

        assert(result.has_value());
        assert(result->items.size() == 2);
        assert(result->items[0].id == 7);
        assert(result->items[1].id == 9);
        assert(error.code == cjm::simdjson::DecodeErrorCode::none);
        assert(error.path.empty());
        assert(error.runtime_error == simdjson::SUCCESS);
    }
    {
        cjm::simdjson::DecodeError error;
        const auto result = cjm::simdjson::from_json<Order>(
            R"({"items":[{"id":7},false]})", error);

        assert(!result.has_value());
        assert(error.code == cjm::simdjson::DecodeErrorCode::expected_object);
        assert(error.path.size() == 2);
        assert(error.path[0].field_name == "items");
        assert(error.path[1].kind ==
               cjm::simdjson::DecodePathSegmentKind::index);
        assert(error.path[1].index == 1);
        assert(error.runtime_error == simdjson::INCORRECT_TYPE);
    }
    {
        cjm::simdjson::DecodeError error;
        const auto result = cjm::simdjson::from_json<Order>(
            R"({"items":[{"id":7},{}]})", error);

        assert(!result.has_value());
        assert(error.code ==
               cjm::simdjson::DecodeErrorCode::missing_required_field);
        assert(error.path.size() == 3);
        assert(error.path[0].field_name == "items");
        assert(error.path[1].kind ==
               cjm::simdjson::DecodePathSegmentKind::index);
        assert(error.path[1].index == 1);
        assert(error.path[2].field_name == "id");
        assert(error.runtime_error == simdjson::SUCCESS);
    }

    return 0;
}
