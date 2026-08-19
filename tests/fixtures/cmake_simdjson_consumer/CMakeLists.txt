cmake_minimum_required(VERSION 3.20)

project(CJMSimdjsonConsumer LANGUAGES CXX)

if (NOT DEFINED CJM_SOURCE_DIR)
  message(FATAL_ERROR "CJM_SOURCE_DIR is required")
endif()

add_subdirectory("${CJM_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/_cjm")

add_executable(cjm_cmake_simdjson_consumer main.cpp)

target_compile_features(cjm_cmake_simdjson_consumer PRIVATE cxx_std_17)
target_include_directories(cjm_cmake_simdjson_consumer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

cjm_generate(
  TARGET cjm_cmake_simdjson_consumer
  HEADERS user.hpp
  JSON_BACKEND simdjson
  GENERATED_TARGET cjm_cmake_simdjson_consumer_generated
)
