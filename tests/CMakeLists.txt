list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
include(Catch)

add_executable(cjm_tree_sitter_adapter_tests
  ${PROJECT_SOURCE_DIR}/tests/frontends/cxx/tree_sitter/adapter_tests.cpp
)
target_compile_features(cjm_tree_sitter_adapter_tests PRIVATE cxx_std_17)
target_link_libraries(cjm_tree_sitter_adapter_tests PRIVATE cjm_tree_sitter_adapter)
add_test(NAME cjm_tree_sitter_adapter_tests COMMAND cjm_tree_sitter_adapter_tests)
set_tests_properties(
  cjm_tree_sitter_adapter_tests
  PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_executable(cjm_tree_sitter_dump
  ${PROJECT_SOURCE_DIR}/tools/tree_sitter_dump/main.cpp
)
target_compile_features(cjm_tree_sitter_dump PRIVATE cxx_std_17)
target_link_libraries(cjm_tree_sitter_dump PRIVATE cjm_tree_sitter_cpp_grammar)

if (CJM_ENABLE_SIMDJSON)
  add_executable(cjm_simdjson_dependency_smoke_tests
    ${PROJECT_SOURCE_DIR}/tests/backends/simdjson/dependency_smoke_tests.cpp
  )
  target_compile_features(cjm_simdjson_dependency_smoke_tests PRIVATE cxx_std_17)
  target_link_libraries(cjm_simdjson_dependency_smoke_tests PRIVATE simdjson::simdjson)
  add_test(NAME cjm_simdjson_dependency_smoke_tests COMMAND cjm_simdjson_dependency_smoke_tests)

  add_executable(cjm_simdjson_native_tests
    ${PROJECT_SOURCE_DIR}/tests/backends/simdjson/native_typed_baseline_tests.cpp
  )
  target_compile_features(cjm_simdjson_native_tests PRIVATE cxx_std_17)
  target_link_libraries(
    cjm_simdjson_native_tests
    PRIVATE simdjson::simdjson Catch2::Catch2WithMain
  )
  catch_discover_tests(
    cjm_simdjson_native_tests
    TEST_PREFIX cjm_simdjson_native_tests.
  )

  add_executable(cjm_simdjson_generator_tests
    ${PROJECT_SOURCE_DIR}/tests/backends/simdjson/cpp_generator_tests.cpp
  )
  target_link_libraries(cjm_simdjson_generator_tests PRIVATE cjm_simdjson_generator)
  add_test(NAME cjm_simdjson_generator_tests COMMAND cjm_simdjson_generator_tests)
  set_tests_properties(cjm_simdjson_generator_tests PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

  add_executable(cjm_simdjson_bool_generated_compile_tests
    ${PROJECT_SOURCE_DIR}/tests/backends/simdjson/bool_generated_compile_tests.cpp
  )
  target_compile_features(cjm_simdjson_bool_generated_compile_tests PRIVATE cxx_std_17)
  target_include_directories(cjm_simdjson_bool_generated_compile_tests PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(cjm_simdjson_bool_generated_compile_tests PRIVATE simdjson::simdjson)
  add_test(NAME cjm_simdjson_bool_generated_compile_tests COMMAND cjm_simdjson_bool_generated_compile_tests)

  add_executable(cjm_simdjson_integer_generated_compile_tests
    ${PROJECT_SOURCE_DIR}/tests/backends/simdjson/integer_generated_compile_tests.cpp
  )
  target_compile_features(cjm_simdjson_integer_generated_compile_tests PRIVATE cxx_std_17)
  target_include_directories(cjm_simdjson_integer_generated_compile_tests PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(cjm_simdjson_integer_generated_compile_tests PRIVATE simdjson::simdjson)
  add_test(NAME cjm_simdjson_integer_generated_compile_tests
           COMMAND cjm_simdjson_integer_generated_compile_tests)

  add_executable(cjm_simdjson_string_generated_compile_tests
    ${PROJECT_SOURCE_DIR}/tests/backends/simdjson/string_generated_compile_tests.cpp
  )
  target_compile_features(cjm_simdjson_string_generated_compile_tests PRIVATE cxx_std_17)
  target_include_directories(cjm_simdjson_string_generated_compile_tests PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(cjm_simdjson_string_generated_compile_tests PRIVATE simdjson::simdjson)
  add_test(NAME cjm_simdjson_string_generated_compile_tests
           COMMAND cjm_simdjson_string_generated_compile_tests)

  add_executable(cjm_simdjson_vector_generated_compile_tests
    ${PROJECT_SOURCE_DIR}/tests/backends/simdjson/vector_generated_compile_tests.cpp
  )
  target_compile_features(cjm_simdjson_vector_generated_compile_tests PRIVATE cxx_std_17)
  target_include_directories(cjm_simdjson_vector_generated_compile_tests PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(cjm_simdjson_vector_generated_compile_tests PRIVATE simdjson::simdjson)
  add_test(NAME cjm_simdjson_vector_generated_compile_tests
           COMMAND cjm_simdjson_vector_generated_compile_tests)

  add_executable(cjm_simdjson_vector_user_defined_generated_compile_tests
    ${PROJECT_SOURCE_DIR}/tests/backends/simdjson/vector_user_defined_generated_compile_tests.cpp
  )
  target_compile_features(cjm_simdjson_vector_user_defined_generated_compile_tests PRIVATE cxx_std_17)
  target_include_directories(cjm_simdjson_vector_user_defined_generated_compile_tests PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(cjm_simdjson_vector_user_defined_generated_compile_tests PRIVATE simdjson::simdjson)
  add_test(NAME cjm_simdjson_vector_user_defined_generated_compile_tests
           COMMAND cjm_simdjson_vector_user_defined_generated_compile_tests)

  add_executable(cjm_simdjson_map_generated_compile_tests
    ${PROJECT_SOURCE_DIR}/tests/backends/simdjson/map_generated_compile_tests.cpp
  )
  target_compile_features(cjm_simdjson_map_generated_compile_tests PRIVATE cxx_std_17)
  target_include_directories(cjm_simdjson_map_generated_compile_tests PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(cjm_simdjson_map_generated_compile_tests PRIVATE simdjson::simdjson)
  add_test(NAME cjm_simdjson_map_generated_compile_tests
           COMMAND cjm_simdjson_map_generated_compile_tests)

  add_executable(cjm_simdjson_map_user_defined_generated_compile_tests
    ${PROJECT_SOURCE_DIR}/tests/backends/simdjson/map_user_defined_generated_compile_tests.cpp
  )
  target_compile_features(cjm_simdjson_map_user_defined_generated_compile_tests PRIVATE cxx_std_17)
  target_include_directories(cjm_simdjson_map_user_defined_generated_compile_tests PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(cjm_simdjson_map_user_defined_generated_compile_tests PRIVATE simdjson::simdjson)
  add_test(NAME cjm_simdjson_map_user_defined_generated_compile_tests
           COMMAND cjm_simdjson_map_user_defined_generated_compile_tests)

  add_executable(cjm_simdjson_map_vector_generated_compile_tests
    ${PROJECT_SOURCE_DIR}/tests/backends/simdjson/map_vector_generated_compile_tests.cpp
  )
  target_compile_features(cjm_simdjson_map_vector_generated_compile_tests PRIVATE cxx_std_17)
  target_include_directories(cjm_simdjson_map_vector_generated_compile_tests PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(cjm_simdjson_map_vector_generated_compile_tests PRIVATE simdjson::simdjson)
  add_test(NAME cjm_simdjson_map_vector_generated_compile_tests
           COMMAND cjm_simdjson_map_vector_generated_compile_tests)

  add_executable(cjm_simdjson_array_generated_compile_tests
    ${PROJECT_SOURCE_DIR}/tests/backends/simdjson/array_generated_compile_tests.cpp
  )
  target_compile_features(cjm_simdjson_array_generated_compile_tests PRIVATE cxx_std_17)
  target_include_directories(cjm_simdjson_array_generated_compile_tests PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(cjm_simdjson_array_generated_compile_tests PRIVATE simdjson::simdjson)
  add_test(NAME cjm_simdjson_array_generated_compile_tests
             COMMAND cjm_simdjson_array_generated_compile_tests)


  add_executable(cjm_simdjson_enum_generated_compile_tests
    ${PROJECT_SOURCE_DIR}/tests/backends/simdjson/enum_generated_compile_tests.cpp
  )
  target_compile_features(cjm_simdjson_enum_generated_compile_tests PRIVATE cxx_std_17)
  target_include_directories(cjm_simdjson_enum_generated_compile_tests PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(cjm_simdjson_enum_generated_compile_tests PRIVATE simdjson::simdjson)
  add_test(NAME cjm_simdjson_enum_generated_compile_tests
           COMMAND cjm_simdjson_enum_generated_compile_tests)

  add_executable(cjm_simdjson_optional_integer_generated_compile_tests
    ${PROJECT_SOURCE_DIR}/tests/backends/simdjson/optional_integer_generated_compile_tests.cpp
  )
  target_compile_features(cjm_simdjson_optional_integer_generated_compile_tests PRIVATE cxx_std_17)
  target_include_directories(cjm_simdjson_optional_integer_generated_compile_tests PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(cjm_simdjson_optional_integer_generated_compile_tests PRIVATE simdjson::simdjson)
  add_test(NAME cjm_simdjson_optional_integer_generated_compile_tests
           COMMAND cjm_simdjson_optional_integer_generated_compile_tests)

  add_executable(cjm_simdjson_optional_vector_generated_compile_tests
    ${PROJECT_SOURCE_DIR}/tests/backends/simdjson/optional_vector_generated_compile_tests.cpp
  )
  target_compile_features(cjm_simdjson_optional_vector_generated_compile_tests PRIVATE cxx_std_17)
  target_include_directories(cjm_simdjson_optional_vector_generated_compile_tests PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(cjm_simdjson_optional_vector_generated_compile_tests PRIVATE simdjson::simdjson)
  add_test(NAME cjm_simdjson_optional_vector_generated_compile_tests
           COMMAND cjm_simdjson_optional_vector_generated_compile_tests)

  add_executable(cjm_simdjson_nested_generated_compile_tests
    ${PROJECT_SOURCE_DIR}/tests/backends/simdjson/nested_generated_compile_tests.cpp
  )
  target_compile_features(cjm_simdjson_nested_generated_compile_tests PRIVATE cxx_std_17)
  target_include_directories(cjm_simdjson_nested_generated_compile_tests PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(cjm_simdjson_nested_generated_compile_tests PRIVATE simdjson::simdjson)
  add_test(NAME cjm_simdjson_nested_generated_compile_tests
           COMMAND cjm_simdjson_nested_generated_compile_tests)

  add_executable(cjm_simdjson_vertical_slice_generated_compile_tests
    ${PROJECT_SOURCE_DIR}/tests/backends/simdjson/vertical_slice_generated_compile_tests.cpp
  )
  target_compile_features(cjm_simdjson_vertical_slice_generated_compile_tests PRIVATE cxx_std_17)
  target_include_directories(cjm_simdjson_vertical_slice_generated_compile_tests PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(cjm_simdjson_vertical_slice_generated_compile_tests PRIVATE simdjson::simdjson)
  add_test(NAME cjm_simdjson_vertical_slice_generated_compile_tests.success
           COMMAND cjm_simdjson_vertical_slice_generated_compile_tests success)
  add_test(NAME cjm_simdjson_vertical_slice_generated_compile_tests.optional_type_mismatch
           COMMAND cjm_simdjson_vertical_slice_generated_compile_tests optional_type_mismatch)
  add_test(NAME cjm_simdjson_vertical_slice_generated_compile_tests.nested_non_object
           COMMAND cjm_simdjson_vertical_slice_generated_compile_tests nested_non_object)
  add_test(NAME cjm_simdjson_vertical_slice_generated_compile_tests.nested_child_missing
           COMMAND cjm_simdjson_vertical_slice_generated_compile_tests nested_child_missing)
endif()

# METADATA tests below...
add_executable(cjm_metadata_tests
  ${PROJECT_SOURCE_DIR}/tests/core/ir/model_tests.cpp
)
target_link_libraries(cjm_metadata_tests PRIVATE cjm_metadata)
add_test(NAME cjm_metadata_tests COMMAND cjm_metadata_tests)


# GENERATOR tests below...
add_executable(cjm_generator_tests ${PROJECT_SOURCE_DIR}/tests/backends/nlohmann/cpp_generator_tests.cpp)
target_link_libraries(cjm_generator_tests PRIVATE cjm_generator)
add_test(NAME cjm_generator_tests COMMAND cjm_generator_tests)
set_tests_properties(cjm_generator_tests PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

add_executable(cjm_contract_generator_tests
  ${PROJECT_SOURCE_DIR}/tests/backends/contract/contract_generator_tests.cpp
)
target_link_libraries(cjm_contract_generator_tests PRIVATE cjm_contract_generator)
add_test(NAME cjm_contract_generator_tests COMMAND cjm_contract_generator_tests)
set_tests_properties(cjm_contract_generator_tests PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

add_executable(cjm_contract_generated_compile_tests
  ${PROJECT_SOURCE_DIR}/tests/backends/contract/contract_generated_compile_tests.cpp
)
target_compile_features(cjm_contract_generated_compile_tests PRIVATE cxx_std_17)
target_include_directories(cjm_contract_generated_compile_tests PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(cjm_contract_generated_compile_tests PRIVATE cjm_contract)
add_test(NAME cjm_contract_generated_compile_tests COMMAND cjm_contract_generated_compile_tests)

add_executable(cjm_recursive_contract_generated_compile_tests
  ${PROJECT_SOURCE_DIR}/tests/backends/contract/recursive_contract_generated_compile_tests.cpp
)
target_compile_features(cjm_recursive_contract_generated_compile_tests PRIVATE cxx_std_17)
target_include_directories(cjm_recursive_contract_generated_compile_tests PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(cjm_recursive_contract_generated_compile_tests PRIVATE cjm_contract)
add_test(NAME cjm_recursive_contract_generated_compile_tests COMMAND cjm_recursive_contract_generated_compile_tests)

add_executable(cjm_schema_generator_tests
  ${PROJECT_SOURCE_DIR}/tests/backends/schema/schema_generator_tests.cpp
)
target_link_libraries(cjm_schema_generator_tests PRIVATE cjm_schema_generator)
add_test(NAME cjm_schema_generator_tests COMMAND cjm_schema_generator_tests)
set_tests_properties(cjm_schema_generator_tests PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})


# PIPELINE GOLDEN tests below...
add_executable(cjm_pipeline_golden_tests
  ${PROJECT_SOURCE_DIR}/tests/backends/nlohmann/pipeline_golden_tests.cpp
)
target_link_libraries(cjm_pipeline_golden_tests PRIVATE cjm_generator cjm_parser cjm_semantic)
add_test(NAME cjm_pipeline_golden_tests COMMAND cjm_pipeline_golden_tests)
set_tests_properties(cjm_pipeline_golden_tests PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

# CLI tests below...
add_test(NAME cjm_cli_help COMMAND cjm --help)
add_test(NAME cjm_cli_generate
  COMMAND cjm generate
  --input ${PROJECT_SOURCE_DIR}/examples/basic/user.hpp
  --output ${PROJECT_BINARY_DIR}/user.cjm.hpp
)
set_tests_properties(
  cjm_cli_generate
  PROPERTIES
    PASS_REGULAR_EXPRESSION "cjm: generated"
)

add_test(NAME cjm_cli_generate_explicit_nlohmann
  COMMAND cjm generate
  --backend nlohmann
  --input ${PROJECT_SOURCE_DIR}/examples/basic/user.hpp
  --output ${PROJECT_BINARY_DIR}/user.nlohmann.cjm.hpp
)
set_tests_properties(
  cjm_cli_generate_explicit_nlohmann
  PROPERTIES
    PASS_REGULAR_EXPRESSION "cjm: generated"
)

add_test(NAME cjm_cli_generate_explicit_nlohmann_golden
  COMMAND ${CMAKE_COMMAND} -E compare_files
          ${PROJECT_BINARY_DIR}/user.nlohmann.cjm.hpp
          ${PROJECT_SOURCE_DIR}/tests/golden/basic_pipeline.expected.cjm.hpp
)
set_tests_properties(
  cjm_cli_generate_explicit_nlohmann_golden
  PROPERTIES
    DEPENDS cjm_cli_generate_explicit_nlohmann
)

add_test(NAME cjm_cli_generate_explicit_simdjson
  COMMAND cjm generate
  --backend simdjson
  --input ${PROJECT_SOURCE_DIR}/examples/basic/user.hpp
  --output ${PROJECT_BINARY_DIR}/user.simdjson.cjm.hpp
)
set_tests_properties(
  cjm_cli_generate_explicit_simdjson
  PROPERTIES
    PASS_REGULAR_EXPRESSION "cjm: generated"
)

add_test(NAME cjm_cli_generate_explicit_simdjson_golden
  COMMAND ${CMAKE_COMMAND} -E compare_files
          ${PROJECT_BINARY_DIR}/user.simdjson.cjm.hpp
          ${PROJECT_SOURCE_DIR}/tests/golden/basic_simdjson_cli.expected.cjm.hpp
)
set_tests_properties(
  cjm_cli_generate_explicit_simdjson_golden
  PROPERTIES
    DEPENDS cjm_cli_generate_explicit_simdjson
)

add_test(NAME cjm_cli_unknown_command
  COMMAND cjm unknown
)
set_tests_properties(
  cjm_cli_unknown_command
  PROPERTIES
    WILL_FAIL TRUE
)

add_test(NAME cjm_cli_generate_unknown_backend
  COMMAND ${CMAKE_COMMAND}
  -D "CJM_EXPECT_FAILURE_COMMAND=$<TARGET_FILE:cjm>;generate;--backend;nope;--input;${PROJECT_SOURCE_DIR}/examples/basic/user.hpp;--output;${PROJECT_BINARY_DIR}/unknown-backend.cjm.hpp"
  -D "CJM_EXPECT_FAILURE_OUTPUT=unknown JSON backend"
  -P ${PROJECT_SOURCE_DIR}/tests/cmake/ExpectCommandFailure.cmake
)

add_test(NAME cjm_cli_generate_backend_missing_value
  COMMAND ${CMAKE_COMMAND}
  -D "CJM_EXPECT_FAILURE_COMMAND=$<TARGET_FILE:cjm>;generate;--input;${PROJECT_SOURCE_DIR}/examples/basic/user.hpp;--output;${PROJECT_BINARY_DIR}/missing-backend-value.cjm.hpp;--backend"
  -D "CJM_EXPECT_FAILURE_OUTPUT=--backend requires a value"
  -P ${PROJECT_SOURCE_DIR}/tests/cmake/ExpectCommandFailure.cmake
)

add_test(NAME cjm_cli_generate_schema_rejects_backend
  COMMAND ${CMAKE_COMMAND}
  -D "CJM_EXPECT_FAILURE_COMMAND=$<TARGET_FILE:cjm>;generate-schema;--backend;simdjson;--input;${PROJECT_SOURCE_DIR}/examples/basic/user.hpp;--output;${PROJECT_BINARY_DIR}/schema-backend.schema.json"
  -D "CJM_EXPECT_FAILURE_OUTPUT=--backend is only valid for generate"
  -P ${PROJECT_SOURCE_DIR}/tests/cmake/ExpectCommandFailure.cmake
)

add_test(NAME cjm_cli_generate_missing_args
  COMMAND cjm generate
)
set_tests_properties(
  cjm_cli_generate_missing_args
  PROPERTIES
    WILL_FAIL TRUE
)

add_subdirectory(${PROJECT_SOURCE_DIR}/examples/basic ${PROJECT_BINARY_DIR}/examples/basic)

add_test(
  NAME cjm_basic_example
  COMMAND cjm_basic_example
)

add_test(
  NAME cjm_basic_downstream_consumer
  COMMAND cjm_basic_downstream_consumer
)

add_test(
  NAME cjm_basic_example_generated_header
  COMMAND ${CMAKE_COMMAND} -E compare_files
          ${PROJECT_BINARY_DIR}/examples/basic/generated/cjm/nlohmann/user.cjm.hpp
          ${PROJECT_SOURCE_DIR}/tests/golden/basic_pipeline.expected.cjm.hpp
)
# parser tests below...
add_executable(cjm_parser_tests ${PROJECT_SOURCE_DIR}/tests/frontends/cxx/parser/syntax_tests.cpp)
target_link_libraries(cjm_parser_tests PRIVATE cjm_parser)
add_test(NAME cjm_parser_tests COMMAND cjm_parser_tests)
set_tests_properties(cjm_parser_tests PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

add_executable(cjm_parser_file_tests ${PROJECT_SOURCE_DIR}/tests/frontends/cxx/parser/parser_tests.cpp)
target_link_libraries(cjm_parser_file_tests PRIVATE cjm_parser)
add_test(NAME cjm_parser_file_tests COMMAND cjm_parser_file_tests)
set_tests_properties(cjm_parser_file_tests PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

add_executable(cjm_tree_sitter_smoke_tests
  ${PROJECT_SOURCE_DIR}/tests/frontends/cxx/tree_sitter/build_smoke_tests.cpp
)
target_compile_features(cjm_tree_sitter_smoke_tests PRIVATE cxx_std_17)
target_link_libraries(cjm_tree_sitter_smoke_tests PRIVATE cjm_tree_sitter_cpp_grammar)
add_test(NAME cjm_tree_sitter_smoke_tests COMMAND cjm_tree_sitter_smoke_tests)

# semantic tests below...
add_executable(cjm_semantic_tests ${PROJECT_SOURCE_DIR}/tests/frontends/cxx/semantic/analysis_tests.cpp)
target_link_libraries(cjm_semantic_tests PRIVATE cjm_semantic)
add_test(NAME cjm_semantic_tests COMMAND cjm_semantic_tests)
set_tests_properties(cjm_semantic_tests PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

# practical generated compile tests below...
add_executable(cjm_practical_generated_compile_tests
  ${PROJECT_SOURCE_DIR}/tests/backends/nlohmann/practical_generated_compile_tests.cpp
)
target_compile_features(cjm_practical_generated_compile_tests PRIVATE cxx_std_17)
target_include_directories(cjm_practical_generated_compile_tests PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(cjm_practical_generated_compile_tests PRIVATE nlohmann_json::nlohmann_json cjm_contract)
add_test(NAME cjm_practical_generated_compile_tests COMMAND cjm_practical_generated_compile_tests)

add_executable(cjm_map_generated_compile_tests
  ${PROJECT_SOURCE_DIR}/tests/backends/nlohmann/map_generated_compile_tests.cpp
)
target_compile_features(cjm_map_generated_compile_tests PRIVATE cxx_std_17)
target_include_directories(cjm_map_generated_compile_tests PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(cjm_map_generated_compile_tests PRIVATE nlohmann_json::nlohmann_json cjm_contract)
add_test(NAME cjm_map_generated_compile_tests COMMAND cjm_map_generated_compile_tests)

add_executable(cjm_recursive_generated_compile_tests
  ${PROJECT_SOURCE_DIR}/tests/backends/nlohmann/recursive_generated_compile_tests.cpp
)
target_compile_features(cjm_recursive_generated_compile_tests PRIVATE cxx_std_17)
target_include_directories(cjm_recursive_generated_compile_tests PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(cjm_recursive_generated_compile_tests PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME cjm_recursive_generated_compile_tests COMMAND cjm_recursive_generated_compile_tests)

add_executable(cjm_fixed_width_generated_compile_tests
  ${PROJECT_SOURCE_DIR}/tests/backends/nlohmann/fixed_width_generated_compile_tests.cpp
)
target_compile_features(cjm_fixed_width_generated_compile_tests PRIVATE cxx_std_17)
target_link_libraries(cjm_fixed_width_generated_compile_tests PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME cjm_fixed_width_generated_compile_tests COMMAND cjm_fixed_width_generated_compile_tests)

add_test(NAME cjm_cli_generate_multiple_inputs
  COMMAND cjm generate
  --input ${PROJECT_SOURCE_DIR}/tests/fixtures/cli_multi/address.hpp
  --input ${PROJECT_SOURCE_DIR}/tests/fixtures/cli_multi/user.hpp
  --output ${PROJECT_BINARY_DIR}/cli_multi.cjm.hpp
)
set_tests_properties(
  cjm_cli_generate_multiple_inputs
  PROPERTIES
    PASS_REGULAR_EXPRESSION "cjm: generated"
)

add_test(NAME cjm_cli_generate_multiple_inputs_golden
  COMMAND ${CMAKE_COMMAND} -E compare_files
  ${PROJECT_BINARY_DIR}/cli_multi.cjm.hpp
  ${PROJECT_SOURCE_DIR}/tests/golden/cli_multi.expected.cjm.hpp
)
set_tests_properties(
  cjm_cli_generate_multiple_inputs_golden
  PROPERTIES
    DEPENDS cjm_cli_generate_multiple_inputs
)

add_test(NAME cjm_cli_generate_grouped_inputs
  COMMAND cjm generate
  --input
    ${PROJECT_SOURCE_DIR}/tests/fixtures/cli_multi/address.hpp
    ${PROJECT_SOURCE_DIR}/tests/fixtures/cli_multi/user.hpp
  --output ${PROJECT_BINARY_DIR}/cli_multi_grouped.cjm.hpp
)
set_tests_properties(
  cjm_cli_generate_grouped_inputs
  PROPERTIES
    PASS_REGULAR_EXPRESSION "cjm: generated"
)

add_test(NAME cjm_cli_generate_grouped_inputs_golden
  COMMAND ${CMAKE_COMMAND} -E compare_files
  ${PROJECT_BINARY_DIR}/cli_multi_grouped.cjm.hpp
  ${PROJECT_SOURCE_DIR}/tests/golden/cli_multi.expected.cjm.hpp
)
set_tests_properties(
  cjm_cli_generate_grouped_inputs_golden
  PROPERTIES
    DEPENDS cjm_cli_generate_grouped_inputs
)

add_test(NAME cjm_cli_generate_v03_practical
  COMMAND cjm generate
  --input ${PROJECT_SOURCE_DIR}/tests/fixtures/v03_practical_models.hpp
  --output ${PROJECT_BINARY_DIR}/v03_practical.cjm.hpp
)
set_tests_properties(
  cjm_cli_generate_v03_practical
  PROPERTIES
    PASS_REGULAR_EXPRESSION "cjm: generated"
)

add_test(NAME cjm_cli_generate_v03_practical_golden
  COMMAND ${CMAKE_COMMAND} -E compare_files
  ${PROJECT_BINARY_DIR}/v03_practical.cjm.hpp
  ${PROJECT_SOURCE_DIR}/tests/golden/v03_practical.expected.cjm.hpp
)
set_tests_properties(
  cjm_cli_generate_v03_practical_golden
  PROPERTIES
    DEPENDS cjm_cli_generate_v03_practical
)

add_executable(cjm_model_contract_tests ${PROJECT_SOURCE_DIR}/tests/core/model_contract_tests.cpp)
target_link_libraries(cjm_model_contract_tests PRIVATE cjm_contract)
add_test(NAME cjm_model_contract_tests COMMAND cjm_model_contract_tests)

add_test(NAME cjm_cli_generate_schema
  COMMAND cjm generate-schema
  --input ${PROJECT_SOURCE_DIR}/examples/basic/user.hpp
  --output ${PROJECT_BINARY_DIR}/user.schema.json
)
set_tests_properties(
  cjm_cli_generate_schema
  PROPERTIES
    PASS_REGULAR_EXPRESSION "cjm: generated"
)

add_test(
  NAME cjm_basic_example_generated_schemas
  COMMAND ${CMAKE_COMMAND} -E compare_files
          ${PROJECT_BINARY_DIR}/examples/basic/generated/schemas/user.schema.json
          ${PROJECT_SOURCE_DIR}/tests/golden/basic_user.expected.schema.json
)

add_test(
  NAME cjm_cmake_simdjson_backend_configure
  COMMAND ${CMAKE_COMMAND}
  -S ${PROJECT_SOURCE_DIR}/tests/fixtures/cmake_simdjson_consumer
  -B ${PROJECT_BINARY_DIR}/cmake_simdjson_consumer
  -D CJM_SOURCE_DIR=${PROJECT_SOURCE_DIR}
)

add_test(NAME cjm_cmake_simdjson_backend_build
  COMMAND ${CMAKE_COMMAND}
  --build ${PROJECT_BINARY_DIR}/cmake_simdjson_consumer
  --target cjm_cmake_simdjson_consumer
)
set_tests_properties(
  cjm_cmake_simdjson_backend_build
  PROPERTIES
    DEPENDS cjm_cmake_simdjson_backend_configure
)

add_test(NAME cjm_cmake_simdjson_backend_run
  COMMAND ${PROJECT_BINARY_DIR}/cmake_simdjson_consumer/cjm_cmake_simdjson_consumer
)
set_tests_properties(
  cjm_cmake_simdjson_backend_run
  PROPERTIES
    DEPENDS cjm_cmake_simdjson_backend_build
)

add_test(NAME cjm_cmake_simdjson_backend_golden
  COMMAND ${CMAKE_COMMAND} -E compare_files
          ${PROJECT_BINARY_DIR}/cmake_simdjson_consumer/generated/cjm/simdjson/user.cjm.hpp
          ${PROJECT_SOURCE_DIR}/tests/golden/basic_simdjson_cli.expected.cjm.hpp
)
set_tests_properties(
  cjm_cmake_simdjson_backend_golden
  PROPERTIES
    DEPENDS cjm_cmake_simdjson_backend_build
)

add_executable(cjm_metadata_named_tests ${PROJECT_SOURCE_DIR}/tests/core/ir/model_named_tests.cpp)
target_link_libraries(cjm_metadata_named_tests PRIVATE cjm_metadata Catch2::Catch2WithMain)
catch_discover_tests(cjm_metadata_named_tests TEST_PREFIX cjm_metadata_named_tests.)

add_executable(cjm_test_support_named_tests ${PROJECT_SOURCE_DIR}/tests/support/golden_diff_tests.cpp)
target_link_libraries(cjm_test_support_named_tests PRIVATE Catch2::Catch2WithMain)
target_include_directories(cjm_test_support_named_tests PRIVATE ${PROJECT_SOURCE_DIR}/tests)
catch_discover_tests(cjm_test_support_named_tests TEST_PREFIX cjm_test_support_named_tests.)
