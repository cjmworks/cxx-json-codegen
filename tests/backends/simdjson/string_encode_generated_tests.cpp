#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>

struct StringValues {
    std::string name;
};

#include "tests/golden/simdjson_string.expected.cjm.hpp"

TEST_CASE("string.encode", "[simdjson][encoder]") {
    const struct {
        const char* name;
        std::string input;
        std::string_view expected;
    } cases[] = {
        {"empty", "", R"({"name":""})"},
        {"ascii", "Ada", R"({"name":"Ada"})"},
        {"newline", "A\nB", R"({"name":"A\nB"})"},
        {"quote", "A\"B", R"({"name":"A\"B"})"},
        {"backslash", "A\\B", R"({"name":"A\\B"})"},
        {"chinese", u8"你好", u8R"({"name":"你好"})"},
        {"embedded_nul", std::string{"A\0B", 3}, R"({"name":"A\u0000B"})"},
    };

    for (const auto& item : cases) {
        DYNAMIC_SECTION(item.name) {
            const StringValues value{item.input};
            cjm::simdjson::EncodeError error;
            const auto result = cjm::simdjson::to_json(value, error);

            REQUIRE(result.has_value());
            REQUIRE(*result == item.expected);
            REQUIRE(error.code == cjm::simdjson::EncodeErrorCode::none);
            REQUIRE(error.path.empty());
            REQUIRE(error.runtime_error == ::simdjson::SUCCESS);
        }
    }
}

TEST_CASE("string.invalid_utf8", "[simdjson][encoder]") {
    const struct {
        const char* name;
        std::string input;
    } cases[] = {
        {"invalid", std::string{"\xC3\x28", 2}},
        {"truncated", std::string{"\xC3", 1}},
        {"invalid_after_nul", std::string{"A\0\xC3\x28", 4}},
    };
    for (const auto& item : cases) {
        DYNAMIC_SECTION(item.name) {
            const StringValues value{item.input};
            cjm::simdjson::EncodeError error;
            const auto result = cjm::simdjson::to_json(value, error);

            REQUIRE_FALSE(result.has_value());
            REQUIRE(error.code ==
                    cjm::simdjson::EncodeErrorCode::invalid_utf8_string);
            REQUIRE(error.runtime_error == ::simdjson::UTF8_ERROR);
            REQUIRE(error.path.size() == 1);
            REQUIRE(error.path[0].kind ==
                    cjm::simdjson::EncodePathSegmentKind::field);
            REQUIRE(error.path[0].field_name == "name");
        }
    }
}
