#include "backends/simdjson/encoder.h"

#include <sstream>

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

// Generate an object encoder for supported scalar fields.
void generate_scalar_object_encode_function(std::ostringstream& out,
                                            const metadata::TypeModel& type) {
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
        << "    builder.start_object();\n";

    // 2. Gnerate writes for participating scalar fields.
    bool first_field = true;
    for (const auto& field : type.fields) {
        if (field.json.ignored) {
            continue;
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
        if (!first_field) {
            out << "    builder.append_comma();\n";
        }
        first_field = false;

        out << "    builder.escape_and_append_with_quotes(\"" +
                   field.json.name + "\");\n"
            << "    builder.append_colon();\n";

        if (field.type.kind == metadata::FieldTypeKind::String) {
            out << "    builder.escape_and_append_with_quotes(value." +
                       field.name + ");\n";
        } else {
            out << "    builder.append(value." + field.name + ");\n";
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
        << "            detail::encode_object(builder, value, error);\n";

    // 2. Generate buider-result checking and the owned output.
    out << "        std::string_view view;\n"
        << "        const auto runtime_error = builder.view().get(view);\n"
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
        << "        error.code = EncodeErrorCode::size_limit_exceeded;\n"
        << "        error.runtime_error = ::simdjson::SUCCESS;\n"
        << "        return std::nullopt;\n"
        << "    }\n"
        << "}\n"
        << "\n"
        << "} // namespace cjm::simdjson\n";
}

} // namespace cjm::generator::simdjson::detail
