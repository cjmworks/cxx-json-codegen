#include "backends/simdjson/cpp_generator.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>

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
