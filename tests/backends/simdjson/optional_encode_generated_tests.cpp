#include <catch2/catch_test_macros.hpp>

#include "tests/fixtures/simdjson_optional_encode.hpp"
#include "cjm/simdjson/simdjson_optional_encode.cjm.hpp"

TEST_CASE("optional.obsent", "[simdjson][encoder]") {
    const OptionalEncodeValues value{};
    cjm::simdjson::EncodeError error;

    const auto result = cjm::simdjson::to_json(value, error);

    REQUIRE(result.has_value());
    REQUIRE(*result == R"({"enabled":null})");
    REQUIRE(error.code == cjm::simdjson::EncodeErrorCode::none);
    REQUIRE(error.path.empty());
    REQUIRE(error.runtime_error == ::simdjson::SUCCESS);
}
