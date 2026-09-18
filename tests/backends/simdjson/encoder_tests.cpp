#include "backends/simdjson/cpp_generator.hpp"
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
        out, type);

    const std::string expected = R"(namespace cjm::simdjson::detail {

inline bool encode_object(
    ::simdjson::builder::string_builder& builder,
    const ::BoolValues& value,
    EncodeError& error) {
    builder.start_object();
    builder.escape_and_append_with_quotes("active");
    builder.append_colon();
    builder.append(value.enabled);
    builder.append_comma();
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
        out, type);
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
        out, type);
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
