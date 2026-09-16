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
        {"optional", {FieldTypeKind::Optional, "std::optional<double>",
                      "std::optional", {number}}},
        {"vector", {FieldTypeKind::Vector, "std::vector<double>",
                    "std::vector", {number}}},
        {"array", {FieldTypeKind::Array, "std::array<double, 2>",
                   "std::array", {number}, 2}},
        {"map", {FieldTypeKind::Map, "std::map<std::string, double>",
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

            const auto result = cjm::generator::simdjson::generate_header(project);
            REQUIRE_FALSE(result.success);
            REQUIRE(result.header.empty());
            REQUIRE(result.error.find("UnsupportedValues") != std::string::npos);
            REQUIRE(result.error.find("field 'ratio'") != std::string::npos);
            REQUIRE(result.error.find("json field 'value'") != std::string::npos);
            REQUIRE(result.error.find(item.type.spelling) != std::string::npos);
        }
    }
}
