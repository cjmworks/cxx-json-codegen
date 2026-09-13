#include <catch2/catch_test_macros.hpp>
#include <simdjson.h>
#include <cstdint>
#include <limits>
#include <string_view>

TEST_CASE("string_builder.writes_object_in_cpp17", "[simdjson][builder]") {
    ::simdjson::builder::string_builder builder;

    builder.start_object();
    builder.escape_and_append_with_quotes("name");
    builder.append_colon();
    builder.escape_and_append_with_quotes("A\nB");
    builder.end_object();

    std::string_view output;
    REQUIRE(builder.view().get(output) == ::simdjson::SUCCESS);
    REQUIRE(output == R"({"name":"A\nB"})");
}

TEST_CASE("string_builder.accepts_invalid_utf8_until_validation",
          "[simdjson][builder]") {
    ::simdjson::builder::string_builder builder;
    const std::string_view invalid_utf8{"\xC3\x28", 2};

    builder.escape_and_append_with_quotes(invalid_utf8);

    std::string_view output;
    REQUIRE(builder.view().get(output) == ::simdjson::SUCCESS);
    REQUIRE_FALSE(builder.validate_unicode());
}

TEST_CASE("string_builder.appends_bool_value", "[simdjson][builder]") {
    ::simdjson::builder::string_builder builder;

    builder.start_object();
    builder.escape_and_append_with_quotes("enabled");
    builder.append_colon();
    builder.append(true);
    builder.end_object();

    std::string_view output;
    REQUIRE(builder.view().get(output) == ::simdjson::SUCCESS);
    REQUIRE(output == R"({"enabled":true})");
}

TEST_CASE("string_builder.appends_signed_integer", "[simdjson][builder]") {
    const struct {
        const char* name;
        std::int64_t value;
        std::string_view expected;
    } cases[] = {
        {"zero", 0, "0"},
        {"negative", -15, "-15"},
        {"minimum", std::numeric_limits<std::int64_t>::min(),
         "-9223372036854775808"},
    };

    for (const auto& item : cases) {
        DYNAMIC_SECTION(item.name) {
            ::simdjson::builder::string_builder builder;
            builder.append(item.value);

            std::string_view output;
            REQUIRE(builder.view().get(output) == ::simdjson::SUCCESS);
            REQUIRE(output == item.expected);
        }
    }
}
