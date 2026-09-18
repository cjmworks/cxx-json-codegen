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

TEST_CASE("string_builder.appends_unsigned_integer", "[simdjson][builder]") {
    const struct {
        const char* name;
        std::uint64_t value;
        std::string_view expected;
    } cases[] = {
        {"zero", 0, "0"},
        {"positive", 15, "15"},
        {"maximum", std::numeric_limits<std::uint64_t>::max(),
         "18446744073709551615"},
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

TEST_CASE("string_builder.appends_finite_floating_values",
          "[simdjson][builder]") {
    const float price = 1.5f;
    const double change = -2.25;
    ::simdjson::builder::string_builder builder;

    builder.start_array();
    builder.append(price);
    builder.append_comma();
    builder.append(change);
    builder.end_array();

    std::string_view output;
    REQUIRE(builder.view().get(output) == ::simdjson::SUCCESS);
    REQUIRE(output == "[1.5,-2.25]");
}

TEST_CASE("string.utf8", "[simdjson][builder]") {
    const struct {
        const char* name;
        std::string_view input;
        bool valid;
    } cases[] = {
        {"empty", "", true},
        {"ascii", "hello", true},
        {"chinese", u8"你好", true},
        {"embedded_nul", std::string_view{"A\0B", 3}, true},
        {"invalid", std::string_view{"\xC3\x28", 2}, false},
        {"truncated", std::string_view{"\xC3", 1}, false},
    };

    for (const auto& item : cases) {
        DYNAMIC_SECTION(item.name) {
            REQUIRE(::simdjson::validate_utf8(item.input) == item.valid);
        }
    }
}
