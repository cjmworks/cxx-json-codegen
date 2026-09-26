#include "backends/simdjson/cpp_generator.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

TEST_CASE("float.fields", "[simdjson][decoder]") {
    using namespace cjm::metadata;

    for (const auto* name : {"float", "double"}) {
        DYNAMIC_SECTION(name) {
            FieldModel field;
            field.name = "ratio";
            field.json.name = "ratio";
            field.type.kind = FieldTypeKind::FloatingPoint;
            field.type.spelling = name;
            field.type.qualified_name = name;

            TypeModel type;
            type.name = "FloatValues";
            type.fields.push_back(field);

            ProjectModel project;
            project.types.push_back(type);
            const auto result =
                cjm::generator::simdjson::generate_header(project);

            INFO(result.error);
            REQUIRE(result.success);
            REQUIRE(result.header.find("from_json<::FloatValues>") !=
                    std::string::npos);
            REQUIRE(result.header.find(
                        "field.value().get_double().get(decoded_ratio)") !=
                    std::string::npos);
            REQUIRE(result.header.find(
                        "DecodeErrorCode::floating_point_overflow;") !=
                    std::string::npos);
            REQUIRE(
                result.header.find(
                    "value.ratio = static_cast<target_type>(decoded_ratio);") !=
                std::string::npos);
        }
    }
}

TEST_CASE("float.unsupported", "[simdjson][decoder]") {
    using namespace cjm::metadata;

    const FieldType number{FieldTypeKind::FloatingPoint, "double", "double"};
    const struct {
        const char* name;
        FieldType type;
    } cases[] = {
        {"long_double",
         {FieldTypeKind::FloatingPoint, "long double", "long double"}},
        {"optional_long_double",
         {FieldTypeKind::Optional,
          "std::optional<long double>",
          "std::optional",
          {{FieldTypeKind::FloatingPoint, "long double", "long double"}}}},
        {"vector",
         {FieldTypeKind::Vector,
          "std::vector<double>",
          "std::vector",
          {number}}},
        {"array",
         {FieldTypeKind::Array,
          "std::array<double, 2>",
          "std::array",
          {number},
          2}},
        {"map",
         {FieldTypeKind::Map,
          "std::map<std::string, double>",
          "std::map",
          {{FieldTypeKind::String, "std::string", "std::string"}, number}}},
    };

    for (const auto& item : cases) {
        DYNAMIC_SECTION(item.name) {
            FieldModel field;
            field.name = "ratio";
            field.json.name = "value";
            field.type = item.type;
            TypeModel type;
            type.name = "UnsupportedValues";
            type.fields.push_back(field);
            ProjectModel project;
            project.types.push_back(type);

            const auto result =
                cjm::generator::simdjson::generate_header(project);
            REQUIRE_FALSE(result.success);
            REQUIRE(result.header.empty());
            REQUIRE(result.error.find("UnsupportedValues") !=
                    std::string::npos);
            REQUIRE(result.error.find("field 'ratio'") != std::string::npos);
            REQUIRE(result.error.find("json field 'value'") !=
                    std::string::npos);
            REQUIRE(result.error.find(item.type.spelling) != std::string::npos);
        }
    }
}

TEST_CASE("float.optional", "[simdjson][decoder]") {
    using namespace cjm::metadata;

    for (const auto* name : {"float", "double"}) {
        DYNAMIC_SECTION(name) {
            FieldType inner;
            inner.kind = FieldTypeKind::FloatingPoint;
            inner.spelling = name;
            inner.qualified_name = name;

            FieldModel field;
            field.name = "ratio";
            field.json.name = "ratio";
            field.type.kind = FieldTypeKind::Optional;
            field.type.spelling = "std::optional<" + std::string(name) + ">";
            field.type.qualified_name = "std::optional";
            field.type.arguments = {inner};

            TypeModel type;
            type.name = "OptionalFloatValues";
            type.fields = {field};

            ProjectModel project;
            project.types = {type};
            const auto result =
                cjm::generator::simdjson::generate_header(project);
            INFO(result.error);
            REQUIRE(result.success);

            const auto& code = result.header;
            REQUIRE(code.find(std::string(name) + " decoded_ratio_value{};") !=
                    std::string::npos);
            REQUIRE(code.find("field.value().is_null()") != std::string::npos);
            REQUIRE(
                code.find("field.value().get_double().get(decoded_ratio)") !=
                std::string::npos);
            REQUIRE(code.find("decoded_ratio_value = "
                              "static_cast<target_type>(decoded_ratio);") !=
                    std::string::npos);
            REQUIRE(code.find("value.ratio = decoded_ratio_value;") !=
                    std::string::npos);
        }
    }
}

TEST_CASE("optional.nested_rejected", "[simdjson][decoder]") {
    using namespace cjm::metadata;

    const FieldType number{FieldTypeKind::SignedInteger, "int", "int"};
    const FieldType inner{FieldTypeKind::Optional,
                          "std::optional<int>",
                          "std::optional",
                          {number}};

    FieldModel field;
    field.name = "count";
    field.json.name = "value";
    field.type = FieldType{FieldTypeKind::Optional,
                           "std::optional<std::optional<int>>",
                           "std::optional",
                           {inner}};

    TypeModel model;
    model.name = "NestedOptional";
    model.fields = {field};
    ProjectModel project;
    project.types = {model};

    const auto result = cjm::generator::simdjson::generate_header(project);

    REQUIRE_FALSE(result.success);
    REQUIRE(result.header.empty());
    REQUIRE(result.error.find("NestedOptional") != std::string::npos);
    REQUIRE(result.error.find("field 'count'") != std::string::npos);
    REQUIRE(result.error.find("json field 'value'") != std::string::npos);
    REQUIRE(result.error.find(field.type.spelling) != std::string::npos);
    REQUIRE(result.error.find("directly nested optional") != std::string::npos);
}
