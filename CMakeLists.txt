cmake_minimum_required(VERSION 3.20)

project(CJM VERSION 0.3.0 LANGUAGES CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CJMFetchContent.cmake)

find_package(nlohmann_json QUIET)

if (NOT nlohmann_json_FOUND)
  FetchContent_Declare(
    nlohmann_json 
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d
    DOWNLOAD_NO_PROGRESS FALSE
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_MakeAvailable(nlohmann_json)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(_CJM_BUILD_TESTS_DEFAULT ON)
else()
  set(_CJM_BUILD_TESTS_DEFAULT OFF)
endif()

option(CJM_BUILD_TESTS "Build CJM tests" ${_CJM_BUILD_TESTS_DEFAULT})
option(CJM_ENABLE_SIMDJSON "Enable the experimental simdjson backend dependency" OFF)

if (CJM_ENABLE_SIMDJSON)
  include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CJMSimdjson.cmake)
  cjm_add_simdjson_dependency()
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CJMGenerate.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CJMTreeSitter.cmake)
cjm_add_tree_sitter_frontend_dependencies()

add_library(cjm_tree_sitter_adapter
  src/frontends/cxx/tree_sitter/adapter.cpp
)
target_compile_features(cjm_tree_sitter_adapter PUBLIC cxx_std_17)
target_include_directories(cjm_tree_sitter_adapter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(cjm_tree_sitter_adapter PRIVATE cjm_tree_sitter_cpp_grammar)

# metadata library 
add_library(cjm_contract INTERFACE)
target_include_directories(cjm_contract INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(cjm_contract INTERFACE cxx_std_17)

add_library(cjm_contract_generator src/backends/contract/contract_generator.cpp)
target_link_libraries(cjm_contract_generator PUBLIC cjm_metadata cjm_contract)
target_include_directories(cjm_contract_generator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(cjm_contract_generator PUBLIC cxx_std_17)

add_library(cjm_schema_generator src/backends/schema/schema_generator.cpp)
target_link_libraries(cjm_schema_generator PUBLIC cjm_metadata)
target_include_directories(cjm_schema_generator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(cjm_schema_generator PUBLIC cxx_std_17)

# metadata library 
add_library(cjm_metadata INTERFACE)
target_include_directories(cjm_metadata INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(cjm_metadata INTERFACE cxx_std_17)

# generator library
add_library(cjm_generator src/backends/nlohmann/cpp_generator.cpp)
target_link_libraries(cjm_generator PUBLIC cjm_metadata)
target_include_directories(cjm_generator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(cjm_generator PUBLIC cxx_std_17)

# simdjson generator library
add_library(cjm_simdjson_generator src/backends/simdjson/cpp_generator.cpp)
target_link_libraries(cjm_simdjson_generator PUBLIC cjm_metadata)
target_include_directories(cjm_simdjson_generator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(cjm_simdjson_generator PUBLIC cxx_std_17)

# parser library
add_library(cjm_parser src/frontends/cxx/parser/parser.cpp)
target_include_directories(cjm_parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(cjm_parser PRIVATE cjm_tree_sitter_adapter)
target_compile_features(cjm_parser PUBLIC cxx_std_17)

# semantic library 
add_library(cjm_semantic src/frontends/cxx/semantic/analysis.cpp)
target_include_directories(cjm_semantic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(cjm_semantic PUBLIC cjm_metadata cjm_parser)
target_compile_features(cjm_semantic PUBLIC cxx_std_17)

# cjm main app
add_executable(cjm tools/cjm/main.cpp)
target_link_libraries(cjm PRIVATE cjm_generator cjm_simdjson_generator cjm_schema_generator cjm_semantic)

if (CJM_BUILD_TESTS)
  enable_testing()
  include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CJMCatch2.cmake)
  cjm_add_catch2_dependency()
  add_subdirectory(tests)
endif()
