#include <catch2/catch_test_macros.hpp>

struct FloatingValues {
    float ratio = 0;
    double amount = 0;
};

#include "tests/golden/simdjson_floating.expected.cjm.hpp"

TEST_CASE("float.values", "[simdjson][decoder]") {
    cjm::simdjson::DecodeError error;
    const auto result = cjm::simdjson::from_json<FloatingValues>(
        R"({"ratio":1.5,"amount":-2.25})", error);

    REQUIRE(result.has_value());
    REQUIRE(result->ratio == 1.5f);
    REQUIRE(result->amount == -2.25f);
    REQUIRE(error.code == cjm::simdjson::DecodeErrorCode::none);
    REQUIRE(error.path.empty());
    REQUIRE(error.runtime_error == ::simdjson::SUCCESS);
}
