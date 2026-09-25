#include <catch2/catch_test_macros.hpp>

#include <string_view>

#include "tests/fixtures/simdjson_nested_encode.hpp"
#include "cjm/simdjson/simdjson_nested_encode.cjm.hpp"

TEST_CASE("object.round_trip", "[simdjson][encoder][decoder]") {
    const app::User value{{"Paris"}};
    cjm::simdjson::EncodeError encode_error;

    const auto json = cjm::simdjson::to_json(value, encode_error);
    REQUIRE(json.has_value());
    REQUIRE(*json == R"({"home":{"city":"Paris"}})");
    REQUIRE(encode_error.code == cjm::simdjson::EncodeErrorCode::none);
    REQUIRE(encode_error.path.empty());
    REQUIRE(encode_error.runtime_error == ::simdjson::SUCCESS);

    cjm::simdjson::DecodeError decode_error;
    const auto decoded =
        cjm::simdjson::from_json<app::User>(*json, decode_error);
    REQUIRE(decoded.has_value());
    REQUIRE(decoded->address.city == value.address.city);
    REQUIRE(decode_error.code == cjm::simdjson::DecodeErrorCode::none);
    REQUIRE(decode_error.path.empty());
    REQUIRE(decode_error.runtime_error == ::simdjson::SUCCESS);
}

TEST_CASE("object.error_path", "[simdjson][encoder]") {
    app::User value;
    value.address.city = std::string{"\xC3\x28", 2};
    cjm::simdjson::EncodeError error;

    const auto result = cjm::simdjson::to_json(value, error);

    REQUIRE_FALSE(result.has_value());
    REQUIRE(error.code == cjm::simdjson::EncodeErrorCode::invalid_utf8_string);
    REQUIRE(error.runtime_error == ::simdjson::UTF8_ERROR);
    REQUIRE(error.path.size() == 2);
    REQUIRE(error.path[0].kind == cjm::simdjson::EncodePathSegmentKind::field);
    REQUIRE(error.path[0].field_name == "home");
    REQUIRE(error.path[1].kind == cjm::simdjson::EncodePathSegmentKind::field);
    REQUIRE(error.path[1].field_name == "city");
}

TEST_CASE("object.recovers", "[simdjson][encoder]") {
    app::User value;
    value.address.city = std::string{"\xC3\x28", 2};
    cjm::simdjson::EncodeError error;

    const auto failed = cjm::simdjson::to_json(value, error);
    REQUIRE_FALSE(failed.has_value());
    REQUIRE(error.code == cjm::simdjson::EncodeErrorCode::invalid_utf8_string);
    REQUIRE(error.runtime_error == ::simdjson::UTF8_ERROR);
    REQUIRE(error.path.size() == 2);

    value.address.city = "Paris";
    const auto result = cjm::simdjson::to_json(value, error);

    REQUIRE(result.has_value());
    REQUIRE(*result == R"({"home":{"city":"Paris"}})");
    REQUIRE(error.code == cjm::simdjson::EncodeErrorCode::none);
    REQUIRE(error.runtime_error == ::simdjson::SUCCESS);
    REQUIRE(error.path.empty());
}

TEST_CASE("object.empty_child", "[simdjson][encoder]") {
    const app::UserWithOmittedAddress value{};
    cjm::simdjson::EncodeError error;

    const auto result = cjm::simdjson::to_json(value, error);

    REQUIRE(result.has_value());
    REQUIRE(*result == R"({"home":{}})");
    REQUIRE(error.code == cjm::simdjson::EncodeErrorCode::none);
    REQUIRE(error.path.empty());
    REQUIRE(error.runtime_error == ::simdjson::SUCCESS);
}

TEST_CASE("object.deep_round_trip", "[simdjson][encoder][decoder]") {
    const app::Profile value{7, {{"Paris"}}, true};
    cjm::simdjson::EncodeError encode_error;

    const auto json = cjm::simdjson::to_json(value, encode_error);
    REQUIRE(json.has_value());
    REQUIRE(*json ==
            R"({"id":7,"owner":{"home":{"city":"Paris"}},"enabled":true})");
    REQUIRE(encode_error.code == cjm::simdjson::EncodeErrorCode::none);
    REQUIRE(encode_error.path.empty());
    REQUIRE(encode_error.runtime_error == ::simdjson::SUCCESS);

    cjm::simdjson::DecodeError decode_error;
    const auto decoded =
        cjm::simdjson::from_json<app::Profile>(*json, decode_error);
    REQUIRE(decoded.has_value());
    REQUIRE(decoded->id == value.id);
    REQUIRE(decoded->user.address.city == value.user.address.city);
    REQUIRE(decoded->enabled == value.enabled);
    REQUIRE(decode_error.code == cjm::simdjson::DecodeErrorCode::none);
    REQUIRE(decode_error.path.empty());
    REQUIRE(decode_error.runtime_error == ::simdjson::SUCCESS);
}

TEST_CASE("object.deep_error_path", "[simdjson][encoder]") {
    app::Profile value{7, {{std::string{"\xC3\x28", 2}}}, true};
    cjm::simdjson::EncodeError error;

    const auto result = cjm::simdjson::to_json(value, error);

    REQUIRE_FALSE(result.has_value());
    REQUIRE(error.code == cjm::simdjson::EncodeErrorCode::invalid_utf8_string);
    REQUIRE(error.runtime_error == ::simdjson::UTF8_ERROR);
    REQUIRE(error.path.size() == 3);
    const std::string_view expected[] = {"owner", "home", "city"};
    for (std::size_t i = 0; i < error.path.size(); ++i) {
        CAPTURE(i);
        REQUIRE(error.path[i].kind ==
                cjm::simdjson::EncodePathSegmentKind::field);
        REQUIRE(error.path[i].field_name == expected[i]);
    }
}

TEST_CASE("optional.object_decode", "[simdjson][decoder]") {
    const struct {
        const char* name;
        std::string_view json;
        bool present;
    } cases[]{
        {"missing", R"({})", false},
        {"null", R"({"home":null})", false},
        {"present", R"({"home":{"city":"Paris"}})", true},
    };

    for (const auto& item : cases) {
        DYNAMIC_SECTION(item.name) {
            cjm::simdjson::DecodeError error;
            const auto result =
                cjm::simdjson::from_json<app::OptionalUser>(item.json, error);
            REQUIRE(result.has_value());
            REQUIRE(result->address.has_value() == item.present);
            if (item.present) {
                REQUIRE(result->address->city == "Paris");
            }
            REQUIRE(error.code == cjm::simdjson::DecodeErrorCode::none);
            REQUIRE(error.path.empty());
            REQUIRE(error.runtime_error == ::simdjson::SUCCESS);
        }
    }
}

TEST_CASE("optional.object_encode", "[simdjson][encoder]") {
    const app::OmittedAddress empty{};
    app::OmittedAddress populated;
    populated.city = "Paris";

    const struct {
        const char* name;
        app::OptionalObjectValues value;
        std::string_view expected;
    } cases[] = {
        {"absent", {}, R"({"nullable":null})"},
        {"present",
         {populated, populated},
         R"({"omitted":{"city":"Paris"},"nullable":{"city":"Paris"}})"},
        {"empty_objects", {empty, empty}, R"({"omitted":{},"nullable":{}})"},
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
