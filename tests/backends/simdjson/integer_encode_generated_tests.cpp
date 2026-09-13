#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <limits>
#include <string_view>

struct IntegerValues {
    std::int32_t count = 0;
    std::uint32_t limit = 0;
    std::int8_t narrow = 0;
};

#include "tests/golden/simdjson_integer.expected.cjm.hpp"

TEST_CASE("to_json.integer_values", "[simdjson][encoder]") {
    const struct {
        const char* name;
        IntegerValues value;
        std::string_view expected;
    } cases[] = {
        {"zero", {0, 0, 0}, R"({"count":0,"limit":0,"narrow":0})"},
        {"mixed", {-15, 199, -8}, R"({"count":-15,"limit":199,"narrow":-8})"},
        {"boundaries",
         {
             std::numeric_limits<std::int32_t>::min(),
             std::numeric_limits<std::uint32_t>::max(),
             std::numeric_limits<std::int8_t>::min(),
         },
         R"({"count":-2147483648,"limit":4294967295,"narrow":-128})"},
    };

    for (const auto& item : cases) {
        DYNAMIC_SECTION(item.name) {
            cjm::simdjson::EncodeError error;
            const auto result = cjm::simdjson::to_json(item.value, error);

            REQUIRE(result.has_value());
            REQUIRE(*result == item.expected);
            REQUIRE(error.code == cjm::simdjson::EncodeErrorCode::none);
            REQUIRE(error.path.empty());
            REQUIRE(error.runtime_error == ::simdjson::SUCCESS);
        }
    }
}
