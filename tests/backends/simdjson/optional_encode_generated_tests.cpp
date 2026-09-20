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

TEST_CASE("optional.invalid_utf8", "[simdjson][encoder]") {
    OptionalEncodeValues value;
    value.name = std::string{"\xC3\x28", 2};
    cjm::simdjson::EncodeError error;

    const auto result = cjm::simdjson::to_json(value, error);

    REQUIRE_FALSE(result.has_value());
    REQUIRE(error.code == cjm::simdjson::EncodeErrorCode::invalid_utf8_string);
    REQUIRE(error.runtime_error == ::simdjson::UTF8_ERROR);
    REQUIRE(error.path.size() == 1);
    REQUIRE(error.path[0].kind == cjm::simdjson::EncodePathSegmentKind::field);
    REQUIRE(error.path[0].field_name == "name");
}

TEST_CASE("optional.recovers", "[simdjson][encoder]") {
    OptionalEncodeValues value;
    value.name = std::string{"\xC3\x28", 2};
    cjm::simdjson::EncodeError error;

    const auto failed = cjm::simdjson::to_json(value, error);
    REQUIRE_FALSE(failed.has_value());
    REQUIRE(error.code == cjm::simdjson::EncodeErrorCode::invalid_utf8_string);
    REQUIRE_FALSE(error.path.empty());
    REQUIRE(error.runtime_error == ::simdjson::UTF8_ERROR);

    value.name = "Ada";
    const auto result = cjm::simdjson::to_json(value, error);
    REQUIRE(result.has_value());
    REQUIRE(*result == R"({"name":"Ada","enabled":null})");
    REQUIRE(error.code == cjm::simdjson::EncodeErrorCode::none);
    REQUIRE(error.path.empty());
    REQUIRE(error.runtime_error == ::simdjson::SUCCESS);
}

TEST_CASE("optional.all_omitted", "[simdjson][encoder]") {
    const OmittedOptionalValues value{};
    cjm::simdjson::EncodeError error;

    const auto result = cjm::simdjson::to_json(value, error);

    REQUIRE(result.has_value());
    REQUIRE(*result == "{}");
    REQUIRE(error.code == cjm::simdjson::EncodeErrorCode::none);
    REQUIRE(error.path.empty());
    REQUIRE(error.runtime_error == ::simdjson::SUCCESS);
}

TEST_CASE("optional.ignored", "[simdjson][encoder]") {
    OmittedOptionalValues value;
    value.ignored = std::string{"\xC3\x28", 2};
    value.count = 7;
    cjm::simdjson::EncodeError error;

    const auto result = cjm::simdjson::to_json(value, error);

    REQUIRE(result.has_value());
    REQUIRE(*result == R"({"count":7})");
    REQUIRE(error.code == cjm::simdjson::EncodeErrorCode::none);
    REQUIRE(error.path.empty());
    REQUIRE(error.runtime_error == ::simdjson::SUCCESS);
}

TEST_CASE("optional.enum", "[simdjson][encoder]") {
    const struct {
        const char* name;
        std::optional<OptionalStatus> input;
        std::string_view expected;
    } cases[] = {
        {"absent", std::nullopt, R"({"state":null})"},
        {"active", OptionalStatus::Active, R"({"state":"Active"})"},
        {"disabled", OptionalStatus::Disabled, R"({"state":"Disabled"})"},
    };

    for (const auto& item : cases) {
        DYNAMIC_SECTION(item.name) {
            const OptionalEnumValues value{item.input};
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

TEST_CASE("optional.enum_invalid", "[simdjson][encoder]") {
    const OptionalEnumValues value{static_cast<OptionalStatus>(99)};
    cjm::simdjson::EncodeError error;

    const auto result = cjm::simdjson::to_json(value, error);

    REQUIRE_FALSE(result.has_value());
    REQUIRE(error.code == cjm::simdjson::EncodeErrorCode::invalid_enum_value);
    REQUIRE(error.path.size() == 1);
    REQUIRE(error.path[0].kind == cjm::simdjson::EncodePathSegmentKind::field);
    REQUIRE(error.path[0].field_name == "state");
    REQUIRE(error.runtime_error == ::simdjson::SUCCESS);
}

TEST_CASE("optional.enum_recovers", "[simdjson][encoder]") {
    OptionalEnumValues value{static_cast<OptionalStatus>(99)};
    cjm::simdjson::EncodeError error;

    const auto failed = cjm::simdjson::to_json(value, error);

    REQUIRE_FALSE(failed.has_value());
    REQUIRE(error.code == cjm::simdjson::EncodeErrorCode::invalid_enum_value);
    REQUIRE_FALSE(error.path.empty());

    value.status = OptionalStatus::Active;
    const auto result = cjm::simdjson::to_json(value, error);

    REQUIRE(result.has_value());
    REQUIRE(*result == R"({"state":"Active"})");
    REQUIRE(error.code == cjm::simdjson::EncodeErrorCode::none);
    REQUIRE(error.path.empty());
    REQUIRE(error.runtime_error == ::simdjson::SUCCESS);
}
