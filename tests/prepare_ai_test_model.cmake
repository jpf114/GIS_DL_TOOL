if(NOT DEFINED REPO_ROOT)
    message(FATAL_ERROR "REPO_ROOT is required")
endif()

set(SOURCE_MODEL "${REPO_ROOT}/scripts/test_e2e_data/test_seg_model.onnx")
set(TARGET_DIR "${REPO_ROOT}/test_data/models")
set(TARGET_MODEL "${TARGET_DIR}/test_seg_model.onnx")

file(MAKE_DIRECTORY "${TARGET_DIR}")

if(EXISTS "${SOURCE_MODEL}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${SOURCE_MODEL}" "${TARGET_MODEL}"
        RESULT_VARIABLE copy_result
    )
    if(NOT copy_result EQUAL 0)
        message(FATAL_ERROR "Failed to copy ONNX test model to ${TARGET_MODEL}")
    endif()
    message(STATUS "Prepared AI test model: ${TARGET_MODEL}")
else()
    message(WARNING "ONNX source model not found: ${SOURCE_MODEL}. AI integration tests may be skipped.")
endif()
