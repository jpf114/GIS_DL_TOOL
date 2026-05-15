if(NOT DEFINED REPO_ROOT)
    message(FATAL_ERROR "REPO_ROOT is required")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/gui_regression_helpers.cmake")

set(INSTALL_ROOT "${REPO_ROOT}/install")
set(GUI_PATH "${INSTALL_ROOT}/bin/gis-ai-gui.exe")
set(INPUT_PATH "${REPO_ROOT}/test_data/vector/test_polygons.shp")
set(OUTPUT_DIR "${REPO_ROOT}/build/gui_vector_regression")
set(OUTPUT_PATH "${OUTPUT_DIR}/simplify_output.gpkg")
set(SCREENSHOT_PATH "${OUTPUT_DIR}/vector_simplify.png")
set(STATUS_PATH "${OUTPUT_DIR}/status.json")

if(NOT EXISTS "${GUI_PATH}")
    message(FATAL_ERROR "Installed GUI not found: ${GUI_PATH}")
endif()
if(NOT EXISTS "${INPUT_PATH}")
    message(FATAL_ERROR "Installed GUI regression vector input not found: ${INPUT_PATH}")
endif()

gis_gui_prepare_artifact_paths("${OUTPUT_PATH}" "${SCREENSHOT_PATH}" "${STATUS_PATH}")

execute_process(
    COMMAND "${GUI_PATH}"
        -platform offscreen
        --select-plugin vector
        --select-action vector_simplify
        --set-param "input_vector=${INPUT_PATH}"
        --set-param "output_path=${OUTPUT_PATH}"
        --auto-execute
        --quit-on-finish
        --screenshot "${SCREENSHOT_PATH}"
        --status-file "${STATUS_PATH}"
    WORKING_DIRECTORY "${REPO_ROOT}"
    RESULT_VARIABLE GUI_EXIT_CODE
    OUTPUT_VARIABLE GUI_STDOUT
    ERROR_VARIABLE GUI_STDERR
    TIMEOUT 120
)

if(NOT "${GUI_EXIT_CODE}" STREQUAL "0")
    message(FATAL_ERROR
        "Installed GUI vector regression failed with exit code ${GUI_EXIT_CODE}\n"
        "stdout:\n${GUI_STDOUT}\n"
        "stderr:\n${GUI_STDERR}")
endif()

if(NOT EXISTS "${OUTPUT_PATH}")
    message(FATAL_ERROR "Installed GUI vector regression did not produce output: ${OUTPUT_PATH}")
endif()

file(SIZE "${OUTPUT_PATH}" OUTPUT_SIZE)
if(OUTPUT_SIZE EQUAL 0)
    message(FATAL_ERROR "Installed GUI vector regression produced an empty output file: ${OUTPUT_PATH}")
endif()

if(NOT EXISTS "${SCREENSHOT_PATH}")
    message(FATAL_ERROR "Installed GUI vector regression did not produce screenshot: ${SCREENSHOT_PATH}")
endif()

file(SIZE "${SCREENSHOT_PATH}" SCREENSHOT_SIZE)
if(SCREENSHOT_SIZE EQUAL 0)
    message(FATAL_ERROR "Installed GUI vector regression produced an empty screenshot: ${SCREENSHOT_PATH}")
endif()

gis_gui_read_status_file("Installed GUI vector regression" "${STATUS_PATH}"
    STATUS_SUCCESS STATUS_CANCELLED STATUS_MESSAGE STATUS_RAW_MESSAGE)

gis_gui_assert_status_bool("${STATUS_SUCCESS}" "true"
    "Installed GUI vector regression reported unsuccessful status")
gis_gui_assert_status_bool("${STATUS_CANCELLED}" "false"
    "Installed GUI vector regression unexpectedly reported cancellation")
