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

TEST_CASE("enum.invalid", "[simdjson][encoder]") {
    const EnumValues value{static_cast<Status>(99)};
    cjm::simdjson::EncodeError error;

    const auto result = cjm::simdjson::to_json(value, error);

    REQUIRE_FALSE(result.has_value());
    REQUIRE(error.code == cjm::simdjson::EncodeErrorCode::invalid_enum_value);
    REQUIRE(error.path.size() == 1);
    REQUIRE(error.path[0].kind == cjm::simdjson::EncodePathSegmentKind::field);
    REQUIRE(error.path[0].field_name == "status");
    REQUIRE(error.runtime_error == ::simdjson::SUCCESS);
}

TEST_CASE("enum.recovers", "[simdjson][encoder]") {
    cjm::simdjson::EncodeError error;

    const EnumValues invalid{static_cast<Status>(99)};
    const auto failed = cjm::simdjson::to_json(invalid, error);
    REQUIRE_FALSE(failed.has_value());
    REQUIRE(error.code == cjm::simdjson::EncodeErrorCode::invalid_enum_value);
    REQUIRE_FALSE(error.path.empty());

    const EnumValues valid{Status::Active};
    const auto result = cjm::simdjson::to_json(valid, error);
    REQUIRE(result.has_value());
    REQUIRE(*result == R"({"status":"Active"})");
    REQUIRE(error.code == cjm::simdjson::EncodeErrorCode::none);
    REQUIRE(error.path.empty());
    REQUIRE(error.runtime_error == ::simdjson::SUCCESS);
}
