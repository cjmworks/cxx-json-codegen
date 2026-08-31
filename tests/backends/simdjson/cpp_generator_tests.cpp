#include "backends/simdjson/cpp_generator.hpp"

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

using cjm::metadata::FieldModel;
using cjm::metadata::FieldType;
using cjm::metadata::FieldTypeKind;
using cjm::metadata::JsonFieldMetadata;
using cjm::metadata::ProjectModel;
using cjm::metadata::SourceLocation;
using cjm::metadata::TypeModel;

// Read one expected generated file.
std::string read_file(const std::string& path) {
    std::ifstream file(path);
    assert(file.is_open());

    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

// Build one required field whose JSON name matches its C++ member name.
FieldModel make_required_field(const std::string& name, FieldTypeKind kind,
                               const std::string& spelling) {
    return FieldModel{
        name,
        FieldType{kind, spelling, spelling},
        JsonFieldMetadata{name, false, false},
        SourceLocation{"tests/fixtures/scalar_values.hpp", 1, 1},
    };
}

ProjectModel make_single_field_project(const std::string& model_name,
                                       const FieldModel& field) {
    TypeModel type;
    type.name = model_name;
    type.qualified_name = model_name;
    type.fields = {field};

    ProjectModel project;
    project.types = {type};
    return project;
}

void expect_unsupported_capability(
    const cjm::generator::simdjson::GenerationResult& result,
    const std::string& model_name, const std::string& field_name,
    const std::string& json_name, const std::string& type_name,
    const std::string& reason) {
    assert(!result.success);
    assert(result.header.empty());
    assert(result.error.find("unsupported capability") != std::string::npos);
    assert(result.error.find("model '" + model_name + "'") !=
           std::string::npos);
    assert(result.error.find("field '" + field_name + "'") !=
           std::string::npos);
    assert(result.error.find("json field '" + json_name + "'") !=
           std::string::npos);
    assert(result.error.find(type_name) != std::string::npos);
    assert(result.error.find(reason) != std::string::npos);
}

ProjectModel make_bool_project() {
    TypeModel type;
    type.name = "BoolValues";
    type.qualified_name = "BoolValues";
    type.source_location =
        SourceLocation{"tests/fixtures/bool_values.hpp", 1, 1};
    type.fields = {
        make_required_field("enabled", FieldTypeKind::Bool, "bool"),
    };

    ProjectModel project;
    project.types = {type};
    return project;
}

// Build one model containing a required signed and unsigned integer fields.
ProjectModel make_integer_project() {
    TypeModel type;
    type.name = "IntegerValues";
    type.qualified_name = "IntegerValues";
    type.fields = {
        make_required_field("count", FieldTypeKind::SignedInteger,
                            "std::int32_t"),
        make_required_field("limit", FieldTypeKind::UnsignedInteger,
                            "std::uint32_t"),
        make_required_field("narrow", FieldTypeKind::SignedInteger,
                            "std::int8_t"),
    };

    ProjectModel project;
    project.types = {type};
    return project;
}

// Build a model containing a required vector of owned strings.
ProjectModel make_vector_project() {
    auto tags_field = make_required_field("tags", FieldTypeKind::Vector,
                                          "std::vector<std::string>");
    tags_field.type.qualified_name = "std::vector";
    tags_field.type.arguments = {
        FieldType{
            FieldTypeKind::String,
            "std::string",
            "std::string",
        },
    };

    auto flags_field = make_required_field("flags", FieldTypeKind::Vector,
                                           "std::vector<bool>");
    flags_field.type.qualified_name = "std::vector";
    flags_field.type.arguments = {
        FieldType{
            FieldTypeKind::Bool,
            "bool",
            "bool",
        },
    };

    auto scores_field = make_required_field("scores", FieldTypeKind::Vector,
                                            "std::vector<std::int32_t>");
    scores_field.type.qualified_name = "std::vector";
    scores_field.type.arguments = {
        FieldType{
            FieldTypeKind::SignedInteger,
            "std::int32_t",
            "std::int32_t",
        },
    };
    auto limits_field = make_required_field("limits", FieldTypeKind::Vector,
                                            "std::vector<std::uint32_t>");
    limits_field.type.qualified_name = "std::vector";
    limits_field.type.arguments = {
        FieldType{
            FieldTypeKind::UnsignedInteger,
            "std::uint32_t",
            "std::uint32_t",
        },
    };

    auto statuses_field = make_required_field("statuses", FieldTypeKind::Vector,
                                              "std::vector<Status>");
    statuses_field.type.qualified_name = "std::vector";
    statuses_field.type.arguments = {
        FieldType{
            FieldTypeKind::Enum,
            "Status",
            "Status",
        },
    };

    TypeModel type;
    type.name = "VectorValues";
    type.qualified_name = "VectorValues";
    type.fields = {tags_field, flags_field, scores_field, limits_field,
                   statuses_field};

    cjm::metadata::EnumModel status;
    status.name = "Status";
    status.qualified_name = "Status";
    status.enumerators = {"Active", "Disabled"};

    ProjectModel project;
    project.types = {type};
    project.enums = {status};
    return project;
}

// Build one model containing an optional inner type outside the current
// simdjson slice.
ProjectModel make_optional_vector_project() {
    FieldType vector_inner{
        FieldTypeKind::String,
        "std::string",
        "std::string",
    };
    FieldType inner{
        FieldTypeKind::Vector,
        "std::vector<std::string>",
        "std::vector",
        {vector_inner},
    };
    FieldType optional_type{
        FieldTypeKind::Optional,
        "std::optional<std::vector<std::string>>",
        "std::optional",
        {inner},
    };

    TypeModel type;
    type.name = "OptionalVectorValues";
    type.qualified_name = "OptionalVectorValues";
    type.fields = {
        FieldModel{
            "maybe_tags",
            optional_type,
            JsonFieldMetadata{"maybe_tags", false, false},
            SourceLocation{"tests/fixtures/optional_vector_values.hpp", 1, 1},
        },
    };

    ProjectModel project;
    project.types = {type};
    return project;
}

// Build one model containing a required owned string field.
ProjectModel make_string_project() {
    TypeModel type;
    type.name = "StringValues";
    type.qualified_name = "StringValues";
    type.fields = {
        make_required_field("name", FieldTypeKind::String, "std::string")};

    ProjectModel project;
    project.types = {type};
    return project;
}

// Build one model containing a required enum string field.
ProjectModel make_enum_project() {
    TypeModel type;
    type.name = "EnumValues";
    type.qualified_name = "EnumValues";
    type.fields = {
        make_required_field("status", FieldTypeKind::Enum, "Status"),
    };

    cjm::metadata::EnumModel status;
    status.name = "Status";
    status.qualified_name = "Status";
    status.enumerators = {"Active", "Disabled"};

    ProjectModel project;
    project.types = {type};
    project.enums = {status};
    return project;
}

// Build one model containing supported optional scalar fields.
ProjectModel make_optional_scalar_project() {
    FieldType bool_inner{
        FieldTypeKind::Bool,
        "bool",
        "bool",
    };
    FieldType signed_inner{
        FieldTypeKind::SignedInteger,
        "std::int64_t",
        "std::int64_t",
    };

    FieldType unsigned_inner{
        FieldTypeKind::UnsignedInteger,
        "std::uint64_t",
        "std::uint64_t",
    };

    FieldType string_inner{
        FieldTypeKind::String,
        "std::string",
        "std::string",
    };

    FieldType enum_inner{
        FieldTypeKind::Enum,
        "Status",
        "Status",
    };

    FieldType optional_bool_type{
        FieldTypeKind::Optional,
        "std::optional<bool>",
        "std::optional",
        {bool_inner},
    };

    FieldType optional_signed_type{
        FieldTypeKind::Optional,
        "std::optional<std::int64_t>",
        "std::optional",
        {signed_inner},
    };
    FieldType optional_unsigned_type{
        FieldTypeKind::Optional,
        "std::optional<std::uint64_t>",
        "std::optional",
        {unsigned_inner},
    };
    FieldType optional_string_type{
        FieldTypeKind::Optional,
        "std::optional<std::string>",
        "std::optional",
        {string_inner},
    };
    FieldType optional_enum_type{
        FieldTypeKind::Optional,
        "std::optional<Status>",
        "std::optional",
        {enum_inner},
    };

    TypeModel type;
    type.name = "OptionalScalarValues";
    type.qualified_name = "OptionalScalarValues";
    type.fields = {
        FieldModel{
            "maybe_enabled",
            optional_bool_type,
            JsonFieldMetadata{"maybe_enabled", false, false},
            SourceLocation{
                "tests/fixtures/optional_scalar_values.hpp",
                1,
                1,
            },
        },
        FieldModel{
            "maybe_count",
            optional_signed_type,
            JsonFieldMetadata{"maybe_count", false, false},
            SourceLocation{
                "tests/fixtures/optional_scalar_values.hpp",
                1,
                1,
            },

        },
        FieldModel{
            "maybe_limit",
            optional_unsigned_type,
            JsonFieldMetadata{"maybe_limit", false, false},
            SourceLocation{
                "tests/fixtures/optional_scalar_values.hpp",
                1,
                1,
            },
        },
        FieldModel{
            "maybe_name",
            optional_string_type,
            JsonFieldMetadata{"maybe_name", false, false},
            SourceLocation{
                "tests/fixtures/optional_scalar_values.hpp",
                1,
                1,
            },
        },
        FieldModel{
            "maybe_status",
            optional_enum_type,
            JsonFieldMetadata{"maybe_status", false, false},
            SourceLocation{
                "tests/fixtures/optional_scalar_values.hpp",
                1,
                1,
            },
        },
    };

    cjm::metadata::EnumModel status;
    status.name = "Status";
    status.qualified_name = "Status";
    status.enumerators = {"Active", "Disabled"};

    ProjectModel project;
    project.types = {type};
    project.enums = {status};
    return project;
}

// Build one root model containing one required nested generated model.
ProjectModel make_nested_project() {
    TypeModel address;
    address.name = "Address";
    address.qualified_name = "Address";
    address.fields = {
        make_required_field("city", FieldTypeKind::String, "std::string"),
    };

    FieldType address_type{
        FieldTypeKind::UserDefined,
        "Address",
        "Address",
    };

    TypeModel user;
    user.name = "NestedUser";
    user.qualified_name = "NestedUser";
    user.fields = {
        make_required_field("id", FieldTypeKind::SignedInteger, "std::int64_t"),
        FieldModel{
            "address",
            address_type,
            JsonFieldMetadata{"address", false, false},
            SourceLocation{
                "tests/fixtures/nested_user.hpp",
                1,
                1,
            },
        },
    };

    ProjectModel project;
    project.types = {address, user};
    return project;
}

// Build one representative vertical-slice model.
ProjectModel make_vertical_slice_project() {
    TypeModel address;
    address.name = "SliceAddress";
    address.qualified_name = "SliceAddress";
    address.fields = {
        make_required_field("city", FieldTypeKind::String, "std::string"),
    };

    FieldType optional_inner{
        FieldTypeKind::SignedInteger,
        "std::int64_t",
        "std::int64_t",
    };

    FieldType optional_type{
        FieldTypeKind::Optional,
        "std::optional<std::int64_t>",
        "std::optional",
        {optional_inner},
    };

    FieldType address_type{
        FieldTypeKind::UserDefined,
        "SliceAddress",
        "SliceAddress",
    };

    TypeModel user;
    user.name = "SliceUser";
    user.qualified_name = "SliceUser";
    user.fields = {
        make_required_field("id", FieldTypeKind::SignedInteger, "std::int64_t"),
        make_required_field("name", FieldTypeKind::String, "std::string"),
        FieldModel{
            "maybe_count",
            optional_type,
            JsonFieldMetadata{"maybe_count", false, false},
            SourceLocation{
                "tests/fixtures/slice_user.hpp",
                1,
                1,
            },
        },
        FieldModel{
            "address",
            address_type,
            JsonFieldMetadata{"address", false, false},
            SourceLocation{
                "tests/fixtures/slice_user.hpp",
                1,
                1,
            },
        },
    };

    ProjectModel project;
    project.types = {address, user};
    return project;
}

} // namespace

int main() {
    {
        const auto bool_result =
            cjm::generator::simdjson::generate_header(make_bool_project());

        assert(bool_result.success);
        assert(bool_result.error.empty());
        assert(!bool_result.header.empty());
        assert(bool_result.header.find("#include <simdjson.h>") !=
               std::string::npos);
        assert(bool_result.header.find("namespace cjm::simdjson") !=
               std::string::npos);
        assert(bool_result.header.find("from_json<::BoolValues>(") !=
               std::string::npos);
        assert(bool_result.header.find(
                   "detail::decode_object(object, value, error)") !=
               std::string::npos);
        assert(bool_result.header.find("cjm_decode_simdjson_BoolValues") ==
               std::string::npos);

        const auto expected =
            read_file("tests/golden/simdjson_bool.expected.cjm.hpp");
        if (bool_result.header != expected) {
            std::cerr << "generated simdjson header: \n" << bool_result.header;
        }
        assert(bool_result.header == expected);

        assert(bool_result.header.find("bool has_enabled = false;") !=
               std::string::npos);
        assert(bool_result.header.find("for (auto field : object)") !=
               std::string::npos);
        assert(bool_result.header.find("field.unescaped_key().get(key)") !=
               std::string::npos);
        assert(bool_result.header.find(
                   "field.value().get_bool().get(value.enabled)") !=
               std::string::npos);
    }
    {
        const auto integer_result =
            cjm::generator::simdjson::generate_header(make_integer_project());
        assert(integer_result.success);
        assert(integer_result.error.empty());
        assert(integer_result.header.find("from_json<::IntegerValues>(") !=
               std::string::npos);
        assert(integer_result.header.find("get_int64().get(decoded_count)") !=
               std::string::npos);
        assert(integer_result.header.find("get_uint64().get(decoded_limit)") !=
               std::string::npos);
        assert(integer_result.header.find(
                   "(std::numeric_limits<target_type>::max)()") !=
               std::string::npos);
        assert(integer_result.header.find(
                   "DecodeErrorCode::integer_overflow") != std::string::npos);

        const auto integer_expected =
            read_file("tests/golden/simdjson_integer.expected.cjm.hpp");
        if (integer_result.header != integer_expected) {
            std::cerr << "generated simdjson integer header:\n"
                      << integer_result.header;
        }
        assert(integer_result.header == integer_expected);
    }
    {
        const auto vector_result =
            cjm::generator::simdjson::generate_header(make_vector_project());
        assert(vector_result.success);
        assert(vector_result.error.empty());
        assert(vector_result.header.find("from_json<::VectorValues>(") !=
               std::string::npos);
        assert(vector_result.header.find(
                   "::simdjson::ondemand::array decoded_tags_array;") !=
               std::string::npos);
        assert(vector_result.header.find(
                   "std::size_t decoded_tags_index = 0;") != std::string::npos);
        assert(vector_result.header.find(
                   "for (auto decoded_tags_element : decoded_tags_array)") !=
               std::string::npos);
        assert(vector_result.header.find(
                   "value.tags.push_back(decoded_tags_value);") !=
               std::string::npos);
        assert(
            vector_result.header.find(
                "{DecodePathSegmentKind::index, \"\", decoded_tags_index}") !=
            std::string::npos);

        assert(vector_result.header.find("bool decoded_flags_value{};") !=
               std::string::npos);
        assert(
            vector_result.header.find(
                "decoded_flags_element.get_bool().get(decoded_flags_value)") !=
            std::string::npos);
        assert(vector_result.header.find(
                   "value.flags.push_back(decoded_flags_value);") !=
               std::string::npos);

        assert(
            vector_result.header.find("std::int32_t decoded_scores_value{};") !=
            std::string::npos);
        assert(vector_result.header.find(
                   "decoded_scores_element.get_int64().get(decoded_scores)") !=
               std::string::npos);
        assert(vector_result.header.find(
                   "value.scores.push_back(decoded_scores_value);") !=
               std::string::npos);

        assert(vector_result.header.find(
                   "std::uint32_t decoded_limits_value{};") !=
               std::string::npos);
        assert(vector_result.header.find(
                   "decoded_limits_element.get_uint64().get(decoded_limits)") !=
               std::string::npos);

        const auto vector_expected =
            read_file("tests/golden/simdjson_vector.expected.cjm.hpp");
        if (vector_result.header != vector_expected) {
            std::cerr << "generated simdjson vector header: \n"
                      << vector_result.header;
        }
        assert(vector_result.header == vector_expected);

        assert(vector_result.header.find(
                   "::Status decoded_statuses_value{};") != std::string::npos);
        assert(vector_result.header.find("decoded_statuses_element.get_string()"
                                         ".get(decoded_statuses_view)") !=
               std::string::npos);
        assert(vector_result.header.find(
                   "decoded_statuses_view == \"Active\"") !=
               std::string::npos);
        assert(vector_result.header.find(
                   "value.statuses.push_back(decoded_statuses_value);") !=
               std::string::npos);
        assert(vector_result.header.find("{DecodePathSegmentKind::index, \"\", "
                                         "decoded_statuses_index}") !=
               std::string::npos);
    }
    {
        const auto result = cjm::generator::simdjson::generate_header(
            make_optional_vector_project());
        expect_unsupported_capability(
            result, "OptionalVectorValues", "maybe_tags", "maybe_tags",
            "std::optional<std::vector<std::string>>",
            "optional decode is only implemented for scalar inner values");
    }
    {
        const auto result =
            cjm::generator::simdjson::generate_header(make_single_field_project(
                "FloatingPointValues",
                make_required_field("ratio", FieldTypeKind::FloatingPoint,
                                    "double")));
        expect_unsupported_capability(
            result, "FloatingPointValues", "ratio", "ratio", "double",
            "floating-point decode is not implemented");
    }
    {
        const auto result =
            cjm::generator::simdjson::generate_header(make_enum_project());
        assert(result.success);
        assert(result.error.empty());
        assert(result.header.find("from_json<::EnumValues>(") !=
               std::string::npos);
        assert(result.header.find("std::string_view decoded_status_view;") !=
               std::string::npos);
        assert(result.header.find("if (decoded_status_view == \"Active\")") !=
               std::string::npos);
        assert(result.header.find("value.status = ::Status::Active;") !=
               std::string::npos);
        assert(result.header.find("if (decoded_status_view == \"Disabled\")") !=
               std::string::npos);
        assert(result.header.find("value.status = ::Status::Disabled;") !=
               std::string::npos);
        assert(result.header.find("DecodeErrorCode::invalid_enum_string") !=
               std::string::npos);

        const auto enum_expected =
            read_file("tests/golden/simdjson_enum.expected.cjm.hpp");
        if (result.header != enum_expected) {
            std::cerr << "generated simdjson enum header:\n" << result.header;
        }
        assert(result.header == enum_expected);
    }
    {
        auto field = make_required_field("samples", FieldTypeKind::Array,
                                         "std::array<int, 4>");
        field.type.qualified_name = "std::array";
        field.type.arguments = {
            FieldType{FieldTypeKind::SignedInteger, "int", "int"},
        };
        field.type.array_extent = 4;

        const auto result = cjm::generator::simdjson::generate_header(
            make_single_field_project("ArrayValues", field));
        expect_unsupported_capability(result, "ArrayValues", "samples",
                                      "samples", "std::array<int, 4>",
                                      "fixed array decode is not implemented");
    }
    {
        auto field = make_required_field("counts", FieldTypeKind::Map,
                                         "std::map<std::string, int>");
        field.type.qualified_name = "std::map";
        field.type.arguments = {
            FieldType{FieldTypeKind::String, "std::string", "std::string"},
            FieldType{FieldTypeKind::SignedInteger, "int", "int"},
        };

        const auto result = cjm::generator::simdjson::generate_header(
            make_single_field_project("MapValues", field));
        expect_unsupported_capability(result, "MapValues", "counts", "counts",
                                      "std::map<std::string, int>",
                                      "map decode is not implemented");
    }
    {
        const auto string_result =
            cjm::generator::simdjson::generate_header(make_string_project());
        assert(string_result.success);
        assert(string_result.error.empty());
        assert(string_result.header.find("from_json<::StringValues>(") !=
               std::string::npos);
        assert(string_result.header.find(
                   "std::string_view decoded_name_view;") != std::string::npos);
        assert(string_result.header.find(
                   "field.value().get_string().get(decoded_name_view)") !=
               std::string::npos);
        assert(string_result.header.find("DecodeErrorCode::expected_string") !=
               std::string::npos);
        assert(string_result.header.find(
                   "value.name.assign(decoded_name_view.begin("
                   "), decoded_name_view.end())") != std::string::npos);

        const auto string_expected =
            read_file("tests/golden/simdjson_string.expected.cjm.hpp");
        if (string_result.header != string_expected) {
            std::cerr << "generated simdjson string header:\n"
                      << string_result.header;
        }
        assert(string_result.header == string_expected);
    }
    {
        const auto result = cjm::generator::simdjson::generate_header(
            make_optional_scalar_project());
        assert(result.success);
        assert(result.error.empty());
        assert(result.header.find("from_json<::OptionalScalarValues>(") !=
               std::string::npos);

        assert(result.header.find("bool has_maybe_enabled = false;") ==
               std::string::npos);
        assert(result.header.find("bool has_maybe_count = false;") ==
               std::string::npos);
        assert(result.header.find("bool has_maybe_limit = false;") ==
               std::string::npos);
        assert(result.header.find("bool has_maybe_name = false;") ==
               std::string::npos);
        assert(result.header.find("bool has_maybe_status = false;") ==
               std::string::npos);

        assert(result.header.find("value.maybe_enabled = std::nullopt;") !=
               std::string::npos);
        assert(result.header.find("value.maybe_count = std::nullopt;") !=
               std::string::npos);
        assert(result.header.find("value.maybe_limit = std::nullopt;") !=
               std::string::npos);
        assert(result.header.find("value.maybe_name = std::nullopt;") !=
               std::string::npos);
        assert(result.header.find("value.maybe_status = std::nullopt;") !=
               std::string::npos);
        assert(result.header.find("field.value().is_null()") !=
               std::string::npos);

        assert(result.header.find("bool decoded_maybe_enabled_value{};") !=
               std::string::npos);
        assert(
            result.header.find("std::int64_t decoded_maybe_count_value{};") !=
            std::string::npos);
        assert(
            result.header.find("std::uint64_t decoded_maybe_limit_value{};") !=
            std::string::npos);
        assert(result.header.find("std::string decoded_maybe_name_value{};") !=
               std::string::npos);
        assert(result.header.find("::Status decoded_maybe_status_value{};") !=
               std::string::npos);

        assert(result.header.find(
                   "field.value().get_int64().get(decoded_maybe_count)") !=
               std::string::npos);

        assert(result.header.find(
                   "field.value().get_uint64().get(decoded_maybe_limit)") !=
               std::string::npos);
        assert(result.header.find("field.value().get_string().get(decoded_"
                                  "maybe_name_view)") != std::string::npos);
        assert(result.header.find("field.value().get_string().get(decoded_"
                                  "maybe_status_view)") != std::string::npos);

        assert(result.header.find("if (!has_maybe_enabled)") ==
               std::string::npos);
        assert(result.header.find("if (!has_maybe_count)") ==
               std::string::npos);
        assert(result.header.find("if (!has_maybe_limit)") ==
               std::string::npos);
        assert(result.header.find("if (!has_maybe_name)") == std::string::npos);
        assert(result.header.find("if (!has_maybe_status)") ==
               std::string::npos);

        assert(result.header.find(
                   "value.maybe_enabled = decoded_maybe_enabled_value") !=
               std::string::npos);

        assert(result.header.find(
                   "value.maybe_count = decoded_maybe_count_value") !=
               std::string::npos);
        assert(result.header.find(
                   "value.maybe_limit = decoded_maybe_limit_value") !=
               std::string::npos);
        assert(
            result.header.find("value.maybe_name = decoded_maybe_name_value") !=
            std::string::npos);
        assert(result.header.find(
                   "value.maybe_status = decoded_maybe_status_value") !=
               std::string::npos);
        assert(result.header.find(
                   "decoded_maybe_status_value = ::Status::Active") !=
               std::string::npos);
        assert(
            result.header.find(
                "field.value().get_bool().get(decoded_maybe_enabled_value)") !=
            std::string::npos);
        assert(result.header.find("DecodeErrorCode::expected_bool") !=
               std::string::npos);
        assert(result.header.find("DecodeErrorCode::expected_integer") !=
               std::string::npos);
        assert(
            result.header.find("DecodeErrorCode::expected_unsigned_integer") !=
            std::string::npos);
        assert(result.header.find("DecodeErrorCode::expected_string") !=
               std::string::npos);
        assert(result.header.find("DecodeErrorCode::invalid_enum_string") !=
               std::string::npos);
        const auto optional_integer_expected = read_file(
            "tests/golden/simdjson_optional_integer.expected.cjm.hpp");
        if (result.header != optional_integer_expected) {
            std::cerr << "generated simdjson optional scalar header:\n"
                      << result.header;
        }
        assert(result.header == optional_integer_expected);
    }
    {
        const auto result =
            cjm::generator::simdjson::generate_header(make_nested_project());
        assert(result.success);
        assert(result.error.empty());
        assert(result.header.find("from_json<::Address>(") !=
               std::string::npos);
        assert(result.header.find("from_json<::NestedUser>(") !=
               std::string::npos);
        assert(result.header.find(
                   "::simdjson::ondemand::object decoded_address;") !=
               std::string::npos);
        assert(result.header.find(
                   "field.value().get_object().get(decoded_address)") !=
               std::string::npos);
        assert(result.header.find("detail::decode_object(decoded_address, "
                                  "value.address, error)") !=
               std::string::npos);
        assert(result.header.find("error.path.insert(") != std::string::npos);
        assert(result.header.find(
                   "{DecodePathSegmentKind::field, \"address\", 0}") !=
               std::string::npos);

        const auto nested_expected =
            read_file("tests/golden/simdjson_nested.expected.cjm.hpp");
        if (result.header != nested_expected) {
            std::cerr << "generated simdjson nested header:\n" << result.header;
        }
        assert(result.header == nested_expected);
    }
    {
        const auto result = cjm::generator::simdjson::generate_header(
            make_vertical_slice_project());
        assert(result.success);
        assert(result.error.empty());
        assert(result.header.find("from_json<::SliceUser>(") !=
               std::string::npos);
        assert(result.header.find("std::string_view decoded_name_view;") !=
               std::string::npos);
        assert(result.header.find("value.maybe_count = std::nullopt;") !=
               std::string::npos);
        assert(result.header.find("detail::decode_object(decoded_address, "
                                  "value.address, error)") !=
               std::string::npos);

        const auto vertical_slice_expected =
            read_file("tests/golden/simdjson_vertical_slice.expected.cjm.hpp");
        if (result.header != vertical_slice_expected) {
            std::cerr << "generated simdjson vertical slice header:\n"
                      << result.header;
        }
        assert(result.header == vertical_slice_expected);
    }
    return 0;
}
