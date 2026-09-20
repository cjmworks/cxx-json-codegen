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
    if (!first_field) {
        builder.append_comma();
    }
    first_field = false;
    builder.escape_and_append_with_quotes("active");
    builder.append_colon();
    builder.append(value.enabled);
    if (!first_field) {
        builder.append_comma();
    }
    first_field = false;
    builder.escape_and_append_with_quotes("visible");
    builder.append_colon();
    builder.append(value.visible);
    builder.end_object();
    return true;
}

} // namespace cjm::simdjson::detail
)";
    REQUIRE(out.str() == expected);
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
        error.path = {{EncodePathSegmentKind::field, "cost", 0}};
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
        error.path = {{EncodePathSegmentKind::field, "username", 0}};
        error.runtime_error = ::simdjson::UTF8_ERROR;
        return false;
    }
)";
    const auto guard_pos = code.find(guard);
    const auto key_pos =
        code.find(R"(builder.escape_and_append_with_quotes("username");)");
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
        error.path = {{EncodePathSegmentKind::field, "state", 0}};
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
        error.path = {{EncodePathSegmentKind::field, "state", 0}};
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
                "    builder.escape_and_append_with_quotes(\"" +
                std::string(name) + "\");";
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
        "    if (!first_field) {\n"
        "        builder.append_comma();\n"
        "    }\n"
        "    first_field = false;\n"
        "    builder.escape_and_append_with_quotes(\"total\");\n"
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
        "\"username\", 0}};\n"
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
        "            error.path = {{EncodePathSegmentKind::field, \"state\", "
        "0}};\n"
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
        "            error.path = {{EncodePathSegmentKind::field, \"state\", "
        "0}};\n"
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
        "\"fraction\", 0}};\n"
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
