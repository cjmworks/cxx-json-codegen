#include "backends/simdjson/cpp_generator.hpp"
#include "support/golden_diff.hpp"
#include "backends/simdjson/encoder.h"
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <sstream>
#include <string_view>

namespace {

struct ExpectedFragment {
    std::string_view name;
    std::string_view text;
};
} // namespace

TEST_CASE("generate_encode_error_model.emits_public_contract",
          "[simdjson][encoder]") {
    std::ostringstream out;
    cjm::generator::simdjson::detail::generate_encode_error_model(out);

    const std::array cases{
        ExpectedFragment{
            "uses an independent guard",
            "#ifndef CJM_SIMDJSON_ENCODE_RUNTIME_TYPES_DEFINED\n",
        },
        ExpectedFragment{
            "reuses the code path",
            "using EncodePathSegment = DecodePathSegment;\n",
        },
        ExpectedFragment{
            "declares encode-specific error codes",
            "enum class EncodeErrorCode {\n",
        },
        ExpectedFragment{
            "declares the public error object",
            "struct EncodeError {\n",
        },
        ExpectedFragment{
            "declares to_json",
            "std::optional<std::string> to_json(\n",
        },
    };

    for (const auto& test_case : cases) {
        DYNAMIC_SECTION(test_case.name) {
            REQUIRE(out.str().find(test_case.text) != std::string::npos);
        }
    }
}

TEST_CASE("generate_scalar_object_encode_function.writes_two_fields",
          "[simdjson][encoder]") {
    cjm::metadata::TypeModel type;
    type.name = "BoolValues";

    cjm::metadata::FieldModel enabled;
    enabled.name = "enabled";
    enabled.type.kind = cjm::metadata::FieldTypeKind::Bool;
    enabled.json.name = "active";

    cjm::metadata::FieldModel visible;
    visible.name = "visible";
    visible.type.kind = cjm::metadata::FieldTypeKind::Bool;
    visible.json.name = "visible";

    type.fields = {enabled, visible};

    std::ostringstream out;
    cjm::generator::simdjson::detail::generate_scalar_object_encode_function(
        out, type, {});

    const std::string expected = R"(namespace cjm::simdjson::detail {

inline bool encode_object(
    ::simdjson::builder::string_builder& builder,
    const ::BoolValues& value,
    EncodeError& error) {
    builder.start_object();
    bool first_field = true;
    if (!::simdjson::validate_utf8(std::string_view{"active", 6})) {
        error.code = EncodeErrorCode::invalid_utf8_key;
        error.path = {{EncodePathSegmentKind::field, std::string{std::string_view{"active", 6}}, 0}};
        error.runtime_error = ::simdjson::UTF8_ERROR;
        return false;
    }
    if (!first_field) {
        builder.append_comma();
    }
    first_field = false;
    builder.escape_and_append_with_quotes(std::string_view{"active", 6});
    builder.append_colon();
    builder.append(value.enabled);
    if (!::simdjson::validate_utf8(std::string_view{"visible", 7})) {
        error.code = EncodeErrorCode::invalid_utf8_key;
        error.path = {{EncodePathSegmentKind::field, std::string{std::string_view{"visible", 7}}, 0}};
        error.runtime_error = ::simdjson::UTF8_ERROR;
        return false;
    }
    if (!first_field) {
        builder.append_comma();
    }
    first_field = false;
    builder.escape_and_append_with_quotes(std::string_view{"visible", 7});
    builder.append_colon();
    builder.append(value.visible);
    builder.end_object();
    return true;
}

} // namespace cjm::simdjson::detail
)";
    const auto actual = out.str();
    if (actual != expected) {
        FAIL(cjm::test::format_golden_mismatch(expected, actual));
    }
    REQUIRE(actual == expected);
}

TEST_CASE("generate_header.encodes_bool_and_integer_fields",
          "[simdjson][encoder]") {
    using namespace cjm::metadata;

    TypeModel type;
    type.name = "Counters";

    const struct {
        const char* name;
        FieldTypeKind kind;
        const char* spelling;
    } fields[] = {
        {"enabled", FieldTypeKind::Bool, "bool"},
        {"count", FieldTypeKind::SignedInteger, "std::int64_t"},
        {"limits", FieldTypeKind::UnsignedInteger, "std::uint64_t"},
    };

    for (const auto& item : fields) {
        FieldModel field;
        field.name = item.name;
        field.json.name = item.name;
        field.type.kind = item.kind;
        field.type.spelling = item.spelling;
        type.fields.push_back(field);
    }

    ProjectModel project;
    project.types.push_back(type);
    const auto result = cjm::generator::simdjson::generate_header(project);

    INFO(result.error);
    REQUIRE(result.success);
    REQUIRE(result.header.find("to_json<::Counters>") != std::string::npos);

    for (const auto& item : fields) {
        DYNAMIC_SECTION(item.name) {
            const auto expected =
                std::string("builder.append(value.") + item.name + ");";
            REQUIRE(result.header.find(expected) != std::string::npos);
        }
    }
}

TEST_CASE("float.guard", "[simdjson][encoder]") {
    using namespace cjm::metadata;

    TypeModel type;
    type.name = "Quote";

    FieldModel field;
    field.name = "price";
    field.json.name = "cost";
    field.type.kind = FieldTypeKind::FloatingPoint;
    field.type.spelling = "double";
    type.fields.push_back(field);

    std::ostringstream out;
    cjm::generator::simdjson::detail::generate_scalar_object_encode_function(
        out, type, {});
    const auto code = out.str();

    const std::string guard = R"(    if (!std::isfinite(value.price)) {
        error.code = EncodeErrorCode::non_finite_number;
        error.path = {{EncodePathSegmentKind::field, std::string{std::string_view{"cost", 4}}, 0}};
        error.runtime_error = ::simdjson::SUCCESS;
        return false;
    }
)";
    const auto guard_pos = code.find(guard);
    const auto write_pos = code.find("builder.append(value.price);");

    REQUIRE(guard_pos != std::string::npos);
    REQUIRE(write_pos != std::string::npos);
    REQUIRE(guard_pos + guard.size() <= write_pos);
}

TEST_CASE("float.fields", "[simdjson][encoder]") {
    using namespace cjm::metadata;

    for (const auto* name : {"float", "double"}) {
        DYNAMIC_SECTION(name) {
            FieldModel field;
            field.name = "price";
            field.json.name = "price";
            field.type.kind = FieldTypeKind::FloatingPoint;
            field.type.spelling = name;
            field.type.qualified_name = name;

            TypeModel type;
            type.name = "Quote";
            type.fields.push_back(field);
            ProjectModel project;
            project.types.push_back(type);

            const auto result =
                cjm::generator::simdjson::generate_header(project);
            INFO(result.error);
            REQUIRE(result.success);
            REQUIRE(result.header.find("to_json<::Quote>") !=
                    std::string::npos);

            const auto guard =
                result.header.find("if (!std::isfinite(value.price))");
            const auto write =
                result.header.find("builder.append(value.price);");
            REQUIRE(guard != std::string::npos);
            REQUIRE(write != std::string::npos);
            REQUIRE(guard < write);
        }
    }
}

TEST_CASE("string.guard", "[simdjson][encoder]") {
    using namespace cjm::metadata;

    FieldModel field;
    field.name = "name";
    field.json.name = "username";
    field.type.kind = FieldTypeKind::String;
    field.type.spelling = "std::string";

    TypeModel type;
    type.name = "User";
    type.fields.push_back(field);

    std::ostringstream out;
    cjm::generator::simdjson::detail::generate_scalar_object_encode_function(
        out, type, {});
    const auto code = out.str();

    const std::string guard =
        R"(   if (!::simdjson::validate_utf8(value.name)) {
        error.code = EncodeErrorCode::invalid_utf8_string;
        error.path = {{EncodePathSegmentKind::field, std::string{std::string_view{"username", 8}}, 0}};
        error.runtime_error = ::simdjson::UTF8_ERROR;
        return false;
    }
)";
    const auto guard_pos = code.find(guard);
    const auto key_pos = code.find(
        R"(builder.escape_and_append_with_quotes(std::string_view{"username", 8});)");
    REQUIRE(guard_pos != std::string::npos);
    REQUIRE(key_pos != std::string::npos);
    REQUIRE(guard_pos + guard.size() <= key_pos);
}

TEST_CASE("string.write", "[simdjson][encoder]") {
    using namespace cjm::metadata;

    FieldModel field;
    field.name = "name";
    field.json.name = "username";
    field.type.kind = FieldTypeKind::String;
    field.type.spelling = "std::string";

    TypeModel type;
    type.name = "User";
    type.fields.push_back(field);

    std::ostringstream out;
    cjm::generator::simdjson::detail::generate_scalar_object_encode_function(
        out, type, {});
    const auto code = out.str();

    REQUIRE(code.find("builder.escape_and_append_with_quotes(value.name);") !=
            std::string::npos);
    REQUIRE(code.find("builder.append(value.name);") == std::string::npos);
}

TEST_CASE("string.fields", "[simdjson][encoder]") {
    using namespace cjm::metadata;

    FieldModel name;
    name.name = "name";
    name.json.name = "username";
    name.type.kind = FieldTypeKind::String;
    name.type.spelling = "std::string";

    FieldModel age;
    age.name = "age";
    age.json.name = "age";
    age.type.kind = FieldTypeKind::SignedInteger;
    age.type.spelling = "int";

    TypeModel type;
    type.name = "User";
    type.fields = {name, age};

    ProjectModel project;
    project.types.push_back(type);

    const auto result = cjm::generator::simdjson::generate_header(project);
    INFO(result.error);
    REQUIRE(result.success);
    REQUIRE(result.header.find("to_json<::User>") != std::string::npos);
    REQUIRE(result.header.find(
                "builder.escape_and_append_with_quotes(value.name);") !=
            std::string::npos);
    REQUIRE(result.header.find("builder.append(value.age);") !=
            std::string::npos);
}

TEST_CASE("enum.write", "[simdjson][encoder]") {
    using namespace cjm::metadata;

    FieldModel field;
    field.name = "status";
    field.json.name = "state";
    field.type.kind = FieldTypeKind::Enum;
    field.type.qualified_name = "app::Status";

    EnumModel model;
    model.name = "Status";
    model.qualified_name = "app::Status";
    model.enumerators = {"Active", "Disabled"};

    std::ostringstream out;
    cjm::generator::simdjson::detail::generate_enum_field_encode(
        out, field, model, "value.status", 1);
    const auto code = out.str();

    const std::array cases{
        ExpectedFragment{
            "first enumerator",
            R"(if (value.status == ::app::Status::Active) {
        builder.escape_and_append_with_quotes("Active");
    })",
        },
        ExpectedFragment{
            "second enumerator",
            R"(else if (value.status == ::app::Status::Disabled) {
        builder.escape_and_append_with_quotes("Disabled");
    })",
        },
        ExpectedFragment{
            "unmapped value",
            R"(else {
        error.code = EncodeErrorCode::invalid_enum_value;
        error.path = {{EncodePathSegmentKind::field, std::string{std::string_view{"state", 5}}, 0}};
        error.runtime_error = ::simdjson::SUCCESS;
        return false;
    })",
        },
    };

    for (const auto& item : cases) {
        DYNAMIC_SECTION(item.name) {
            INFO(code);
            REQUIRE(code.find(item.text) != std::string::npos);
        }
    }
}

TEST_CASE("enum.empty", "[simdjson][encoder]") {
    using namespace cjm::metadata;

    FieldModel field;
    field.name = "status";
    field.json.name = "state";
    field.type.kind = FieldTypeKind::Enum;

    EnumModel model;
    model.name = "Status";
    // No named enumerators.

    std::ostringstream out;
    cjm::generator::simdjson::detail::generate_enum_field_encode(
        out, field, model, "value.status", 1);
    const auto code = out.str();

    REQUIRE(code.find("if (") == std::string::npos);
    REQUIRE(code.find("else") == std::string::npos);
    REQUIRE(code.find("builder.") == std::string::npos);

    const std::string expected = R"({
        error.code = EncodeErrorCode::invalid_enum_value;
        error.path = {{EncodePathSegmentKind::field, std::string{std::string_view{"state", 5}}, 0}};
        error.runtime_error = ::simdjson::SUCCESS;
        return false;
    })";
    REQUIRE(code.find(expected) != std::string::npos);
}

TEST_CASE("enum.lookup", "[simdjson][encoder]") {
    using namespace cjm::metadata;

    FieldModel field;
    field.name = "status";
    field.json.name = "state";
    field.type.kind = FieldTypeKind::Enum;
    field.type.qualified_name = "app::Status";

    TypeModel type;
    type.name = "User";
    type.fields = {field};

    EnumModel other;
    other.name = "Status";
    other.qualified_name = "other::Status";
    other.enumerators = {"Unknown"};

    EnumModel target;
    target.name = "Status";
    target.qualified_name = "app::Status";
    target.enumerators = {"Active", "Disabled"};

    std::ostringstream out;
    cjm::generator::simdjson::detail::generate_scalar_object_encode_function(
        out, type, {other, target});
    const auto code = out.str();

    REQUIRE(code.find("value.status == ::app::Status::Active") !=
            std::string::npos);
    REQUIRE(code.find("value.status == ::app::Status::Disabled") !=
            std::string::npos);
    REQUIRE(code.find("::other::Status") == std::string::npos);
    REQUIRE(code.find("builder.append(value.status);") == std::string::npos);
}

TEST_CASE("enum.fields", "[simdjson][encoder]") {
    using namespace cjm::metadata;

    FieldModel field;
    field.name = "status";
    field.json.name = "state";
    field.type.kind = FieldTypeKind::Enum;
    field.type.spelling = "Status";
    field.type.qualified_name = "Status";

    TypeModel type;
    type.name = "EnumValues";
    type.qualified_name = "EnumValues";
    type.fields = {field};

    EnumModel model;
    model.name = "Status";
    model.qualified_name = "Status";
    model.enumerators = {"Active", "Disabled"};

    ProjectModel project;
    project.types = {type};
    project.enums = {model};

    const auto result = cjm::generator::simdjson::generate_header(project);
    INFO(result.error);
    REQUIRE(result.success);
    REQUIRE(result.header.find("to_json<::EnumValues>") != std::string::npos);
    REQUIRE(result.header.find("value.status == ::Status::Active") !=
            std::string::npos);
    REQUIRE(result.header.find(
                "builder.escape_and_append_with_quotes(\"Active\");") !=
            std::string::npos);
}

TEST_CASE("object.commas", "[simdjson][encoder]") {
    using namespace cjm::metadata;

    FieldModel first;
    first.name = "enabled";
    first.json.name = "enabled";
    first.type.kind = FieldTypeKind::Bool;

    FieldModel second = first;
    second.name = "visible";
    second.json.name = "visible";

    TypeModel type;
    type.name = "Flags";
    type.fields = {first, second};

    std::ostringstream out;
    cjm::generator::simdjson::detail::generate_scalar_object_encode_function(
        out, type, {});
    const auto code = out.str();

    const auto init = code.find("bool first_field = true;");
    REQUIRE(init != std::string::npos);

    for (const auto* name : {"enabled", "visible"}) {
        DYNAMIC_SECTION(name) {
            const std::string expected =
                "    if (!first_field) {\n"
                "        builder.append_comma();\n"
                "    }\n"
                "    first_field = false;\n"
                "    "
                "builder.escape_and_append_with_quotes(std::string_view{\"" +
                std::string(name) + "\", 7});";
            const auto write = code.find(expected);
            REQUIRE(write != std::string::npos);
            REQUIRE(init < write);
        }
    }
}

TEST_CASE("value.scalars", "[simdjson][encoder]") {
    using namespace cjm::metadata;

    const struct {
        const char* name;
        FieldTypeKind kind;
        std::string_view expression;
        std::size_t indent;
        std::string_view expected;
    } cases[] = {
        {"bool", FieldTypeKind::Bool, "value.enabled", 1,
         "    builder.append(value.enabled);\n"},
        {"signed", FieldTypeKind::SignedInteger, "value.count", 1,
         "    builder.append(value.count);\n"},
        {"unsigned", FieldTypeKind::UnsignedInteger, "value.limit", 1,
         "    builder.append(value.limit);\n"},
        {"dereferenced", FieldTypeKind::SignedInteger, "*value.count", 1,
         "    builder.append(*value.count);\n"},
    };

    for (const auto& item : cases) {
        DYNAMIC_SECTION(item.name) {
            FieldModel field;
            FieldType value_type;
            value_type.kind = item.kind;

            std::ostringstream out;
            cjm::generator::simdjson::detail::generate_value_encode(
                out, field, value_type, item.expression, {}, item.indent);
            REQUIRE(out.str() == item.expected);
        }
    }
}

TEST_CASE("value.optional", "[simdjson][encoder]") {
    using namespace cjm::metadata;

    FieldType inner;
    inner.kind = FieldTypeKind::SignedInteger;

    FieldModel field;
    field.name = "count";
    field.json.name = "count";
    field.type.kind = FieldTypeKind::Optional;
    field.type.arguments = {inner};

    std::ostringstream out;
    cjm::generator::simdjson::detail::generate_value_encode(
        out, field, field.type, "value.count", {}, 1);

    const std::string expected = "    if ((value.count).has_value()) {\n"
                                 "        builder.append(*(value.count));\n"
                                 "    } else {\n"
                                 "        builder.append_null();\n"
                                 "    }\n";

    REQUIRE(out.str() == expected);
}

TEST_CASE("optional.omission", "[simdjson][encoder]") {
    using namespace cjm::metadata;

    FieldType inner;
    inner.kind = FieldTypeKind::SignedInteger;

    FieldModel field;
    field.name = "count";
    field.json.name = "total";
    field.json.omit_empty = true;
    field.type.kind = FieldTypeKind::Optional;
    field.type.arguments = {inner};

    TypeModel type;
    type.name = "Counts";
    type.fields = {field};

    std::ostringstream out;
    cjm::generator::simdjson::detail::generate_scalar_object_encode_function(
        out, type, {});
    const auto code = out.str();

    const std::string expected =
        "    if (value.count.has_value()) {\n"
        "        if (!::simdjson::validate_utf8(std::string_view{\"total\", "
        "5})) {\n"
        "            error.code = EncodeErrorCode::invalid_utf8_key;\n"
        "            error.path = {{EncodePathSegmentKind::field, "
        "std::string{std::string_view{\"total\", 5}}, 0}};\n"
        "            error.runtime_error = ::simdjson::UTF8_ERROR;\n"
        "            return false;\n"
        "        }\n"
        "    if (!first_field) {\n"
        "        builder.append_comma();\n"
        "    }\n"
        "    first_field = false;\n"
        "    builder.escape_and_append_with_quotes(std::string_view{\"total\", "
        "5});\n"
        "    builder.append_colon();\n"
        "        if ((value.count).has_value()) {\n"
        "            builder.append(*(value.count));\n"
        "        } else {\n"
        "            builder.append_null();\n"
        "        }\n"
        "    }\n"
        "    builder.end_object();\n";
    REQUIRE(code.find(expected) != std::string::npos);
}

TEST_CASE("optional.fields", "[simdjson][encoder]") {
    using namespace cjm::metadata;

    const struct {
        const char* name;
        FieldTypeKind kind;
        const char* spelling;
        bool encodable;
    } cases[] = {
        {"bool", FieldTypeKind::Bool, "bool", true},
        {"signed", FieldTypeKind::SignedInteger, "int", true},
        {"unsigned", FieldTypeKind::UnsignedInteger, "unsigned int", true},
        {"string", FieldTypeKind::String, "std::string", true},
        {"enum", FieldTypeKind::Enum, "Status", true},
        {"float", FieldTypeKind::FloatingPoint, "float", true},
        {"double", FieldTypeKind::FloatingPoint, "double", true},
    };

    for (const auto& item : cases) {
        DYNAMIC_SECTION(item.name) {
            FieldType inner;
            inner.kind = item.kind;
            inner.spelling = item.spelling;
            inner.qualified_name = item.spelling;

            FieldModel field;
            field.name = "value";
            field.json.name = "value";
            field.type.kind = FieldTypeKind::Optional;
            field.type.spelling =
                "std::optional<" + std::string(item.spelling) + ">";
            field.type.qualified_name = "std::optional";
            field.type.arguments = {inner};

            TypeModel type;
            type.name = "OptionalValues";
            type.qualified_name = "OptionalValues";
            type.fields = {field};

            ProjectModel project;
            project.types = {type};
            if (item.kind == FieldTypeKind::Enum) {
                EnumModel model;
                model.name = "Status";
                model.qualified_name = "Status";
                model.enumerators = {"Active", "Disabled"};
                project.enums = {model};
            }

            const auto result =
                cjm::generator::simdjson::generate_header(project);
            INFO(result.error);
            REQUIRE(result.success);

            const bool has_encoder =
                result.header.find("to_json<::OptionalValues>") !=
                std::string::npos;
            REQUIRE(has_encoder == item.encodable);
        }
    }
}

TEST_CASE("value.string", "[simdjson][encoder]") {
    using namespace cjm::metadata;

    FieldType inner;
    inner.kind = FieldTypeKind::String;

    FieldModel field;
    field.name = "name";
    field.json.name = "username";
    field.type.kind = FieldTypeKind::Optional;
    field.type.arguments = {inner};

    std::ostringstream out;
    cjm::generator::simdjson::detail::generate_value_encode(
        out, field, inner, "*(value.name)", {}, 2);

    const std::string expected =
        "        if (!::simdjson::validate_utf8(*(value.name))) {\n"
        "            error.code = EncodeErrorCode::invalid_utf8_string;\n"
        "            error.path = {{EncodePathSegmentKind::field, "
        "std::string{std::string_view{\"username\", 8}}, 0}};\n"
        "            error.runtime_error = ::simdjson::UTF8_ERROR;\n"
        "            return false;\n"
        "        }\n"
        "        builder.escape_and_append_with_quotes(*(value.name));\n";

    const auto actual = out.str();
    if (actual != expected) {
        FAIL(cjm::test::format_golden_mismatch(expected, actual));
    }
}

TEST_CASE("enum.expression", "[simdjson][encoder]") {
    using namespace cjm::metadata;

    FieldModel field;
    field.name = "status";
    field.json.name = "state";
    field.type.kind = FieldTypeKind::Optional;

    EnumModel model;
    model.name = "Status";
    model.qualified_name = "app::Status";
    model.enumerators = {"Active"};

    std::ostringstream out;
    cjm::generator::simdjson::detail::generate_enum_field_encode(
        out, field, model, "*(value.status)", 2);

    const std::string expected =
        "        if (*(value.status) == ::app::Status::Active) {\n"
        "            builder.escape_and_append_with_quotes(\"Active\");\n"
        "        }\n"
        "        else {\n"
        "            error.code = EncodeErrorCode::invalid_enum_value;\n"
        "            error.path = {{EncodePathSegmentKind::field, "
        "std::string{std::string_view{\"state\", 5}}, 0}};\n"
        "            error.runtime_error = ::simdjson::SUCCESS;\n"
        "            return false;\n"
        "        }\n";

    const auto actual = out.str();
    if (actual != expected) {
        FAIL(cjm::test::format_golden_mismatch(expected, actual));
    }
}

TEST_CASE("value.optional_enum", "[simdjson][encoder]") {
    using namespace cjm::metadata;

    FieldType inner;
    inner.kind = FieldTypeKind::Enum;
    inner.qualified_name = "app::Status";

    FieldModel field;
    field.name = "status";
    field.json.name = "state";
    field.type.kind = FieldTypeKind::Optional;
    field.type.qualified_name = "std::optional";
    field.type.arguments = {inner};

    EnumModel model;
    model.name = "Status";
    model.qualified_name = "app::Status";
    model.enumerators = {"Active"};

    std::ostringstream out;
    cjm::generator::simdjson::detail::generate_value_encode(
        out, field, field.type, "value.status", {model}, 1);

    const std::string expected =
        "    if ((value.status).has_value()) {\n"
        "        if (*(value.status) == ::app::Status::Active) {\n"
        "            builder.escape_and_append_with_quotes(\"Active\");\n"
        "        }\n"
        "        else {\n"
        "            error.code = EncodeErrorCode::invalid_enum_value;\n"
        "            error.path = {{EncodePathSegmentKind::field, "
        "std::string{std::string_view{\"state\", 5}}, 0}};\n"
        "            error.runtime_error = ::simdjson::SUCCESS;\n"
        "            return false;\n"
        "        }\n"
        "    } else {\n"
        "        builder.append_null();\n"
        "    }\n";

    const auto actual = out.str();
    if (actual != expected) {
        FAIL(cjm::test::format_golden_mismatch(expected, actual));
    }
}

TEST_CASE("value.optional_float", "[simdjson][encoder]") {
    using namespace cjm::metadata;

    FieldType inner;
    inner.kind = FieldTypeKind::FloatingPoint;
    inner.spelling = "double";
    inner.qualified_name = "double";

    FieldModel field;
    field.name = "ratio";
    field.json.name = "fraction";
    field.type.kind = FieldTypeKind::Optional;
    field.type.arguments = {inner};

    std::ostringstream out;
    cjm::generator::simdjson::detail::generate_value_encode(
        out, field, field.type, "value.ratio", {}, 1);

    const std::string expected =
        "    if ((value.ratio).has_value()) {\n"
        "        if (!std::isfinite(*(value.ratio))) {\n"
        "            error.code = EncodeErrorCode::non_finite_number;\n"
        "            error.path = {{EncodePathSegmentKind::field, "
        "std::string{std::string_view{\"fraction\", 8}}, 0}};\n"
        "            error.runtime_error = ::simdjson::SUCCESS;\n"
        "            return false;\n"
        "        }\n"
        "        builder.append(*(value.ratio));\n"
        "    } else {\n"
        "        builder.append_null();\n"
        "    }\n";
    const auto actual = out.str();
    if (actual != expected) {
        FAIL(cjm::test::format_golden_mismatch(expected, actual));
    }
}

TEST_CASE("value.object", "[simdjson][encoder]") {
    using namespace cjm::metadata;

    FieldModel field;
    field.name = "address";
    field.json.name = "home";
    field.type.kind = FieldTypeKind::UserDefined;
    field.type.qualified_name = "app::Address";

    std::ostringstream out;
    cjm::generator::simdjson::detail::generate_value_encode(
        out, field, field.type, "value.address", {}, 1);

    const std::string expected =
        "    if (!encode_object(builder, value.address, error)) {\n"
        "        error.path.insert(error.path.begin(),\n"
        "            EncodePathSegment{EncodePathSegmentKind::field, "
        "std::string{std::string_view{\"home\", 4}}, 0});\n"
        "        return false;\n"
        "    }\n";

    const auto actual = out.str();
    if (actual != expected) {
        FAIL(cjm::test::format_golden_mismatch(expected, actual));
    }
}

TEST_CASE("object.nested", "[simdjson][encoder]") {
    using namespace cjm::metadata;

    FieldModel field;
    field.name = "address";
    field.json.name = "home";
    field.type.kind = FieldTypeKind::UserDefined;
    field.type.qualified_name = "app::Address";

    TypeModel type;
    type.name = "User";
    type.qualified_name = "app::User";
    type.fields = {field};

    std::ostringstream out;
    cjm::generator::simdjson::detail::generate_scalar_object_encode_function(
        out, type, {});
    const auto code = out.str();

    REQUIRE(code.find("if (!encode_object(builder, value.address, error))") !=
            std::string::npos);
    REQUIRE(code.find("builder.append(value.address);") == std::string::npos);
}

TEST_CASE("capability.scalar", "[simdjson][encoder]") {
    using namespace cjm::metadata;
    const struct {
        const char* name;
        FieldTypeKind kind;
        bool expected;
    } cases[] = {
        {"bool", FieldTypeKind::Bool, true},
        {"int", FieldTypeKind::SignedInteger, true},
        {"unsigned", FieldTypeKind::UnsignedInteger, true},
        {"string", FieldTypeKind::String, true},
        {"float", FieldTypeKind::FloatingPoint, true},
        {"double", FieldTypeKind::FloatingPoint, true},
        {"long double", FieldTypeKind::FloatingPoint, false},
        {"Address", FieldTypeKind::UserDefined, false},
        {"optional", FieldTypeKind::Optional, false},
        {"vector", FieldTypeKind::Vector, false},
    };

    for (const auto& item : cases) {
        DYNAMIC_SECTION(item.name) {
            FieldType type;
            type.kind = item.kind;
            type.spelling = item.name;
            REQUIRE(cjm::generator::simdjson::detail::
                        is_supported_scalar_encode_type(type) == item.expected);
        }
    }
}

TEST_CASE("capability.long_double", "[simdjson][encoder]") {
    using namespace cjm::metadata;

    const FieldType number{FieldTypeKind::FloatingPoint, "long double",
                           "long double"};
    const struct {
        const char* name;
        FieldType type;
    } cases[] = {
        {"scalar", number},
        {"optional",
         {FieldTypeKind::Optional,
          "std::optional<long double>",
          "std::optional",
          {number}}},
        {"vector",
         {FieldTypeKind::Vector,
          "std::vector<long double>",
          "std::vector",
          {number}}},
        {"array",
         {FieldTypeKind::Array,
          "std::array<long double, 2>",
          "std::array",
          {number},
          2}},
        {"map",
         {FieldTypeKind::Map,
          "std::map<std::string, long double>",
          "std::map",
          {{FieldTypeKind::String, "std::string", "std::string"}, number}}},
    };

    for (const auto& item : cases) {
        for (const bool ignored : {false, true}) {
            DYNAMIC_SECTION(item.name
                            << (ignored ? ".ignored" : ".participating")) {
                FieldModel extended;
                extended.name = "ratio";
                extended.json.name = "amount";
                extended.json.ignored = ignored;
                extended.type = item.type;

                FieldModel count;
                count.name = "count";
                count.json.name = "count";
                count.type =
                    FieldType{FieldTypeKind::SignedInteger, "int", "int"};

                TypeModel model;
                model.name = "ExtendedValues";
                model.qualified_name = "app::ExtendedValues";
                model.fields = {extended, count};
                ProjectModel project;
                project.types = {model};

                const auto result =
                    cjm::generator::simdjson::generate_header(project);
                INFO(result.error);
                REQUIRE(result.success == ignored);

                if (!ignored) {
                    REQUIRE(result.header.empty());
                    REQUIRE(result.error.find("unsupported capability") !=
                            std::string::npos);
                    REQUIRE(result.error.find("model 'app::ExtendedValues'") !=
                            std::string::npos);
                    REQUIRE(result.error.find("field 'ratio'") !=
                            std::string::npos);
                    REQUIRE(result.error.find("json field 'amount'") !=
                            std::string::npos);
                    REQUIRE(result.error.find("C++ type '" +
                                              item.type.spelling + "'") !=
                            std::string::npos);
                } else {
                    REQUIRE(result.error.empty());
                    REQUIRE(result.header.find(
                                "from_json<::app::ExtendedValues>") !=
                            std::string::npos);
                    REQUIRE(
                        result.header.find("to_json<::app::ExtendedValues>") !=
                        std::string::npos);
                    REQUIRE(
                        result.header.find("builder.append(value.count);") !=
                        std::string::npos);
                    REQUIRE(result.header.find("value.ratio") ==
                            std::string::npos);
                    REQUIRE(result.header.find("\"amount\"") ==
                            std::string::npos);
                }
            }
        }
    }
}

TEST_CASE("capability.nested_long_double", "[simdjson][encoder]") {
    using namespace cjm::metadata;

    for (const bool optional : {false, true}) {
        for (const bool ignored : {false, true}) {
            DYNAMIC_SECTION(
                (optional ? "optional_child" : "required_child")
                << (ignored ? ".ignored_leaf" : ".participating_leaf")) {
                FieldModel ratio;
                ratio.name = "ratio";
                ratio.json.name = "amount";
                ratio.json.ignored = ignored;
                ratio.type = {FieldTypeKind::FloatingPoint, "long double",
                              "long double"};

                FieldModel count;
                count.name = "count";
                count.json.name = "count";
                count.type = {FieldTypeKind::SignedInteger, "int", "int"};

                TypeModel child;
                child.name = "Metrics";
                child.qualified_name = "app::Metrics";
                child.fields = {ratio, count};

                FieldModel metrics;
                metrics.name = "metrics";
                metrics.json.name = "stats";
                metrics.type = {FieldTypeKind::UserDefined, "app::Metrics",
                                "app::Metrics"};
                if (optional) {
                    const auto inner = metrics.type;
                    metrics.type = {FieldTypeKind::Optional,
                                    "std::optional<app::Metrics>",
                                    "std::optional",
                                    {inner}};
                }

                TypeModel parent;
                parent.name = "Report";
                parent.qualified_name = "app::Report";
                parent.fields = {metrics};
                ProjectModel project;
                project.types = {child, parent};

                const auto result =
                    cjm::generator::simdjson::generate_header(project);
                INFO(result.error);
                REQUIRE(result.success == ignored);
                if (!ignored) {
                    REQUIRE(result.header.empty());
                    REQUIRE(result.error.find("unsupported capability") !=
                            std::string::npos);
                    REQUIRE(result.error.find("model 'app::Metrics'") !=
                            std::string::npos);
                    REQUIRE(result.error.find("field 'ratio'") !=
                            std::string::npos);
                    REQUIRE(result.error.find("json field 'amount'") !=
                            std::string::npos);
                    REQUIRE(result.error.find("C++ type 'long double'") !=
                            std::string::npos);
                    REQUIRE(result.error.find("model 'app::Report'") ==
                            std::string::npos);
                } else {
                    REQUIRE(result.error.empty());
                    REQUIRE(result.header.find("from_json<::app::Metrics>") !=
                            std::string::npos);
                    REQUIRE(result.header.find("to_json<::app::Metrics>") !=
                            std::string::npos);
                    REQUIRE(result.header.find("from_json<::app::Report>") !=
                            std::string::npos);
                    REQUIRE(result.header.find("to_json<::app::Report>") !=
                            std::string::npos);
                    REQUIRE(
                        result.header.find("builder.append(value.count);") !=
                        std::string::npos);
                    REQUIRE(result.header.find("encode_object(builder, ") !=
                            std::string::npos);
                    REQUIRE(result.header.find("value.ratio") ==
                            std::string::npos);
                    REQUIRE(result.header.find("\"amount\"") ==
                            std::string::npos);
                }
            }
        }
    }
}

TEST_CASE("capability.object", "[simdjson][encoder]") {
    using namespace cjm::metadata;
    FieldType type;
    type.kind = FieldTypeKind::UserDefined;
    type.qualified_name = "app::Address";

    const struct {
        const char* name;
        std::set<std::string> encoded_models;
        bool expected;
    } cases[] = {
        {"missing", {}, false},
        {"available", {"app::Address"}, true},
        {"different_namespace", {"other::Address"}, false},
    };
    for (const auto& item : cases) {
        DYNAMIC_SECTION(item.name) {
            REQUIRE(cjm::generator::simdjson::detail::
                        is_supported_value_encode_type(
                            type, item.encoded_models) == item.expected);
        }
    }
}

TEST_CASE("object.capability", "[simdjson][encoder]") {
    using namespace cjm::metadata;

    const struct {
        const char* name;
        bool container;
        bool expected;
    } cases[] = {
        {"scalar_child", false, true},
        {"unsupported_child", true, false},
    };

    for (const auto& item : cases) {
        DYNAMIC_SECTION(item.name) {
            FieldModel city;
            city.name = "city";
            city.json.name = "city";
            city.type =
                FieldType{FieldTypeKind::String, "std::string", "std::string"};

            if (item.container) {
                const auto element = city.type;
                city.type.kind = FieldTypeKind::Vector;
                city.type.spelling = "std::vector<std::string>";
                city.type.qualified_name = "std::vector";
                city.type.arguments = {element};
            }

            TypeModel address;
            address.name = "Address";
            address.qualified_name = "app::Address";
            address.fields = {city};

            FieldModel home;
            home.name = "address";
            home.json.name = "home";
            home.type = FieldType{FieldTypeKind::UserDefined, "app::Address",
                                  "app::Address"};

            TypeModel user;
            user.name = "User";
            user.qualified_name = "app::User";
            user.fields = {home};

            ProjectModel project;
            project.types = {address, user};
            const auto result =
                cjm::generator::simdjson::generate_header(project);
            INFO(result.error);
            REQUIRE(result.success);
            REQUIRE((result.header.find("to_json<::app::Address>") !=
                     std::string::npos) == item.expected);
            REQUIRE((result.header.find("to_json<::app::User>") !=
                     std::string::npos) == item.expected);
            REQUIRE(result.header.find("from_json<::app::User>") !=
                    std::string::npos);
        }
    }
}

TEST_CASE("capability.optional_object", "[simdjson][encoder]") {
    using namespace cjm::metadata;

    FieldType object;
    object.kind = FieldTypeKind::UserDefined;
    object.qualified_name = "app::Address";

    FieldType optional;
    optional.kind = FieldTypeKind::Optional;
    optional.arguments = {object};

    const std::set<std::string> available{"app::Address"};
    using cjm::generator::simdjson::detail::is_supported_value_encode_type;

    REQUIRE_FALSE(is_supported_value_encode_type(optional, {}));
    REQUIRE(is_supported_value_encode_type(optional, available));

    FieldType nested;
    nested.kind = FieldTypeKind::Optional;
    nested.arguments = {optional};
    REQUIRE_FALSE(is_supported_value_encode_type(nested, available));
}

#include "backends/simdjson/string_literal.h"

TEST_CASE("literal.basic", "[simdjson][generated]") {
    const struct {
        const char* name;
        std::string_view input;
        std::string_view expected;
    } cases[] = {
        {"plain", "home", R"("home")"},
        {"quote", R"(display"name)", R"("display\"name")"},
        {"backslash", R"(a\b)", R"("a\\b")"},
        {"newline", "\n", R"("\012")"},
        {"nul_then_digit ",
         std::string_view{"A\0"
                          "7",
                          3},
         R"("A\0007")"},
        {"invalid_byte", "\xFF", R"("\377")"},
    };

    for (const auto& item : cases) {
        DYNAMIC_SECTION(item.name) {
            const auto actual =
                cjm::generator::simdjson::detail::cpp_string_literal(
                    item.input);
            if (actual != item.expected) {
                FAIL(cjm::test::format_golden_mismatch(item.expected, actual));
            }
        }
    }
}

TEST_CASE("literal.view", "[simdjson][encoder]") {
    const auto actual =
        cjm::generator::simdjson::detail::cpp_string_view_expression(
            std::string_view{"A\0B", 3});
    const std::string expected = R"(std::string_view{"A\000B", 3})";

    if (actual != expected) {
        FAIL(cjm::test::format_golden_mismatch(expected, actual));
    }
    REQUIRE(actual == expected);
}

TEST_CASE("object.key_nul", "[simdjson][encoder]") {
    cjm::metadata::FieldModel field;
    field.name = "enabled";
    field.json.name = std::string{"A\0B", 3};
    field.type.kind = cjm::metadata::FieldTypeKind::Bool;

    cjm::metadata::TypeModel type;
    type.name = "Flags";
    type.fields = {field};

    std::ostringstream out;
    cjm::generator::simdjson::detail::generate_scalar_object_encode_function(
        out, type, {});

    const std::string expected =
        R"(    builder.escape_and_append_with_quotes(std::string_view{"A\000B", 3});)";
    const auto code = out.str();
    INFO(code);
    REQUIRE(code.find(expected) != std::string::npos);
}

TEST_CASE("key.validation", "[simdjson][encoder]") {
    std::ostringstream out;
    cjm::generator::simdjson::detail::generate_object_key_validation(
        out, std::string_view{"A\0\xFF", 3}, 1);

    const std::string expected =
        R"(    if (!::simdjson::validate_utf8(std::string_view{"A\000\377", 3})) {
        error.code = EncodeErrorCode::invalid_utf8_key;
        error.path = {{EncodePathSegmentKind::field, std::string{std::string_view{"A\000\377", 3}}, 0}};
        error.runtime_error = ::simdjson::UTF8_ERROR;
        return false;
    }
)";

    const auto actual = out.str();
    if (actual != expected) {
        FAIL(cjm::test::format_golden_mismatch(expected, actual));
    }
    REQUIRE(actual == expected);
}

TEST_CASE("key.omission_guard", "[simdjson][encoder]") {
    using namespace cjm::metadata;

    FieldModel field;
    field.name = "count";
    field.json.name = "total";
    field.json.omit_empty = true;
    field.type.kind = FieldTypeKind::Optional;
    FieldType inner;
    inner.kind = FieldTypeKind::SignedInteger;
    field.type.arguments = {inner};

    TypeModel type;
    type.name = "Counts";
    type.fields = {field};

    std::ostringstream out;
    cjm::generator::simdjson::detail::generate_scalar_object_encode_function(
        out, type, {});

    const std::string expected = R"(    if (value.count.has_value()) {
        if (!::simdjson::validate_utf8(std::string_view{"total", 5})) {
)";
    const auto code = out.str();
    INFO(code);
    REQUIRE(code.find(expected) != std::string::npos);
}

TEST_CASE("value.error_key", "[simdjson][encoder]") {
    using namespace cjm::metadata;
    const struct {
        const char* name;
        FieldTypeKind kind;
        const char* error;
    } cases[] = {
        {"floating", FieldTypeKind::FloatingPoint, "non_finite_number"},
        {"string", FieldTypeKind::String, "invalid_utf8_string"},
    };

    for (const auto& item : cases) {
        DYNAMIC_SECTION(item.name) {
            FieldModel field;
            field.name = "value";
            field.json.name = std::string{"A\0B", 3};
            field.type.kind = item.kind;

            std::ostringstream out;
            cjm::generator::simdjson::detail::generate_value_encode(
                out, field, field.type, "value.type", {}, 1);
            const std::string expected =
                "        error.code = EncodeErrorCode::" +
                std::string(item.error) + ";\n" +
                R"(        error.path = {{EncodePathSegmentKind::field, std::string{std::string_view{"A\000B", 3}}, 0}};)";

            const auto code = out.str();
            INFO(code);
            REQUIRE(code.find(expected) != std::string::npos);
        }
    }
}

TEST_CASE("enum.error_key", "[simdjson][encoder]") {
    using namespace cjm::metadata;

    FieldModel field;
    field.name = "status";
    field.json.name = std::string{"A\0B", 3};
    field.type.kind = FieldTypeKind::Enum;
    field.type.qualified_name = "app::Status";

    EnumModel model;
    model.name = "Status";
    model.qualified_name = "app::Status";
    model.enumerators = {"Active"};

    std::ostringstream out;
    cjm::generator::simdjson::detail::generate_enum_field_encode(
        out, field, model, "value.status", 1);

    const std::string expected =
        R"(        error.code = EncodeErrorCode::invalid_enum_value;
        error.path = {{EncodePathSegmentKind::field, std::string{std::string_view{"A\000B", 3}}, 0}};
)";
    const auto code = out.str();
    INFO(code);
    REQUIRE(code.find(expected) != std::string::npos);
}

TEST_CASE("object.parent_key", "[simdjson][encoder]") {
    using namespace cjm::metadata;

    FieldModel field;
    field.name = "address";
    field.json.name = std::string{"A\0B", 3};
    field.type.kind = FieldTypeKind::UserDefined;
    field.type.qualified_name = "app::Status";

    std::ostringstream out;
    cjm::generator::simdjson::detail::generate_value_encode(
        out, field, field.type, "value.address", {}, 1);

    const std::string expected =
        R"(    if (!encode_object(builder, value.address, error)) {
        error.path.insert(error.path.begin(),
            EncodePathSegment{EncodePathSegmentKind::field, std::string{std::string_view{"A\000B", 3}}, 0});
        return false;
    }
)";

    const auto actual = out.str();
    if (actual != expected) {
        FAIL(cjm::test::format_golden_mismatch(expected, actual));
    }
    REQUIRE(actual == expected);
}

TEST_CASE("object.error_key", "[simdjson][encoder]") {
    using namespace cjm::metadata;
    const struct {
        const char* name;
        FieldTypeKind kind;
        const char* error;
    } cases[] = {
        {"floating", FieldTypeKind::FloatingPoint, "non_finite_number"},
        {"string", FieldTypeKind::String, "invalid_utf8_string"},
    };

    for (const auto& item : cases) {
        DYNAMIC_SECTION(item.name) {
            FieldModel field;
            field.name = "value";
            field.json.name = std::string{"A\0B", 3};
            field.type.kind = item.kind;

            TypeModel type;
            type.name = "Values";
            type.fields = {field};

            std::ostringstream out;
            cjm::generator::simdjson::detail::
                generate_scalar_object_encode_function(out, type, {});
            const std::string expected =
                "        error.code = EncodeErrorCode::" +
                std::string(item.error) + ";\n" +
                R"(        error.path = {{EncodePathSegmentKind::field, std::string{std::string_view{"A\000B", 3}}, 0}};)";

            const auto code = out.str();
            INFO(code);
            REQUIRE(code.find(expected) != std::string::npos);
        }
    }
}

TEST_CASE("validate_encode.long_double", "[simdjson][encoder]") {
    using namespace cjm::metadata;

    FieldModel field;
    field.name = "ratio";
    field.json.name = "amount";
    field.type = {FieldTypeKind::FloatingPoint, "long double", "long double"};

    TypeModel model;
    model.name = "Metrics";
    model.qualified_name = "app::Metrics";
    model.fields = {field};

    ProjectModel project;
    project.types = {model};

    const auto error =
        cjm::generator::simdjson::detail::validate_encode_project(project);
    INFO(error);
    for (const auto* fragment : {
             "model 'app::Metrics'",
             "field 'ratio'",
             "json field 'amount'",
             "C++ type 'long double'",
             "encoding",
             "narrowing",
         }) {
        REQUIRE(error.find(fragment) != std::string::npos);
    }
}
