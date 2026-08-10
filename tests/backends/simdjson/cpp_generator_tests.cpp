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

// Build a model containing one type unsupported by the initial spike.
ProjectModel make_vector_project() {
    auto vector_field = make_required_field("tags", FieldTypeKind::Vector,
                                            "std::vector<std::string>");
    vector_field.type.qualified_name = "std::vector";
    vector_field.type.arguments = {
        FieldType{
            FieldTypeKind::String,
            "std::string",
            "std::string",
        },
    };

    TypeModel type;
    type.name = "VectorValues";
    type.qualified_name = "VectorValues";
    type.fields = {vector_field};

    ProjectModel project;
    project.types = {type};
    return project;
}

} // namespace

int main() {
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
    assert(integer_result.header.find("DecodeErrorCode::integer_overflow") !=
           std::string::npos);

    const auto integer_expected =
        read_file("tests/golden/simdjson_integer.expected.cjm.hpp");
    if (integer_result.header != integer_expected) {
        std::cerr << "generated simdjson integer header:\n"
                  << integer_result.header;
    }
    assert(integer_result.header == integer_expected);

    const auto vector_result =
        cjm::generator::simdjson::generate_header(make_vector_project());
    assert(!vector_result.success);
    assert(vector_result.header.empty());
    assert(vector_result.error.find("tags") != std::string::npos);
    assert(vector_result.error.find("std::vector<std::string>") !=
           std::string::npos);
    return 0;
}
