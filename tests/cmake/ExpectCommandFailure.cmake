if(NOT DEFINED CJM_EXPECT_FAILURE_COMMAND)
  message(FATAL_ERROR "CJM_EXPECT_FAILURE_COMMAND is required")
endif()

if(NOT DEFINED CJM_EXPECT_FAILURE_OUTPUT)
  message(FATAL_ERROR "CJM_EXPECT_FAILURE_OUTPUT is required")
endif()

execute_process(
  COMMAND ${CJM_EXPECT_FAILURE_COMMAND}
  RESULT_VARIABLE cjm_expect_failure_result
  OUTPUT_VARIABLE cjm_expect_failure_stdout
  ERROR_VARIABLE cjm_expect_failure_stderr
)

if(cjm_expect_failure_result EQUAL 0)
  message(FATAL_ERROR
    "Expected command to fail, but it exited successfully.\n"
    "stdout:\n${cjm_expect_failure_stdout}\n"
    "stderr:\n${cjm_expect_failure_stderr}"
  )
endif()

set(cjm_expect_failure_output
  "${cjm_expect_failure_stdout}${cjm_expect_failure_stderr}"
)
string(FIND
  "${cjm_expect_failure_output}"
  "${CJM_EXPECT_FAILURE_OUTPUT}"
  cjm_expect_failure_output_position
)

if(cjm_expect_failure_output_position EQUAL -1)
  message(FATAL_ERROR
    "Expected failure output to contain:\n"
    "${CJM_EXPECT_FAILURE_OUTPUT}\n"
    "stdout:\n${cjm_expect_failure_stdout}\n"
    "stderr:\n${cjm_expect_failure_stderr}"
  )
endif()
