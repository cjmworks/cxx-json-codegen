#include <catch2/catch_test_macros.hpp>
#include <string_view>

#include "tests/fixtures/simdjson_optional_encode.hpp"
#include "cjm/simdjson/simdjson_optional_encode.cjm.hpp"

TEST_CASE("optional.absent", "[simdjson][encoder]") {
    const OptionalEncodeValues value{};
    cjm::simdjson::EncodeError error;

    const auto result = cjm::simdjson::to_json(value, error);

    REQUIRE(result.has_value());
    REQUIRE(*result == R"({"enabled":null})");
    REQUIRE(error.code == cjm::simdjson::EncodeErrorCode::none);
    REQUIRE(error.path.empty());
    REQUIRE(error.runtime_error == ::simdjson::SUCCESS);
}

TEST_CASE("optional.present", "[simdjson][encoder]") {
    const struct {
        const char* name;
        OptionalEncodeValues value;
        std::string_view expected;
    } cases[] = {
        {"zero",
         {0, std::nullopt, std::nullopt},
         R"({"count":0,"enabled":null})"},
        {"empty_string",
         {std::nullopt, std::string{}, std::nullopt},
         R"({"name":"","enabled":null})"},
        {"false", {std::nullopt, std::nullopt, false}, R"({"enabled":false})"},
        {"all_present",
         {18, std::string{"Ada"}, true},
         R"({"count":18,"name":"Ada","enabled":true})"},
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
