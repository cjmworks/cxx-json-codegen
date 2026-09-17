#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <limits>
#include <string_view>

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
    REQUIRE(result->amount == -2.25);
    REQUIRE(error.code == cjm::simdjson::DecodeErrorCode::none);
    REQUIRE(error.path.empty());
    REQUIRE(error.runtime_error == ::simdjson::SUCCESS);
}

TEST_CASE("float.overflow", "[simdjson][decoder]") {
    const struct {
        const char* name;
        std::string_view json;
    } cases[] = {
        {"positive", R"({"ratio":1e100, "amount":0})"},
        {"negative", R"({"ratio":-1e100, "amount":0})"},
    };

    for (const auto& item : cases) {
        DYNAMIC_SECTION(item.name) {
            cjm::simdjson::DecodeError error;
            const auto result =
                cjm::simdjson::from_json<FloatingValues>(item.json, error);

            REQUIRE_FALSE(result.has_value());
            REQUIRE(error.code ==
                    cjm::simdjson::DecodeErrorCode::floating_point_overflow);
            REQUIRE(error.runtime_error == ::simdjson::SUCCESS);
            REQUIRE(error.path.size() == 1);
            REQUIRE(error.path[0].kind ==
                    cjm::simdjson::DecodePathSegmentKind::field);
            REQUIRE(error.path[0].field_name == "ratio");
        }
    }
}

TEST_CASE("float.read_error", "[simdjson][decoder]") {
    const struct {
        const char* name;
        std::string_view json;
        simdjson::error_code expected;
        std::string_view field;
    } cases[] = {
        {"wrong_type", R"({"ratio":"1.5", "amount":0})",
         simdjson::INCORRECT_TYPE, "ratio"},
        {"double_overflow", R"({"ratio":1e400, "amount":0})",
         simdjson::NUMBER_ERROR, "ratio"},
        {"double_wrong_type", R"({"ratio":0,"amount":"1.5"})",
         simdjson::INCORRECT_TYPE, "amount"},
        {"double_positive_overflow", R"({"ratio":0,"amount":1e400})",
         simdjson::NUMBER_ERROR, "amount"},
        {"double_negative_overflow", R"({"ratio":0,"amount":-1e400})",
         simdjson::NUMBER_ERROR, "amount"},
    };

    for (const auto& item : cases) {
        DYNAMIC_SECTION(item.name) {
            cjm::simdjson::DecodeError error;
            const auto result =
                cjm::simdjson::from_json<FloatingValues>(item.json, error);

            REQUIRE_FALSE(result.has_value());
            REQUIRE(error.code ==
                    cjm::simdjson::DecodeErrorCode::expected_number);
            REQUIRE(error.runtime_error == item.expected);
            REQUIRE(error.path.size() == 1);
            REQUIRE(error.path[0].kind ==
                    cjm::simdjson::DecodePathSegmentKind::field);
            REQUIRE(error.path[0].field_name == item.field);
        }
    }
}

TEST_CASE("float.boundaries", "[simdjson][decoder]") {
    const struct {
        const char* name;
        std::string_view json;
        float ratio;
        double amount;
    } cases[] = {
        {"positive_max",
         R"({"ratio":3.4028234663852886e38,"amount":1.7976931348623157e308})",
         (std::numeric_limits<float>::max)(),
         (std::numeric_limits<double>::max)()},
        {"negative_max",
         R"({"ratio":-3.4028234663852886e38,"amount":-1.7976931348623157e308})",
         -(std::numeric_limits<float>::max)(),
         -(std::numeric_limits<double>::max)()},
        {"smallest_subnormal",
         R"({"ratio":1.401298464324817e-45,"amount":4.9406564584124654e-324})",
         std::numeric_limits<float>::denorm_min(),
         std::numeric_limits<double>::denorm_min()},
        {"decimal_rounding", R"({"ratio":0.1,"amount":0.1})", 0.1f, 0.1},
    };

    for (const auto& item : cases) {
        DYNAMIC_SECTION(item.name) {
            cjm::simdjson::DecodeError error;
            const auto result =
                cjm::simdjson::from_json<FloatingValues>(item.json, error);
            REQUIRE(result.has_value());
            REQUIRE(result->ratio == item.ratio);
            REQUIRE(result->amount == item.amount);
            REQUIRE(error.code == cjm::simdjson::DecodeErrorCode::none);
            REQUIRE(error.path.empty());
            REQUIRE(error.runtime_error == simdjson::SUCCESS);
        }
    }
}

TEST_CASE("float.signed_zero", "[simdjson][decoder]") {
    const struct {
        const char* name;
        std::string_view json;
        bool negative;
    } cases[] = {
        {"positive_zero", R"({"ratio":0.0,"amount":0.0})", false},
        {"negative_zero", R"({"ratio":-0.0,"amount":-0.0})", true},
        {"positive_underflow", R"({"ratio":1e-100,"amount":1e-400})", false},
        {"negative_underflow", R"({"ratio":-1e-100,"amount":-1e-400})", true},
    };

    for (const auto& item : cases) {
        DYNAMIC_SECTION(item.name) {
            cjm::simdjson::DecodeError error;
            const auto result =
                cjm::simdjson::from_json<FloatingValues>(item.json, error);
            REQUIRE(result.has_value());
            REQUIRE(result->ratio == 0.0f);
            REQUIRE(result->amount == 0.0);
            REQUIRE(std::signbit(result->ratio) == item.negative);
            REQUIRE(std::signbit(result->amount) == item.negative);
            REQUIRE(error.code == cjm::simdjson::DecodeErrorCode::none);
            REQUIRE(error.path.empty());
            REQUIRE(error.runtime_error == simdjson::SUCCESS);
        }
    }
}

TEST_CASE("float.recovers", "[simdjson][decoder]") {
    const struct {
        const char* name;
        std::string_view json;
        cjm::simdjson::DecodeErrorCode code;
        simdjson::error_code runtime_error;
    } cases[] = {
        {"target_overflow", R"({"ratio":1e100,"amount":0})",
         cjm::simdjson::DecodeErrorCode::floating_point_overflow,
         simdjson::SUCCESS},
        {"read_error", R"({"ratio":"bad","amount":0})",
         cjm::simdjson::DecodeErrorCode::expected_number,
         simdjson::INCORRECT_TYPE},
    };

    for (const auto& item : cases) {
        DYNAMIC_SECTION(item.name) {
            cjm::simdjson::DecodeError error;
            const auto failed =
                cjm::simdjson::from_json<FloatingValues>(item.json, error);
            REQUIRE_FALSE(failed.has_value());
            REQUIRE(error.code == item.code);
            REQUIRE(error.runtime_error == item.runtime_error);
            REQUIRE(error.path.size() == 1);
            REQUIRE(error.path[0].field_name == "ratio");

            const auto recovered = cjm::simdjson::from_json<FloatingValues>(
                R"({"ratio":1.5,"amount":-2.25})", error);
            REQUIRE(recovered.has_value());
            REQUIRE(recovered->ratio == 1.5f);
            REQUIRE(recovered->amount == -2.25);
            REQUIRE(error.code == cjm::simdjson::DecodeErrorCode::none);
            REQUIRE(error.path.empty());
            REQUIRE(error.runtime_error == simdjson::SUCCESS);
        }
    }
}

TEST_CASE("float.encode", "[simdjson][encoder]") {
    const FloatingValues value{1.5f, -2.25};
    cjm::simdjson::EncodeError error;

    const auto result = cjm::simdjson::to_json(value, error);

    REQUIRE(result.has_value());
    REQUIRE(*result == R"({"ratio":1.5,"amount":-2.25})");
    REQUIRE(error.code == cjm::simdjson::EncodeErrorCode::none);
    REQUIRE(error.path.empty());
    REQUIRE(error.runtime_error == simdjson::SUCCESS);
    REQUIRE(value.ratio == 1.5f);
    REQUIRE(value.amount == -2.25);
}

TEST_CASE("float.non_finite", "[simdjson][encoder]") {
    const struct {
        const char* name;
        FloatingValues value;
        std::string_view field;
    } cases[] = {
        {"float_nan", {std::numeric_limits<float>::quiet_NaN(), 0}, "ratio"},
        {"float_positive_inf",
         {std::numeric_limits<float>::infinity(), 0},
         "ratio"},
        {"float_negative_inf",
         {-std::numeric_limits<float>::infinity(), 0},
         "ratio"},
        {"double_nan", {0, std::numeric_limits<double>::quiet_NaN()}, "amount"},
        {"double_positive_inf",
         {0, std::numeric_limits<double>::infinity()},
         "amount"},
        {"double_negative_inf",
         {0, -std::numeric_limits<double>::infinity()},
         "amount"},
    };

    for (const auto& item : cases) {
        DYNAMIC_SECTION(item.name) {
            cjm::simdjson::EncodeError error;
            const auto result = cjm::simdjson::to_json(item.value, error);

            REQUIRE_FALSE(result.has_value());
            REQUIRE(error.code ==
                    cjm::simdjson::EncodeErrorCode::non_finite_number);
            REQUIRE(error.path.size() == 1);
            REQUIRE(error.path[0].kind ==
                    cjm::simdjson::EncodePathSegmentKind::field);
            REQUIRE(error.path[0].field_name == item.field);
        }
    }
}

TEST_CASE("float.round_trip", "[simdjson][encoder]") {
    const struct {
        const char* name;
        FloatingValues value;
    } cases[] = {
        {"maximum",
         {(std::numeric_limits<float>::max)(),
          (std::numeric_limits<double>::max)()}},
        {"negative_maximum",
         {-(std::numeric_limits<float>::max)(),
          -(std::numeric_limits<double>::max)()}},
        {"subnormal",
         {std::numeric_limits<float>::denorm_min(),
          std::numeric_limits<double>::denorm_min()}},
        {"negative_zero", {-0.0f, -0.0}},
        {"decimal", {0.1f, 0.1}},
    };

    for (const auto& item : cases) {
        DYNAMIC_SECTION(item.name) {
            cjm::simdjson::EncodeError encode_error;
            const auto json = cjm::simdjson::to_json(item.value, encode_error);
            REQUIRE(json.has_value());
            REQUIRE(encode_error.code == cjm::simdjson::EncodeErrorCode::none);
            REQUIRE(encode_error.path.empty());
            REQUIRE(encode_error.runtime_error == simdjson::SUCCESS);

            cjm::simdjson::DecodeError decode_error;
            const auto result =
                cjm::simdjson::from_json<FloatingValues>(*json, decode_error);
            REQUIRE(result.has_value());
            REQUIRE(decode_error.code == cjm::simdjson::DecodeErrorCode::none);
            REQUIRE(result->ratio == item.value.ratio);
            REQUIRE(result->amount == item.value.amount);
            REQUIRE(std::signbit(result->ratio) ==
                    std::signbit(item.value.ratio));
            REQUIRE(std::signbit(result->amount) ==
                    std::signbit(item.value.amount));
        }
    }
}

TEST_CASE("float.encode_recovers", "[simdjson][encoder]") {
    cjm::simdjson::EncodeError error;
    const FloatingValues invalid{1.5f,
                                 std::numeric_limits<double>::quiet_NaN()};

    const auto failed = cjm::simdjson::to_json(invalid, error);
    REQUIRE_FALSE(failed.has_value());
    REQUIRE(error.code == cjm::simdjson::EncodeErrorCode::non_finite_number);
    REQUIRE(error.path.size() == 1);
    REQUIRE(error.path[0].field_name == "amount");

    const FloatingValues valid{1.5f, -2.25};
    const auto recovered = cjm::simdjson::to_json(valid, error);
    REQUIRE(recovered.has_value());
    REQUIRE(*recovered == R"({"ratio":1.5,"amount":-2.25})");
    REQUIRE(error.code == cjm::simdjson::EncodeErrorCode::none);
    REQUIRE(error.path.empty());
    REQUIRE(error.runtime_error == simdjson::SUCCESS);
}
