#include "backends/simdjson/cpp_generator.hpp"

#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

namespace cjm::generator::simdjson {
namespace {

// Return whether an optional field has a complete generated decoder.
bool is_supported_optional_field(const metadata::FieldType& type) {
    return type.kind == metadata::FieldTypeKind::Optional &&
           type.arguments.size() == 1 &&
           type.arguments[0].kind == metadata::FieldTypeKind::SignedInteger &&
           type.arguments[0].spelling == "std::int64_t" &&
           type.arguments[0].qualified_name == "std::int64_t";
}

// Return whether a Metadata IR kind has a complete generated decoder.
bool is_supported_scalar_kind(metadata::FieldTypeKind kind) {
    switch (kind) {
    case metadata::FieldTypeKind::Bool:
    case metadata::FieldTypeKind::SignedInteger:
    case metadata::FieldTypeKind::UnsignedInteger:
    case metadata::FieldTypeKind::String:
        return true;
    case metadata::FieldTypeKind::FloatingPoint:
    case metadata::FieldTypeKind::Enum:
    case metadata::FieldTypeKind::Array:
    case metadata::FieldTypeKind::Vector:
    case metadata::FieldTypeKind::Map:
    case metadata::FieldTypeKind::UserDefined:
        return false;
    }
    return false;
}

// Return the globally qualified C++ name of one generated model.
std::string generated_type_name(const metadata::TypeModel& type) {
    const auto& name =
        type.qualified_name.empty() ? type.name : type.qualified_name;
    if (name.rfind("::", 0) == 0) {
        return name;
    }
    return "::" + name;
}

// Write one generated source line at the requested indentation level.
void write_line(std::ostringstream& out, std::size_t indent_level,
                const std::string& text) {
    for (std::size_t level = 0; level < indent_level; ++level) {
        out << "    ";
    }
    out << text << '\n';
}

// Generate the experimental decode error and structured path types.
void generate_decode_error_model(std::ostringstream& out) {
    out << "#ifndef CJM_SIMDJSON_RUNTIME_TYPES_DEFINED\n"
        << "#define CJM_SIMDJSON_RUNTIME_TYPES_DEFINED\n"
        << "\n"
        << "namespace cjm::simdjson {\n"
        << "\n"
        << "enum class DecodeErrorCode {\n"
        << "    none,\n"
        << "    syntax_error,\n"
        << "    trailing_content,\n"
        << "    expected_object,\n"
        << "    expected_bool,\n"
        << "    expected_string,\n"
        << "    expected_integer,\n"
        << "    expected_unsigned_integer,\n"
        << "    integer_overflow,\n"
        << "    missing_required_field\n"
        << "};\n"
        << "\n"
        << "enum class DecodePathSegmentKind {\n"
        << "    field,\n"
        << "    index\n"
        << "};\n"
        << "\n"
        << "struct DecodePathSegment {\n"
        << "    DecodePathSegmentKind kind = "
           "DecodePathSegmentKind::field;\n"
        << "    std::string field_name;\n"
        << "    std::size_t index = 0;\n"
        << "};\n"
        << "\n"
        << "struct DecodeError {\n"
        << "    DecodeErrorCode code = DecodeErrorCode::none;\n"
        << "    std::vector<DecodePathSegment> path;\n"
        << "    ::simdjson::error_code runtime_error = "
           "::simdjson::SUCCESS;\n"
        << "};\n"
        << "\n"
        << "template <typename T>\n"
        << "std::optional<T> from_json(\n"
        << "    std::string_view input,\n"
        << "    DecodeError& error);\n"
        << "\n"
        << "} // namespace cjm::simdjson\n"
        << "\n"
        << "#endif\n";
}

// Generate one structured field path append.
void generate_field_error_path(std::ostringstream& out,
                               const metadata::FieldModel& field,
                               std::size_t indent_level) {
    write_line(out, indent_level, "error.path.push_back(");
    write_line(out, indent_level + 1,
               "{DecodePathSegmentKind::field, \"" + field.json.name +
                   "\", 0});");
}

// Generate one required bool field decoder.
void generate_bool_field_decode(std::ostringstream& out,
                                const metadata::FieldModel& field) {
    write_line(out, 2, "if (key == \"" + field.json.name + "\") {");
    write_line(out, 3,
               "runtime_error = field.value().get_bool().get(value." +
                   field.name + ");");
    write_line(out, 3, "if (runtime_error) {");
    write_line(out, 4, "error.code = DecodeErrorCode::expected_bool;");
    generate_field_error_path(out, field, 4);
    write_line(out, 4, "error.runtime_error = runtime_error;");
    write_line(out, 4, "return false;");
    write_line(out, 3, "}");
    write_line(out, 3, "has_" + field.name + " = true;");
    write_line(out, 3, "continue;");
    write_line(out, 2, "}");
}

// Generate one required owned string field decoder.
void generate_string_field_decode(std::ostringstream& out,
                                  const metadata::FieldModel& field) {
    const std::string decoded_name = "decoded_" + field.name;

    write_line(out, 2, "if (key == \"" + field.json.name + "\") {");
    write_line(out, 3, "std::string_view " + decoded_name + ";");
    write_line(out, 3,
               "runtime_error = field.value().get_string().get(" +
                   decoded_name + ");");
    write_line(out, 3, "if (runtime_error) {");
    write_line(out, 4, "error.code = DecodeErrorCode::expected_string;");
    generate_field_error_path(out, field, 4);
    write_line(out, 4, "error.runtime_error = runtime_error;");
    write_line(out, 4, "return false;");
    write_line(out, 3, "}");
    write_line(out, 3,
               "value." + field.name + ".assign(" + decoded_name +
                   ".begin(), " + decoded_name + ".end());");
    write_line(out, 3, "has_" + field.name + " = true;");
    write_line(out, 3, "continue;");
    write_line(out, 2, "}");
}

// Generate one required signed or unsigned integer field decoder.
void generate_integer_field_decode(std::ostringstream& out,
                                   const metadata::FieldModel& field) {
    const bool is_signed =
        field.type.kind == metadata::FieldTypeKind::SignedInteger;
    const std::string decoded_type =
        is_signed ? "std::int64_t" : "std::uint64_t";
    const std::string getter = is_signed ? "get_int64" : "get_uint64";
    const std::string expected_error =
        is_signed ? "DecodeErrorCode::expected_integer"
                  : "DecodeErrorCode::expected_unsigned_integer";
    const std::string decoded_name = "decoded_" + field.name;

    write_line(out, 2, "if (key == \"" + field.json.name + "\") {");
    write_line(out, 3,
               "using target_type = decltype(value." + field.name + ");");
    write_line(out, 3, decoded_type + " " + decoded_name + " = 0;");
    write_line(out, 3,
               "runtime_error = field.value()." + getter + "().get(" +
                   decoded_name + ");");
    write_line(out, 3, "if (runtime_error) {");
    write_line(out, 4, "error.code = " + expected_error + ";");
    generate_field_error_path(out, field, 4);
    write_line(out, 4, "error.runtime_error = runtime_error;");
    write_line(out, 4, "return false;");
    write_line(out, 3, "}");
    out << "\n";

    if (is_signed) {
        write_line(out, 3,
                   "const auto target_min = static_cast<std::int64_t>(");
        write_line(out, 4, "(std::numeric_limits<target_type>::min)());");
    }

    write_line(out, 3,
               "const auto target_max = static_cast<" + decoded_type + ">(");
    write_line(out, 4, "(std::numeric_limits<target_type>::max)());");

    std::string overflow_condition = decoded_name + " > target_max";
    if (is_signed) {
        overflow_condition =
            decoded_name + " < target_min || " + overflow_condition;
    }

    write_line(out, 3, "if (" + overflow_condition + ") {");
    write_line(out, 4, "error.code = DecodeErrorCode::integer_overflow;");
    generate_field_error_path(out, field, 4);
    write_line(out, 4, "return false;");
    write_line(out, 3, "}");
    out << "\n";

    write_line(out, 3,
               "value." + field.name + " = static_cast<target_type>(" +
                   decoded_name + ");");
    write_line(out, 3, "has_" + field.name + " = true;");
    write_line(out, 3, "continue;");
    write_line(out, 2, "}");
}

// Generate one supported scalar field decoder.
void generate_field_decode(std::ostringstream& out,
                           const metadata::FieldModel& field) {
    switch (field.type.kind) {
    case metadata::FieldTypeKind::Bool:
        generate_bool_field_decode(out, field);
        return;
    case metadata::FieldTypeKind::SignedInteger:
    case metadata::FieldTypeKind::UnsignedInteger:
        generate_integer_field_decode(out, field);
        return;
    case metadata::FieldTypeKind::FloatingPoint:
    case metadata::FieldTypeKind::String:
        generate_string_field_decode(out, field);
        return;
    case metadata::FieldTypeKind::Enum:
    case metadata::FieldTypeKind::Array:
    case metadata::FieldTypeKind::Vector:
    case metadata::FieldTypeKind::Map:
    case metadata::FieldTypeKind::Optional:
    case metadata::FieldTypeKind::UserDefined:
        return;
    }
}

// Generate the internal object decoder for one model.
void generate_object_decode_function(std::ostringstream& out,
                                     const metadata::TypeModel& type) {
    const auto cpp_type = generated_type_name(type);
    // 1. Open namespace cjm::simdjson::detail.
    write_line(out, 0, "namespace cjm::simdjson::detail {");
    write_line(out, 0, "");
    write_line(out, 0, "inline bool decode_object(");
    write_line(out, 1, "::simdjson::ondemand::object& object,");
    write_line(out, 1, cpp_type + "& value,");
    write_line(out, 1, "DecodeError& error) {");

    write_line(out, 1,
               "::simdjson::error_code runtime_error = ::simdjson::SUCCESS;");
    write_line(out, 0, "");

    write_line(out, 1, "// 1. Track required fields for this object.");

    for (const auto& field : type.fields) {
        if (!field.json.ignored) {
            out << "    bool has_" << field.name << " = false;\n";
        }
    }

    out << "\n"
        << "    // 2. Visit each JSON field once.\n"
        << "    for (auto field : object) {\n"
        << "        std::string_view key;\n"
        << "        runtime_error = field.unescaped_key().get(key);\n"
        << "        if (runtime_error) {\n"
        << "            error.code = DecodeErrorCode::syntax_error;\n"
        << "            error.runtime_error = runtime_error;\n"
        << "            return false;\n"
        << "        }\n"
        << "\n";

    for (const auto& field : type.fields) {
        if (field.json.ignored) {
            continue;
        }
        generate_field_decode(out, field);
    }

    out << "    }\n"
        << "\n"
        << "    // 3. Verify that every required field was present.\n";

    for (const auto& field : type.fields) {
        if (field.json.ignored) {
            continue;
        }

        out << "    if (!has_" << field.name << ") {\n"
            << "        error.code = "
            << "DecodeErrorCode::missing_required_field;\n"
            << "        error.path.push_back(\n"
            << "            {DecodePathSegmentKind::field, \""
            << field.json.name << "\", 0});\n"
            << "        return false;\n"
            << "    }\n";
    }

    out << "\n"
        << "    // 4. Report that this object was decoded successfully.\n"
        << "    return true;\n"
        << "}\n"
        << "\n"
        << "} // namespace cjm::simdjson::detail\n";
}

// Generate the public root decoder for one model.
void generate_root_decode_function(std::ostringstream& out,
                                   const metadata::TypeModel& type) {
    const auto cpp_type = generated_type_name(type);

    out << "namespace cjm::simdjson {\n"
        << "\n"
        << "template <>\n"
        << "inline std::optional<" << cpp_type << ">\n"
        << "from_json<" << cpp_type << ">(\n"
        << "    std::string_view input,\n"
        << "    DecodeError& error) {\n"
        << "    // 1. Prepare the padded input owned for this decode.\n"
        << "    error = {};\n"
        << "    ::simdjson::padded_string padded_input(input);\n"
        << "    ::simdjson::ondemand::parser parser;\n"
        << "\n"
        << "    // 2. Start one On-Demand document and read its root object.\n"
        << "    ::simdjson::ondemand::document document;\n"
        << "    auto runtime_error = "
           "parser.iterate(padded_input).get(document);\n"
        << "    if (runtime_error) {\n"
        << "        error.code = DecodeErrorCode::syntax_error;\n"
        << "        error.runtime_error = runtime_error;\n"
        << "        return std::nullopt;\n"
        << "    }\n"
        << "\n"
        << "    ::simdjson::ondemand::object object;\n"
        << "    runtime_error = document.get_object().get(object);\n"
        << "    if (runtime_error) {\n"
        << "        error.code = runtime_error == ::simdjson::INCORRECT_TYPE\n"
        << "                         ? DecodeErrorCode::expected_object\n"
        << "                         : DecodeErrorCode::syntax_error;\n"
        << "        error.runtime_error = runtime_error;\n"
        << "        return std::nullopt;\n"
        << "    }\n"

        << "    // 3. Decode the root object into a new value.\n"
        << "    " << cpp_type << " value{};\n"
        << "    if (!detail::decode_object(object, value, error)) {\n"
        << "        return std::nullopt;\n"
        << "    }\n"
        << "\n"
        << "    // 4. Reject non-whitespace content after the root object.\n"
        << "    if (!document.at_end()) {\n"
        << "        error.code = DecodeErrorCode::trailing_content;\n"
        << "        error.runtime_error = ::simdjson::TRAILING_CONTENT;\n"
        << "        return std::nullopt;\n"
        << "    }\n"
        << "\n"
        << "    // 5. Return the completely decoded object.\n"
        << "    return value;\n"
        << "}\n"
        << "\n"
        << "} // namespace cjm::simdjson\n";
}

// Return the first unsupported-field diagnostic, or an empty string.
std::string validate_project(const metadata::ProjectModel& project) {
    for (const auto& type : project.types) {
        for (const auto& field : type.fields) {
            if (field.json.ignored) {
                continue;
            }
            if (is_supported_scalar_kind(field.type.kind) ||
                is_supported_optional_field(field.type)) {
                continue;
            }

            const auto& type_name = field.type.spelling.empty()
                                        ? field.type.qualified_name
                                        : field.type.spelling;

            return "simdjson backend does not support field '" + field.name +
                   "' of type " + type_name;
        }
    }
    return {};
}

} // namespace

GenerationResult generate_header(const metadata::ProjectModel& project) {
    const auto error = validate_project(project);
    if (!error.empty()) {
        return GenerationResult{false, {}, error};
    }

    std::ostringstream header;
    header << "// This file was generated by CJM.\n"
           << "// Do not edit this file manually.\n"
           << "\n"
           << "#pragma once\n"
           << "\n"
           << "#include <simdjson.h>\n"
           << "\n"
           << "#include <cstddef>\n"
           << "#include <cstdint>\n"
           << "#include <limits>\n"
           << "#include <optional>\n"
           << "#include <string>\n"
           << "#include <string_view>\n"
           << "#include <vector>\n"
           << "\n";

    generate_decode_error_model(header);

    for (const auto& type : project.types) {
        header << "\n";
        generate_object_decode_function(header, type);
        header << "\n";
        generate_root_decode_function(header, type);
    }
    return GenerationResult{true, header.str(), {}};
}

} // namespace cjm::generator::simdjson
