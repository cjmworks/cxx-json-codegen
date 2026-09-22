#include "backends/simdjson/encoder.h"

#include <sstream>
#include <stdexcept>
#include <string>

namespace cjm::generator::simdjson::detail {

// Generate the experimental encode error and public API declarations
void generate_encode_error_model(std::ostringstream& out) {
    out << "#ifndef CJM_SIMDJSON_ENCODE_RUNTIME_TYPES_DEFINED\n"
        << "#define CJM_SIMDJSON_ENCODE_RUNTIME_TYPES_DEFINED\n"
        << "\n"
        << "namespace cjm::simdjson {\n"
        << "\n"
        << "using EncodePathSegmentKind = DecodePathSegmentKind;\n"
        << "using EncodePathSegment = DecodePathSegment;\n"
        << "\n"
        << "enum class EncodeErrorCode {\n"
        << "    none,\n"
        << "    invalid_utf8_string,\n"
        << "    invalid_utf8_key,\n"
        << "    non_finite_number,\n"
        << "    invalid_enum_value,\n"
        << "    output_failure,\n"
        << "    allocation_failure,\n"
        << "    size_limit_exceeded\n"
        << "};\n"
        << "\n"
        << "struct EncodeError {\n"
        << "    EncodeErrorCode code = EncodeErrorCode::none;\n"
        << "    std::vector<EncodePathSegment> path;\n"
        << "    ::simdjson::error_code runtime_error = ::simdjson::SUCCESS;\n"
        << "};\n"
        << "\n"
        << "template <typename T>\n"
        << "std::optional<std::string> to_json(\n"
        << "    const T& value,\n"
        << "    EncodeError& error);\n"
        << "\n"
        << "} // namespace cjm::simdjson\n"
        << "\n"
        << "#endif\n";
}

// Generate value encoding and error reporting for one enum field.
void generate_enum_field_encode(std::ostringstream& out,
                                const metadata::FieldModel& field,
                                const metadata::EnumModel& enum_model,
                                std::string_view value_expression,
                                std::size_t indent_level) {
    const auto& name = enum_model.qualified_name.empty()
                           ? enum_model.name
                           : enum_model.qualified_name;
    const auto cpp_type = name.rfind("::", 0) == 0 ? name : "::" + name;
    const std::string indent(indent_level * 4, ' ');

    // 1. Generate a string write for each known enumerator.
    bool first = true;
    for (const auto& enumerator : enum_model.enumerators) {
        out << indent << (first ? "if (" : "else if (") << value_expression
            << " == " << cpp_type + "::" << enumerator << ") {\n"
            << indent
            << "    builder.escape_and_append_with_quotes(\"" + enumerator +
                   "\");\n"
            << indent << "}\n";
        first = false;
    }

    // 2. Generate the unmapped-value error.
    out << indent << (first ? "{\n" : "else {\n") << indent
        << "    error.code = EncodeErrorCode::invalid_enum_value;\n"
        << indent
        << "    error.path = {{EncodePathSegmentKind::field, \"" +
               field.json.name + "\", 0}};\n"
        << indent << "    error.runtime_error = ::simdjson::SUCCESS;\n"
        << indent << "    return false;\n"
        << indent << "}\n";
}

// Generate validation and encoding for one value expression.
void generate_value_encode(std::ostringstream& out,
                           const metadata::FieldModel& field,
                           const metadata::FieldType& value_type,
                           std::string_view value_expression,
                           const std::vector<metadata::EnumModel>& enums,
                           std::size_t indent_level) {

    const std::string indent(indent_level * 4, ' ');

    switch (value_type.kind) {
    case metadata::FieldTypeKind::Bool:
    case metadata::FieldTypeKind::SignedInteger:
    case metadata::FieldTypeKind::UnsignedInteger:
        out << indent << "builder.append(" << value_expression << ");\n";
        return;
    case metadata::FieldTypeKind::FloatingPoint:
        out << indent << "if (!std::isfinite(" << value_expression << ")) {\n"
            << indent
            << "    error.code = EncodeErrorCode::non_finite_number;\n"
            << indent << "    error.path = {{EncodePathSegmentKind::field, \""
            << field.json.name << "\", 0}};\n"
            << indent << "    error.runtime_error = ::simdjson::SUCCESS;\n"
            << indent << "    return false;\n"
            << indent << "}\n"
            << indent << "builder.append(" << value_expression << ");\n";
        return;
    case metadata::FieldTypeKind::String:
        out << indent << "if (!::simdjson::validate_utf8(" << value_expression
            << ")) {\n"
            << indent << "    "
            << "error.code = EncodeErrorCode::invalid_utf8_string;\n"
            << indent << "    "
            << "error.path = {{EncodePathSegmentKind::field, \"" +
                   field.json.name + "\", 0}};\n"
            << indent << "    "
            << "error.runtime_error = ::simdjson::UTF8_ERROR;\n"
            << indent << "    " << "return false;\n"
            << indent << "}\n";
        out << indent << "builder.escape_and_append_with_quotes("
            << value_expression << ");\n";
        return;
    case metadata::FieldTypeKind::Enum:
        for (const auto& enum_model : enums) {
            if (enum_model.qualified_name == value_type.qualified_name) {
                generate_enum_field_encode(out, field, enum_model,
                                           value_expression, indent_level);
                return;
            }
        }
        throw std::logic_error("generate_value_encode: missing enum model");
    case metadata::FieldTypeKind::Optional: {
        const auto expression = "(" + std::string(value_expression) + ")";
        out << indent << "if (" + expression + ".has_value()) {\n";

        generate_value_encode(out, field, value_type.arguments.at(0),
                              "*" + expression, enums, indent_level + 1);

        out << indent << "} else {\n"
            << indent << "    builder.append_null();\n"
            << indent << "}\n";
        return;
    }
    case metadata::FieldTypeKind::UserDefined:
        out << indent << "if (!encode_object(builder, " << value_expression
            << ", error)) {\n"
            << indent << "    error.path.insert(error.path.begin(),\n"
            << indent << "        EncodePathSegment{"
            << "EncodePathSegmentKind::field, \"" << field.json.name
            << "\", 0});\n"
            << indent << "    return false;\n"
            << indent << "}\n";
        return;
    default:
        throw std::logic_error("generate_value_encode: unimplemented type");
    }
}

// Generate an object encoder for supported scalar fields.
void generate_scalar_object_encode_function(
    std::ostringstream& out, const metadata::TypeModel& type,
    const std::vector<metadata::EnumModel>& enums) {
    const auto& name =
        type.qualified_name.empty() ? type.name : type.qualified_name;
    const auto cpp_type = name.rfind("::", 0) == 0 ? name : "::" + name;

    // 1. Generate the object encoder signature and opening brace.
    out << "namespace cjm::simdjson::detail {\n"
        << "\n"
        << "inline bool encode_object(\n"
        << "    ::simdjson::builder::string_builder& builder,\n"
        << "    const " + cpp_type + "& value,\n"
        << "    EncodeError& error) {\n"
        << "    builder.start_object();\n"
        << "    bool first_field = true;\n";

    // 2. Gnerate writes for participating scalar fields.
    for (const auto& field : type.fields) {
        if (field.json.ignored) {
            continue;
        }

        const bool omit_disengaged =
            field.type.kind == metadata::FieldTypeKind::Optional &&
            field.json.omit_empty;
        if (omit_disengaged) {
            out << "    if (value." + field.name + ".has_value()) {\n";
        }

        if (field.type.kind == metadata::FieldTypeKind::FloatingPoint) {
            out << "    if (!std::isfinite(value." + field.name + ")) {\n"
                << "        error.code = EncodeErrorCode::non_finite_number;\n"
                << "        error.path = {{EncodePathSegmentKind::field, \"" +
                       field.json.name + "\", 0}};\n"
                << "        error.runtime_error = ::simdjson::SUCCESS;\n"
                << "        return false;\n"
                << "    }\n";
        }
        if (field.type.kind == metadata::FieldTypeKind::String) {
            out << "    if (!::simdjson::validate_utf8(value." + field.name +
                       ")) {\n"
                << "        error.code = "
                   "EncodeErrorCode::invalid_utf8_string;\n"
                << "        error.path = {{EncodePathSegmentKind::field, \"" +
                       field.json.name + "\", 0}};\n"
                << "        error.runtime_error = ::simdjson::UTF8_ERROR;\n"
                << "        return false;\n"
                << "    }\n";
        }

        out << "    if (!first_field) {\n"
            << "        builder.append_comma();\n"
            << "    }\n"
            << "    first_field = false;\n";

        out << "    builder.escape_and_append_with_quotes(\"" +
                   field.json.name + "\");\n"
            << "    builder.append_colon();\n";

        if (field.type.kind == metadata::FieldTypeKind::Optional) {
            generate_value_encode(out, field, field.type, "value." + field.name,
                                  enums, omit_disengaged ? 2 : 1);
        } else if (field.type.kind == metadata::FieldTypeKind::String) {
            out << "    builder.escape_and_append_with_quotes(value." +
                       field.name + ");\n";
        } else if (field.type.kind == metadata::FieldTypeKind::Enum) {
            for (const auto& enum_model : enums) {
                if (enum_model.qualified_name == field.type.qualified_name) {
                    generate_enum_field_encode(out, field, enum_model,
                                               "value." + field.name, 1);
                    break;
                }
            }
        } else {
            out << "    builder.append(value." + field.name + ");\n";
        }
        if (omit_disengaged) {
            out << "    }\n";
        }
    }
    // 3. Generate the object closing brace and return.
    out << "    builder.end_object();\n"
        << "    return true;\n"
        << "}\n"
        << "\n"
        << "} // namespace cjm::simdjson::detail\n";
}

// Generate the public root encoder for one model.
void generate_root_encode_function(std::ostringstream& out,
                                   const metadata::TypeModel& type) {
    const auto& name =
        type.qualified_name.empty() ? type.name : type.qualified_name;
    const auto cpp_type = name.rfind("::", 0) == 0 ? name : "::" + name;

    // 1. Genereate the root signature and reset the previous error.
    out << "namespace cjm::simdjson {\n"
        << "\n"
        << "template <>\n"
        << "inline std::optional<std::string>\n"
        << "to_json<" + cpp_type << ">(\n"
        << "    const " + cpp_type + "& value,\n"
        << "    EncodeError& error) {\n"
        << "    error = {};\n"
        << "    try {\n"
        << "        ::simdjson::builder::string_builder builder;\n"
        << "        const bool model_valid =\n"
        << "            detail::encode_object(builder, value, "
           "error);\n";

    // 2. Generate buider-result checking and the owned output.
    out << "        std::string_view view;\n"
        << "        const auto runtime_error = "
           "builder.view().get(view);\n"
        << "        if (runtime_error != ::simdjson::SUCCESS) {\n"
        << "            error.path.clear();\n"
        << "            error.code = EncodeErrorCode::output_failure;\n"
        << "            error.runtime_error = runtime_error;\n"
        << "            return std::nullopt;\n"
        << "        }\n"
        << "        if (!model_valid) {\n"
        << "            return std::nullopt;\n"
        << "        }\n"
        << "        return std::string(view);\n";

    // 3. Gnerate resource-error handling and close the function.
    out << "    } catch (const std::bad_alloc&) {\n"
        << "        error.path.clear();\n"
        << "        error.code = EncodeErrorCode::allocation_failure;\n"
        << "        error.runtime_error = ::simdjson::SUCCESS;\n"
        << "        return std::nullopt;\n"
        << "    } catch (const std::length_error&) {\n"
        << "        error.path.clear();\n"
        << "        error.code = "
           "EncodeErrorCode::size_limit_exceeded;\n"
        << "        error.runtime_error = ::simdjson::SUCCESS;\n"
        << "        return std::nullopt;\n"
        << "    }\n"
        << "}\n"
        << "\n"
        << "} // namespace cjm::simdjson\n";
}

} // namespace cjm::generator::simdjson::detail
