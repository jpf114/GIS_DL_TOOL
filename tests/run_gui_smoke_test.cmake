set(GUI_PATH "${CMAKE_CURRENT_LIST_DIR}/../build/debug/bin/Debug/gis-ai-gui.exe")
execute_process(
    COMMAND "${GUI_PATH}" -platform offscreen --self-test
    RESULT_VARIABLE GUI_EXIT_CODE
    TIMEOUT 30
)
if(NOT GUI_EXIT_CODE EQUAL 0)
    message(FATAL_ERROR "GUI smoke test failed with exit code ${GUI_EXIT_CODE}")
endif()
message(STATUS "GUI smoke test passed")
