include_guard(GLOBAL)

# Register one simdjson runtime test, optionally generating its model codec.
function(cjm_add_simdjson_test target) 
  cmake_parse_arguments(PARSE_ARGV 1 CJM_TEST "" "SOURCE;MODEL" "")

  if (CJM_TEST_UNPARSED_ARGUMENTS OR CJM_TEST_KEYWORDS_MISSING_VALUES)
    message(FATAL_ERROR "Invalid arguments for ${target}")
  endif()
  if (NOT CJM_TEST_SOURCE)
    message(FATAL_ERROR "${target} requires SOURCE")
  endif()

  add_executable(${target} "${PROJECT_SOURCE_DIR}/tests/${CJM_TEST_SOURCE}")
  target_compile_features(${target} PRIVATE cxx_std_17)
  target_include_directories(${target} PRIVATE "${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/tests")
  target_link_libraries(${target} PRIVATE simdjson::simdjson Catch2::Catch2WithMain)

  if (CJM_TEST_MODEL) 
    cjm_generate(
      TARGET ${target}
      JSON_BACKEND simdjson
      HEADERS "${PROJECT_SOURCE_DIR}/tests/${CJM_TEST_MODEL}"
    )
  endif()

  catch_discover_tests(${target} TEST_PREFIX "${target}.")
endfunction()

