#include <catch2/catch_test_macros.hpp>

#include <string_view>

enum class Status {
    Active,
    Disabled,
};

struct EnumValues {
    Status status;
};

#include "tests/golden/simdjson_enum.expected.cjm.hpp"

TEST_CASE("enum.encode", "[simdjson][encoder]") {
    const struct {
        const char* name;
        Status input;
        std::string_view expected;
    } cases[] = {
        {"active", Status::Active, R"({"status":"Active"})"},
        {"disabled", Status::Disabled, R"({"status":"Disabled"})"},
    };

    for (const auto& item : cases) {
        DYNAMIC_SECTION(item.name) {
            const EnumValues value{item.input};
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
