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
