#include <array>
#include <cassert>

struct ArrayValues {
    std::array<int, 4> samples;
};

struct ZeroArrayValues {
    std::array<int, 0> samples;
};

#include "tests/golden/simdjson_array.expected.cjm.hpp"

int main() {
    {
        cjm::simdjson::DecodeError error;
        const auto result = cjm::simdjson::from_json<ArrayValues>(
            R"({"samples":[10,20,30,40]})", error);

        assert(result.has_value());
        const auto& value = result.value();
        assert(value.samples[0] == 10);
        assert(value.samples[1] == 20);
        assert(value.samples[2] == 30);
        assert(value.samples[3] == 40);
        assert(error.code == cjm::simdjson::DecodeErrorCode::none);
        assert(error.path.empty());
        assert(error.runtime_error == simdjson::SUCCESS);
    }
    {
        cjm::simdjson::DecodeError error;
        const auto result = cjm::simdjson::from_json<ArrayValues>(
            R"({"samples":"not-an-array"})", error);

        assert(!result.has_value());
        assert(error.code == cjm::simdjson::DecodeErrorCode::expected_array);
        assert(error.path.size() == 1);
        assert(error.path[0].kind ==
               cjm::simdjson::DecodePathSegmentKind::field);
        assert(error.path[0].field_name == "samples");
        assert(error.runtime_error == simdjson::INCORRECT_TYPE);
    }
    {
        cjm::simdjson::DecodeError error;
        const auto result = cjm::simdjson::from_json<ArrayValues>(
            R"({"samples":[10,20,30]})", error);

        assert(!result.has_value());
        assert(error.code ==
               cjm::simdjson::DecodeErrorCode::fixed_array_extent_mismatch);
        assert(error.path.size() == 1);
        assert(error.path[0].kind ==
               cjm::simdjson::DecodePathSegmentKind::field);
        assert(error.path[0].field_name == "samples");
        assert(error.runtime_error == simdjson::SUCCESS);
    }
    {
        cjm::simdjson::DecodeError error;
        const auto result = cjm::simdjson::from_json<ArrayValues>(
            R"({"samples":[10,20,30,40,50]})", error);

        assert(!result.has_value());
        assert(error.code ==
               cjm::simdjson::DecodeErrorCode::fixed_array_extent_mismatch);
        assert(error.path.size() == 1);
        assert(error.path[0].kind ==
               cjm::simdjson::DecodePathSegmentKind::field);
        assert(error.path[0].field_name == "samples");
        assert(error.runtime_error == simdjson::SUCCESS);
    }
    {
        cjm::simdjson::DecodeError error;
        const auto result = cjm::simdjson::from_json<ArrayValues>(
            R"({"samples":[10,"bad",30,40]})", error);

        assert(!result.has_value());
        assert(error.code == cjm::simdjson::DecodeErrorCode::expected_integer);
        assert(error.path.size() == 2);
        assert(error.path[0].kind ==
               cjm::simdjson::DecodePathSegmentKind::field);
        assert(error.path[0].field_name == "samples");
        assert(error.path[1].kind ==
               cjm::simdjson::DecodePathSegmentKind::index);
        assert(error.path[1].index == 1);
        assert(error.runtime_error == simdjson::INCORRECT_TYPE);
    }
    {
        cjm::simdjson::DecodeError error;
        const auto result = cjm::simdjson::from_json<ZeroArrayValues>(
            R"({"samples":[]})", error);

        assert(result.has_value());
        assert(result.value().samples.empty());
        assert(error.code == cjm::simdjson::DecodeErrorCode::none);
        assert(error.path.empty());
        assert(error.runtime_error == simdjson::SUCCESS);
    }

    return 0;
}
