set(_gui_candidates
    "${CMAKE_CURRENT_LIST_DIR}/../build/release/bin/Release/gis-ai-gui.exe"
    "${CMAKE_CURRENT_LIST_DIR}/../build/debug/bin/Debug/gis-ai-gui.exe"
)

set(GUI_PATH "")
foreach(candidate IN LISTS _gui_candidates)
    if(EXISTS "${candidate}")
        set(GUI_PATH "${candidate}")
        break()
    endif()
endforeach()

if(GUI_PATH STREQUAL "")
    message(FATAL_ERROR "GUI smoke test could not find gis-ai-gui.exe in build/release or build/debug")
endif()

set(SMOKE_DIR "${CMAKE_CURRENT_LIST_DIR}/../build/gui_smoke")
file(MAKE_DIRECTORY "${SMOKE_DIR}")

set(STATUS_FILE "${SMOKE_DIR}/status.json")
set(SCREENSHOT_FILE "${SMOKE_DIR}/smoke.png")
set(OUTPUT_TIF "${SMOKE_DIR}/smoke_output.tif")

file(REMOVE "${STATUS_FILE}" "${SCREENSHOT_FILE}" "${OUTPUT_TIF}")

execute_process(
    COMMAND "${GUI_PATH}"
        -platform offscreen
        --self-test
    WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/.."
    RESULT_VARIABLE GUI_SELF_TEST_EXIT_CODE
    TIMEOUT 30
)

if(NOT GUI_SELF_TEST_EXIT_CODE EQUAL 0)
    message(FATAL_ERROR "GUI self-test failed with exit code ${GUI_SELF_TEST_EXIT_CODE}")
endif()

execute_process(
    COMMAND "${GUI_PATH}"
        -platform offscreen
        --select-plugin segment
        --select-action segment_raster
        --set-param "model_path=test_data/models/test_seg_model.onnx"
        --set-param "input_raster=test_data/raster/test_100x100.tif"
        --set-param "output_tif=${OUTPUT_TIF}"
        --status-file "${STATUS_FILE}"
        --screenshot "${SCREENSHOT_FILE}"
        --auto-execute
    WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/.."
    RESULT_VARIABLE GUI_EXIT_CODE
    TIMEOUT 60
)

if(NOT GUI_EXIT_CODE EQUAL 0)
    message(FATAL_ERROR "GUI automation flow failed with exit code ${GUI_EXIT_CODE}")
endif()

if(NOT EXISTS "${STATUS_FILE}")
    message(FATAL_ERROR "GUI smoke test did not write status file: ${STATUS_FILE}")
endif()

if(NOT EXISTS "${SCREENSHOT_FILE}")
    message(FATAL_ERROR "GUI smoke test did not write screenshot: ${SCREENSHOT_FILE}")
endif()

file(READ "${STATUS_FILE}" STATUS_JSON)

string(FIND "${STATUS_JSON}" "\"success\": true" SUCCESS_POS)
if(SUCCESS_POS EQUAL -1)
    message(FATAL_ERROR "GUI smoke test status does not report success: ${STATUS_JSON}")
endif()

string(FIND "${STATUS_JSON}" "\"raw_message\"" RAW_MESSAGE_POS)
if(RAW_MESSAGE_POS EQUAL -1)
    message(FATAL_ERROR "GUI smoke test status does not contain raw_message: ${STATUS_JSON}")
endif()

if(NOT EXISTS "${OUTPUT_TIF}")
    message(FATAL_ERROR "GUI smoke test did not produce output raster: ${OUTPUT_TIF}")
endif()

message(STATUS "GUI smoke test passed with automation flow")
