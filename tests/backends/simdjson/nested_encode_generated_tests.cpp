#include <catch2/catch_test_macros.hpp>

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
