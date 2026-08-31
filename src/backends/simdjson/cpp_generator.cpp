#include "backends/simdjson/cpp_generator.hpp"

#include <cstddef>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace cjm::generator::simdjson {
namespace {

struct UnsupportedCapability {
    std::string model_name;
    std::string field_name;
    std::string json_name;
    std::string type_name;
    std::string reason;
};

// Return the display name for a Metadata IR type.
std::string metadata_type_name(const metadata::FieldType& type) {
    if (!type.spelling.empty()) {
        return type.spelling;
    }
    if (!type.qualified_name.empty()) {
        return type.qualified_name;
    }
    return "<unknown>";
}

// Return the display name for one Metadata IR model.
std::string generated_model_name(const metadata::TypeModel& type) {
    if (!type.qualified_name.empty()) {
        return type.qualified_name;
    }
    return type.name;
}

// Format one unsupported backend capability diagnostic.
std::string
format_unsupported_capability(const UnsupportedCapability& unsupported) {
    std::ostringstream error;
    error << "simdjson backend unsupported capability: model '"
          << unsupported.model_name << "', field '" << unsupported.field_name
          << "', json field '" << unsupported.json_name << "', C++ type '"
          << unsupported.type_name << "': " << unsupported.reason
          << ". Supported simdjson decode shapes currently include bool, "
          << "string, signed integers, unsigned integers, optional scalar "
             "fields, enum strings, and generated nested models.";
    return error.str();
}

// Find the Metadata IR enum model for one enum field type.
const metadata::EnumModel*
find_enum_model(const std::vector<metadata::EnumModel>& enums,
                const metadata::FieldType& type) {
    if (type.kind != metadata::FieldTypeKind::Enum) {
        return nullptr;
    }

    for (const auto& enum_model : enums) {
        if (enum_model.qualified_name == type.qualified_name) {
            return &enum_model;
        }
    }
    return nullptr;
}

// Return whether an optional field has a complete generated decoder.
bool is_supported_optional_field(
    const metadata::FieldType& type,
    const std::vector<metadata::EnumModel>& enums) {
    if (type.kind != metadata::FieldTypeKind::Optional ||
        type.arguments.size() != 1) {
        return false;
    }

    switch (type.arguments[0].kind) {
    case metadata::FieldTypeKind::Bool:
    case metadata::FieldTypeKind::SignedInteger:
    case metadata::FieldTypeKind::UnsignedInteger:
    case metadata::FieldTypeKind::String:
        return true;
    case metadata::FieldTypeKind::Enum:
        return find_enum_model(enums, type.arguments[0]) != nullptr;
    case metadata::FieldTypeKind::FloatingPoint:
    case metadata::FieldTypeKind::Array:
    case metadata::FieldTypeKind::Vector:
    case metadata::FieldTypeKind::Map:
    case metadata::FieldTypeKind::Optional:
    case metadata::FieldTypeKind::UserDefined:
        return false;
    }
    return false;
}

// Return whether a user-defined field has a complete generated decoder.
bool is_supported_user_defined_field(const metadata::FieldType& type) {
    return type.kind == metadata::FieldTypeKind::UserDefined &&
           (!type.qualified_name.empty() || !type.spelling.empty());
}

// Return whether a Metadata IR kind has a complete generated decoder.
bool is_supported_scalar_kind(metadata::FieldTypeKind kind) {
    switch (kind) {
    case metadata::FieldTypeKind::Bool:
    case metadata::FieldTypeKind::SignedInteger:
    case metadata::FieldTypeKind::UnsignedInteger:
    case metadata::FieldTypeKind::String:
    case metadata::FieldTypeKind::Enum:
        return true;
    case metadata::FieldTypeKind::FloatingPoint:
    case metadata::FieldTypeKind::Array:
    case metadata::FieldTypeKind::Vector:
    case metadata::FieldTypeKind::Map:
    case metadata::FieldTypeKind::Optional:
    case metadata::FieldTypeKind::UserDefined:
        return false;
    }
    return false;
}

// Return whether a field type has a complete scalar decoder.
bool is_supported_scalar_type(const metadata::FieldType& type,
                              const std::vector<metadata::EnumModel>& enums) {
    if (type.kind == metadata::FieldTypeKind::Enum) {
        return find_enum_model(enums, type) != nullptr;
    }
    return is_supported_scalar_kind(type.kind);
}

// Return whether a vector field has a complete scalar element decoder.
bool is_supported_vector_field(const metadata::FieldType& type,
                               const std::vector<metadata::EnumModel>& enums) {
    return type.kind == metadata::FieldTypeKind::Vector &&
           type.arguments.size() == 1 &&
           is_supported_scalar_type(type.arguments[0], enums);
}

// Return the backend capability failure for one field, when supported.
std::optional<UnsupportedCapability> unsupported_capability_for_field(
    const metadata::TypeModel& parent_type, const metadata::FieldModel& field,
    const std::vector<metadata::EnumModel>& enums) {
    if (field.json.ignored) {
        return std::nullopt;
    }

    if (is_supported_scalar_type(field.type, enums) ||
        is_supported_optional_field(field.type, enums) ||
        is_supported_vector_field(field.type, enums) ||
        is_supported_user_defined_field(field.type)) {
        return std::nullopt;
    }

    std::string reason;
    switch (field.type.kind) {
    case metadata::FieldTypeKind::FloatingPoint:
        reason = "floating-point decode is not implemented";
        break;
    case metadata::FieldTypeKind::Enum:
        reason = "enum string decode requires a resolved enum model";
        break;
    case metadata::FieldTypeKind::Array:
        reason = "fixed array decode is not implemented";
        break;
    case metadata::FieldTypeKind::Vector:
        reason = "vector decode is not implemented";
        break;
    case metadata::FieldTypeKind::Map:
        reason = "map decode is not implemented";
        break;
    case metadata::FieldTypeKind::Optional:
        reason = "optional decode is only implemented for scalar inner values";
        break;
    case metadata::FieldTypeKind::UserDefined:
        reason = "nested model decode requires a resolved user-defined type";
        break;
    case metadata::FieldTypeKind::Bool:
    case metadata::FieldTypeKind::SignedInteger:
    case metadata::FieldTypeKind::UnsignedInteger:
    case metadata::FieldTypeKind::String:
        return std::nullopt;
    }

    return UnsupportedCapability{
        generated_model_name(parent_type), field.name, field.json.name,
        metadata_type_name(field.type),    reason,
    };
}

// Return whether a field must be present in the decoded object.
bool requires_presence_check(const metadata::FieldModel& field) {
    return !field.json.ignored &&
           field.type.kind != metadata::FieldTypeKind::Optional;
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

// Return the globally qualified C++ name of one enum field type.
std::string generated_enum_type_name(const metadata::FieldType& type) {
    const auto& name =
        type.qualified_name.empty() ? type.spelling : type.qualified_name;
    if (name.rfind("::", 0) == 0) {
        return name;
    }
    return "::" + name;
}

// Return the generated C++ type spelling for an optional inner value.
std::string optional_inner_type_name(const metadata::FieldType& inner_type) {
    switch (inner_type.kind) {
    case metadata::FieldTypeKind::Bool:
        return "bool";
    case metadata::FieldTypeKind::String:
        return "std::string";
    case metadata::FieldTypeKind::Enum:
        return generated_enum_type_name(inner_type);
    case metadata::FieldTypeKind::SignedInteger:
    case metadata::FieldTypeKind::UnsignedInteger:
        return metadata_type_name(inner_type);
    case metadata::FieldTypeKind::FloatingPoint:
    case metadata::FieldTypeKind::Array:
    case metadata::FieldTypeKind::Vector:
    case metadata::FieldTypeKind::Map:
    case metadata::FieldTypeKind::Optional:
    case metadata::FieldTypeKind::UserDefined:
        return metadata_type_name(inner_type);
    }
    return metadata_type_name(inner_type);
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
        << "    expected_array,\n"
        << "    expected_bool,\n"
        << "    expected_string,\n"
        << "    expected_integer,\n"
        << "    expected_unsigned_integer,\n"
        << "    invalid_enum_string,\n"
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

void generate_index_error_path(std::ostringstream& out,
                               const std::string& index_expression,
                               std::size_t indent_level) {
    write_line(out, indent_level, "error.path.push_back(");
    write_line(out, indent_level + 1,
               "{DecodePathSegmentKind::index, \"\", " + index_expression +
                   "});");
}

void generate_value_error_path(
    std::ostringstream& out, const metadata::FieldModel& field,
    std::size_t indent_level,
    const std::optional<std::string>& index_expression) {
    generate_field_error_path(out, field, indent_level);
    if (index_expression.has_value()) {
        generate_index_error_path(out, *index_expression, indent_level);
    }
}

// Generate one structured field path prepend.
void generate_prepend_field_error_path(std::ostringstream& out,
                                       const metadata::FieldModel& field,
                                       std::size_t indent_level) {
    write_line(out, indent_level, "error.path.insert(");
    write_line(out, indent_level + 1, "error.path.begin(),");
    write_line(out, indent_level + 1,
               "{DecodePathSegmentKind::field, \"" + field.json.name +
                   "\", 0});");
}

// Generate one bool value decoder.
void generate_bool_value_decode(
    std::ostringstream& out, const metadata::FieldModel& field,
    const std::string& simdjson_value_expression,
    const std::string& target_expression, std::size_t indent_level,
    const std::optional<std::string>& index_expression) {
    write_line(out, indent_level,
               "runtime_error = " + simdjson_value_expression +
                   ".get_bool().get(" + target_expression + ");");

    write_line(out, indent_level, "if (runtime_error) {");
    write_line(out, indent_level + 1,
               "error.code = DecodeErrorCode::expected_bool;");
    generate_value_error_path(out, field, indent_level + 1, index_expression);
    write_line(out, indent_level + 1, "error.runtime_error = runtime_error;");
    write_line(out, indent_level + 1, "return false;");
    write_line(out, indent_level, "}");
}

// Generate one owned string value decoder.
void generate_string_value_decode(
    std::ostringstream& out, const metadata::FieldModel& field,
    const std::string& simdjson_value_expression,
    const std::string& target_expression, std::size_t indent_level,
    const std::optional<std::string>& index_expression) {
    const std::string decoded_name = "decoded_" + field.name + "_view";

    write_line(out, indent_level, "std::string_view " + decoded_name + ";");
    write_line(out, indent_level,
               "runtime_error = " + simdjson_value_expression +
                   ".get_string().get(" + decoded_name + ");");
    write_line(out, indent_level, "if (runtime_error) {");
    write_line(out, indent_level + 1,
               "error.code = DecodeErrorCode::expected_string;");
    generate_value_error_path(out, field, indent_level + 1, index_expression);
    write_line(out, indent_level + 1, "error.runtime_error = runtime_error;");
    write_line(out, indent_level + 1, "return false;");
    write_line(out, indent_level, "}");
    write_line(out, indent_level,
               target_expression + ".assign(" + decoded_name + ".begin(), " +
                   decoded_name + ".end());");
}

// Generate one signed or unsigned integer value decoder.
void generate_integer_value_decode(
    std::ostringstream& out, const metadata::FieldModel& field,
    const metadata::FieldType& type,
    const std::string& simdjson_value_expression,
    const std::string& target_expression, std::size_t indent_level,
    const std::optional<std::string>& index_expression) {
    const bool is_signed = type.kind == metadata::FieldTypeKind::SignedInteger;
    const std::string decoded_type =
        is_signed ? "std::int64_t" : "std::uint64_t";
    const std::string getter = is_signed ? "get_int64" : "get_uint64";
    const std::string expected_error =
        is_signed ? "DecodeErrorCode::expected_integer"
                  : "DecodeErrorCode::expected_unsigned_integer";
    const std::string decoded_name = "decoded_" + field.name;

    write_line(out, indent_level,
               "using target_type = decltype(" + target_expression + ");");
    write_line(out, indent_level, decoded_type + " " + decoded_name + " = 0;");
    write_line(out, indent_level,
               "runtime_error = " + simdjson_value_expression + "." + getter +
                   "().get(" + decoded_name + ");");
    write_line(out, indent_level, "if (runtime_error) {");
    write_line(out, indent_level + 1, "error.code = " + expected_error + ";");
    generate_value_error_path(out, field, indent_level + 1, index_expression);
    write_line(out, indent_level + 1, "error.runtime_error = runtime_error;");
    write_line(out, indent_level + 1, "return false;");
    write_line(out, indent_level, "}");
    out << "\n";

    if (is_signed) {
        write_line(out, indent_level,
                   "const auto target_min = static_cast<std::int64_t>(");
        write_line(out, indent_level + 1,
                   "(std::numeric_limits<target_type>::min)());");
    }

    write_line(out, indent_level,
               "const auto target_max = static_cast<" + decoded_type + ">(");
    write_line(out, indent_level + 1,
               "(std::numeric_limits<target_type>::max)());");

    std::string overflow_condition = decoded_name + " > target_max";
    if (is_signed) {
        overflow_condition =
            decoded_name + " < target_min || " + overflow_condition;
    }

    write_line(out, indent_level, "if (" + overflow_condition + ") {");
    write_line(out, indent_level + 1,
               "error.code = DecodeErrorCode::integer_overflow;");
    generate_value_error_path(out, field, indent_level + 1, index_expression);
    write_line(out, indent_level + 1, "return false;");
    write_line(out, indent_level, "}");
    out << "\n";

    write_line(out, indent_level,
               target_expression + " = static_cast<target_type>(" +
                   decoded_name + ");");
}

// Generate one enum string value decoder.
void generate_enum_value_decode(
    std::ostringstream& out, const metadata::FieldModel& field,
    const metadata::FieldType& type, const metadata::EnumModel& enum_model,
    const std::string& simdjson_value_expression,
    const std::string& target_expression, std::size_t indent_level,
    const std::optional<std::string>& index_expression) {
    const std::string decoded_name = "decoded_" + field.name + "_view";
    const std::string matched_name = "decoded_" + field.name + "_matches";
    const std::string enum_type_name = generated_enum_type_name(type);

    write_line(out, indent_level, "std::string_view " + decoded_name + ";");
    write_line(out, indent_level,
               "runtime_error = " + simdjson_value_expression +
                   ".get_string().get(" + decoded_name + ");");
    write_line(out, indent_level, "if (runtime_error) {");
    write_line(out, indent_level + 1,
               "error.code = DecodeErrorCode::expected_string;");
    generate_value_error_path(out, field, indent_level + 1, index_expression);
    write_line(out, indent_level + 1, "error.runtime_error = runtime_error;");
    write_line(out, indent_level + 1, "return false;");
    write_line(out, indent_level, "}");

    write_line(out, indent_level, "bool " + matched_name + " = false;");
    for (const auto& enumerator : enum_model.enumerators) {
        write_line(out, indent_level,
                   "if (" + decoded_name + " == \"" + enumerator + "\") {");
        write_line(out, indent_level + 1,
                   target_expression + " = " + enum_type_name +
                       "::" + enumerator + ";");
        write_line(out, indent_level + 1, matched_name + " = true;");
        write_line(out, indent_level, "}");
    }
    write_line(out, indent_level, "if (!" + matched_name + ") {");
    write_line(out, indent_level + 1,
               "error.code = DecodeErrorCode::invalid_enum_string;");
    generate_value_error_path(out, field, indent_level + 1, index_expression);
    write_line(out, indent_level + 1, "return false;");
    write_line(out, indent_level, "}");
}

// Generate one supported scalar value decoder.
void generate_scalar_value_decode(
    std::ostringstream& out, const metadata::FieldModel& field,
    const metadata::FieldType& type,
    const std::vector<metadata::EnumModel>& enums,
    const std::string& simdjson_value_expression,
    const std::string& target_expression, std::size_t indent_level,
    const std::optional<std::string>& index_expression) {
    switch (type.kind) {
    case metadata::FieldTypeKind::Bool:
        generate_bool_value_decode(out, field, simdjson_value_expression,
                                   target_expression, indent_level,
                                   index_expression);
        return;
    case metadata::FieldTypeKind::String:
        generate_string_value_decode(out, field, simdjson_value_expression,
                                     target_expression, indent_level,
                                     index_expression);
        return;
    case metadata::FieldTypeKind::SignedInteger:
    case metadata::FieldTypeKind::UnsignedInteger:
        generate_integer_value_decode(
            out, field, type, simdjson_value_expression, target_expression,
            indent_level, index_expression);
        return;
    case metadata::FieldTypeKind::Enum: {
        const auto* enum_model = find_enum_model(enums, type);
        if (enum_model == nullptr) {
            return;
        }
        generate_enum_value_decode(out, field, type, *enum_model,
                                   simdjson_value_expression, target_expression,
                                   indent_level, index_expression);
        return;
    }
    case metadata::FieldTypeKind::FloatingPoint:
    case metadata::FieldTypeKind::Array:
    case metadata::FieldTypeKind::Vector:
    case metadata::FieldTypeKind::Map:
    case metadata::FieldTypeKind::Optional:
    case metadata::FieldTypeKind::UserDefined:
        return;
    }
}

// Generate one required scalar field decoder.
void generate_scalar_field_decode(
    std::ostringstream& out, const metadata::FieldModel& field,
    const std::vector<metadata::EnumModel>& enums) {
    write_line(out, 2, "if (key == \"" + field.json.name + "\") {");
    generate_scalar_value_decode(out, field, field.type, enums, "field.value()",
                                 "value." + field.name, 3, std::nullopt);
    write_line(out, 3, "has_" + field.name + " = true;");
    write_line(out, 3, "continue;");
    write_line(out, 2, "}");
}

// Generate one optional scalar field decoder.
void generate_optional_field_decode(
    std::ostringstream& out, const metadata::FieldModel& field,
    const std::vector<metadata::EnumModel>& enums) {
    const auto& inner_type = field.type.arguments[0];

    const std::string decoded_name = "decoded_" + field.name;
    const std::string target_name = decoded_name + "_value";

    write_line(out, 2, "if (key == \"" + field.json.name + "\") {");
    write_line(out, 3, "value." + field.name + " = std::nullopt;");
    write_line(out, 3, "if (field.value().is_null()) {");
    write_line(out, 4, "continue;");
    write_line(out, 3, "}");

    write_line(out, 3,
               optional_inner_type_name(inner_type) + " " + target_name +
                   "{};");
    generate_scalar_value_decode(out, field, inner_type, enums, "field.value()",
                                 target_name, 3, std::nullopt);

    write_line(out, 3, "value." + field.name + " = " + target_name + ";");
    write_line(out, 3, "continue;");
    write_line(out, 2, "}");
}

// Return the generated C++ type spelling for one scalar decoded value.
std::string scalar_value_type_name(const metadata::FieldType& type) {
    switch (type.kind) {
    case metadata::FieldTypeKind::Bool:
        return "bool";
    case metadata::FieldTypeKind::String:
        return "std::string";
    case metadata::FieldTypeKind::Enum:
        return generated_enum_type_name(type);
    case metadata::FieldTypeKind::SignedInteger:
    case metadata::FieldTypeKind::UnsignedInteger:
        return metadata_type_name(type);
    case metadata::FieldTypeKind::FloatingPoint:
    case metadata::FieldTypeKind::Array:
    case metadata::FieldTypeKind::Vector:
    case metadata::FieldTypeKind::Map:
    case metadata::FieldTypeKind::Optional:
    case metadata::FieldTypeKind::UserDefined:
        return metadata_type_name(type);
    }
    return metadata_type_name(type);
}

void generate_vector_scalar_field_decode(
    std::ostringstream& out, const metadata::FieldModel& field,
    const std::vector<metadata::EnumModel>& enums) {
    const auto& element_type = field.type.arguments[0];
    const std::string member_name = "value." + field.name;
    const std::string array_name = "decoded_" + field.name + "_array";
    const std::string index_name = "decoded_" + field.name + "_index";
    const std::string element_name = "decoded_" + field.name + "_element";
    const std::string value_name = "decoded_" + field.name + "_value";

    write_line(out, 2, "if (key == \"" + field.json.name + "\") {");
    write_line(out, 3, "::simdjson::ondemand::array " + array_name + ";");
    write_line(out, 3,
               "runtime_error = field.value().get_array().get(" + array_name +
                   ");");
    write_line(out, 3, "if (runtime_error) {");
    write_line(out, 4, "error.code = DecodeErrorCode::expected_array;");
    generate_value_error_path(out, field, 4, std::nullopt);
    write_line(out, 4, "error.runtime_error = runtime_error;");
    write_line(out, 4, "return false;");
    write_line(out, 3, "}");
    write_line(out, 0, "");

    write_line(out, 3, member_name + ".clear();");
    write_line(out, 3, "std::size_t " + index_name + " = 0;");
    write_line(out, 3,
               "for (auto " + element_name + " : " + array_name + ") {");
    write_line(out, 4,
               scalar_value_type_name(element_type) + " " + value_name + "{};");

    generate_scalar_value_decode(out, field, element_type, enums, element_name,
                                 value_name, 4, index_name);

    write_line(out, 4, member_name + ".push_back(" + value_name + ");");
    write_line(out, 4, "++" + index_name + ";");
    write_line(out, 3, "}");
    write_line(out, 3, "has_" + field.name + " = true;");
    write_line(out, 3, "continue;");
    write_line(out, 2, "}");
}

// Generate one required nested object field decoder.
void generate_user_defined_field_decode(std::ostringstream& out,
                                        const metadata::FieldModel& field) {
    const std::string decoded_name = "decoded_" + field.name;

    write_line(out, 2, "if (key == \"" + field.json.name + "\") {");
    write_line(out, 3, "::simdjson::ondemand::object " + decoded_name + ";");
    write_line(out, 3,
               "runtime_error = field.value().get_object().get(" +
                   decoded_name + ");");
    write_line(out, 3, "if (runtime_error) {");
    write_line(out, 4, "error.code = DecodeErrorCode::expected_object;");
    generate_value_error_path(out, field, 4, std::nullopt);
    write_line(out, 4, "error.runtime_error = runtime_error;");
    write_line(out, 4, "return false;");
    write_line(out, 3, "}");
    write_line(out, 3,
               "if (!detail::decode_object(" + decoded_name + ", value." +
                   field.name + ", error)) {");
    generate_prepend_field_error_path(out, field, 4);
    write_line(out, 4, "return false;");
    write_line(out, 3, "}");
    write_line(out, 3, "has_" + field.name + " = true;");
    write_line(out, 3, "continue;");
    write_line(out, 2, "}");
}

// Generate one supported field decoder.
void generate_field_decode(std::ostringstream& out,
                           const metadata::FieldModel& field,
                           const std::vector<metadata::EnumModel>& enums) {
    switch (field.type.kind) {
    case metadata::FieldTypeKind::Bool:
    case metadata::FieldTypeKind::SignedInteger:
    case metadata::FieldTypeKind::UnsignedInteger:
    case metadata::FieldTypeKind::String:
    case metadata::FieldTypeKind::Enum:
        generate_scalar_field_decode(out, field, enums);
        return;
    case metadata::FieldTypeKind::Vector:
        generate_vector_scalar_field_decode(out, field, enums);
        return;
    case metadata::FieldTypeKind::FloatingPoint:
    case metadata::FieldTypeKind::Array:
    case metadata::FieldTypeKind::Map:
    case metadata::FieldTypeKind::Optional:
        generate_optional_field_decode(out, field, enums);
        return;
    case metadata::FieldTypeKind::UserDefined:
        generate_user_defined_field_decode(out, field);
        return;
    }
}

// Generate the internal object decoder for one model.
void generate_object_decode_function(
    std::ostringstream& out, const metadata::TypeModel& type,
    const std::vector<metadata::EnumModel>& enums) {
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
        if (requires_presence_check(field)) {
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
        generate_field_decode(out, field, enums);
    }

    out << "    }\n"
        << "\n"
        << "    // 3. Verify that every required field was present.\n";

    for (const auto& field : type.fields) {
        if (!requires_presence_check(field)) {
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
            const auto unsupported =
                unsupported_capability_for_field(type, field, project.enums);
            if (unsupported.has_value()) {
                return format_unsupported_capability(*unsupported);
            }
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
        generate_object_decode_function(header, type, project.enums);
        header << "\n";
        generate_root_decode_function(header, type);
    }
    return GenerationResult{true, header.str(), {}};
}

} // namespace cjm::generator::simdjson
