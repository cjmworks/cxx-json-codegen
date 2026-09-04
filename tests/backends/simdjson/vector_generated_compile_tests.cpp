#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

enum class Status {
    Active,
    Disabled,
};

struct VectorValues {
    std::vector<std::string> tags;
    std::vector<bool> flags;
    std::vector<std::int32_t> scores;
    std::vector<std::uint32_t> limits;
    std::vector<Status> statuses;
};

#include "tests/golden/simdjson_vector.expected.cjm.hpp"

int main() {
    {
        cjm::simdjson::DecodeError error;

        const auto result = cjm::simdjson::from_json<VectorValues>(
            R"({"tags":["cpp","json","simdjson"],"flags":[true,false],"scores":[1,-2,3],"limits":[0, 42, 4294967295],"statuses":["Active", "Disabled"]})",
            error);

        assert(result.has_value());
        assert((result->tags ==
                std::vector<std::string>{"cpp", "json", "simdjson"}));
        assert((result->flags == std::vector<bool>{true, false}));
        assert((result->scores == std::vector<std::int32_t>{1, -2, 3}));
        assert(
            (result->limits == std::vector<std::uint32_t>{0, 42, 4294967295}));
        assert((result->statuses ==
                std::vector<Status>{Status::Active, Status::Disabled}));
        assert(error.code == cjm::simdjson::DecodeErrorCode::none);
        assert(error.path.empty());
        assert(error.runtime_error == simdjson::SUCCESS);
    }
    {
        cjm::simdjson::DecodeError non_array_error;
        const auto non_array_result = cjm::simdjson::from_json<VectorValues>(
            R"({"tags":"not an array"})", non_array_error);

        assert(!non_array_result.has_value());
        assert(non_array_error.code ==
               cjm::simdjson::DecodeErrorCode::expected_array);
        assert(non_array_error.path.size() == 1);
        assert(non_array_error.path[0].kind ==
               cjm::simdjson::DecodePathSegmentKind::field);
        assert(non_array_error.path[0].field_name == "tags");
        assert(non_array_error.runtime_error == simdjson::INCORRECT_TYPE);
    }
    {
        cjm::simdjson::DecodeError empty_error;
        const auto empty_result = cjm::simdjson::from_json<VectorValues>(
            R"({"tags":[],"flags":[], "scores":[],"limits":[],"statuses":[]})",
            empty_error);
        assert(empty_result.has_value());
        assert(empty_result->tags.empty());
        assert(empty_result->flags.empty());
        assert(empty_result->scores.empty());
        assert(empty_result->limits.empty());
        assert(empty_result->statuses.empty());
        assert(empty_error.code == cjm::simdjson::DecodeErrorCode::none);
        assert(empty_error.path.empty());
        assert(empty_error.runtime_error == simdjson::SUCCESS);
    }
    {
        cjm::simdjson::DecodeError bool_element_error;
        const auto bool_element_result = cjm::simdjson::from_json<VectorValues>(
            R"({"tags":["ok"],"flags":[true,"nope"],"scores":[1],"limits":[1]})",
            bool_element_error);

        assert(!bool_element_result.has_value());
        assert(bool_element_error.code ==
               cjm::simdjson::DecodeErrorCode::expected_bool);
        assert(bool_element_error.path.size() == 2);
        assert(bool_element_error.path[0].kind ==
               cjm::simdjson::DecodePathSegmentKind::field);
        assert(bool_element_error.path[0].field_name == "flags");
        assert(bool_element_error.path[1].kind ==
               cjm::simdjson::DecodePathSegmentKind::index);
        assert(bool_element_error.path[1].index == 1);
        assert(bool_element_error.runtime_error == simdjson::INCORRECT_TYPE);
    }
    {
        cjm::simdjson::DecodeError integer_overflow_error;
        const auto integer_overflow_result = cjm::simdjson::from_json<
            VectorValues>(
            R"({"tags":["ok"], "flags": [true], "scores": [1, 2147483648],"limits":[1]})",
            integer_overflow_error);
        assert(!integer_overflow_result.has_value());
        assert(integer_overflow_error.code ==
               cjm::simdjson::DecodeErrorCode::integer_overflow);
        assert(integer_overflow_error.path.size() == 2);
        assert(integer_overflow_error.path[0].kind ==
               cjm::simdjson::DecodePathSegmentKind::field);
        assert(integer_overflow_error.path[0].field_name == "scores");
        assert(integer_overflow_error.path[1].kind ==
               cjm::simdjson::DecodePathSegmentKind::index);
        assert(integer_overflow_error.path[1].index == 1);
        assert(integer_overflow_error.runtime_error == simdjson::SUCCESS);
    }
    {
        cjm::simdjson::DecodeError element_error;
        const auto element_result = cjm::simdjson::from_json<VectorValues>(
            R"({"tags":["ok",123]})", element_error);

        assert(!element_result.has_value());
        assert(element_error.code ==
               cjm::simdjson::DecodeErrorCode::expected_string);
        assert(element_error.path.size() == 2);
        assert(element_error.path[0].kind ==
               cjm::simdjson::DecodePathSegmentKind::field);
        assert(element_error.path[0].field_name == "tags");
        assert(element_error.path[1].kind ==
               cjm::simdjson::DecodePathSegmentKind::index);
        assert(element_error.path[1].index == 1);
        assert(element_error.runtime_error == simdjson::INCORRECT_TYPE);
    }
    {
        cjm::simdjson::DecodeError missing_error;
        const auto missing_result =
            cjm::simdjson::from_json<VectorValues>("{}", missing_error);

        assert(!missing_result.has_value());
        assert(missing_error.code ==
               cjm::simdjson::DecodeErrorCode::missing_required_field);
        assert(missing_error.path.size() == 1);
        assert(missing_error.path[0].kind ==
               cjm::simdjson::DecodePathSegmentKind::field);
        assert(missing_error.path[0].field_name == "tags");
        assert(missing_error.runtime_error == simdjson::SUCCESS);
    }
    {
        cjm::simdjson::DecodeError unsigned_overflow_error;
        const auto unsigned_overflow_result = cjm::simdjson::from_json<
            VectorValues>(
            R"({"tags":["ok"],"flags":[true],"scores":[1],"limits":[1,4294967296]})",
            unsigned_overflow_error);
        assert(!unsigned_overflow_result.has_value());
        assert(unsigned_overflow_error.code ==
               cjm::simdjson::DecodeErrorCode::integer_overflow);
        assert(unsigned_overflow_error.path.size() == 2);
        assert(unsigned_overflow_error.path[0].kind ==
               cjm::simdjson::DecodePathSegmentKind::field);
        assert(unsigned_overflow_error.path[0].field_name == "limits");
        assert(unsigned_overflow_error.path[1].kind ==
               cjm::simdjson::DecodePathSegmentKind::index);
        assert(unsigned_overflow_error.path[1].index == 1);
        assert(unsigned_overflow_error.runtime_error == simdjson::SUCCESS);
    }
    {
        cjm::simdjson::DecodeError negative_unsigned_error;
        const auto negative_unsigned_result = cjm::simdjson::from_json<
            VectorValues>(
            R"({"tags":["ok"], "flags":[true],"scores":[1], "limits":[1,-1]})",
            negative_unsigned_error);
        assert(!negative_unsigned_result.has_value());
        assert(negative_unsigned_error.code ==
               cjm::simdjson::DecodeErrorCode::expected_unsigned_integer);
        assert(negative_unsigned_error.path.size() == 2);
        assert(negative_unsigned_error.path[0].kind ==
               cjm::simdjson::DecodePathSegmentKind::field);
        assert(negative_unsigned_error.path[0].field_name == "limits");
        assert(negative_unsigned_error.path[1].kind ==
               cjm::simdjson::DecodePathSegmentKind::index);
        assert(negative_unsigned_error.path[1].index == 1);
        assert(negative_unsigned_error.runtime_error ==
               simdjson::INCORRECT_TYPE);
    }
    {
        cjm::simdjson::DecodeError enum_type_error;
        const auto enum_type_result = cjm::simdjson::from_json<VectorValues>(
            R"({"tags":["ok"],"flags":[true],"scores":[1],"limits":[1],"statuses":["Active",123]})",
            enum_type_error);

        assert(!enum_type_result.has_value());
        assert(enum_type_error.code ==
               cjm::simdjson::DecodeErrorCode::expected_string);
        assert(enum_type_error.path.size() == 2);
        assert(enum_type_error.path[0].kind ==
               cjm::simdjson::DecodePathSegmentKind::field);
        assert(enum_type_error.path[0].field_name == "statuses");
        assert(enum_type_error.path[1].kind ==
               cjm::simdjson::DecodePathSegmentKind::index);
        assert(enum_type_error.path[1].index == 1);
        assert(enum_type_error.runtime_error == simdjson::INCORRECT_TYPE);
    }
    {
        cjm::simdjson::DecodeError invalid_enum_error;
        const auto invalid_enum_result = cjm::simdjson::from_json<VectorValues>(
            R"({"tags":["ok"],"flags":[true],"scores":[1],"limits":[1],"statuses":["Active","Paused"]})",
            invalid_enum_error);

        assert(!invalid_enum_result.has_value());
        assert(invalid_enum_error.code ==
               cjm::simdjson::DecodeErrorCode::invalid_enum_string);
        assert(invalid_enum_error.path.size() == 2);
        assert(invalid_enum_error.path[0].kind ==
               cjm::simdjson::DecodePathSegmentKind::field);
        assert(invalid_enum_error.path[0].field_name == "statuses");
        assert(invalid_enum_error.path[1].kind ==
               cjm::simdjson::DecodePathSegmentKind::index);
        assert(invalid_enum_error.path[1].index == 1);
        assert(invalid_enum_error.runtime_error == simdjson::SUCCESS);
    }
    return 0;
}
