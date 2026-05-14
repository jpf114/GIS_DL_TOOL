set(GIS_TOOL_SOURCE_DIR "" CACHE PATH "Path to GIS_TOOL source tree for cross-tool alignment checks")

if(NOT GIS_TOOL_SOURCE_DIR)
    message(STATUS "GIS_TOOL_SOURCE_DIR not set, skipping cross-tool alignment check")
    return()
endif()

if(NOT EXISTS "${GIS_TOOL_SOURCE_DIR}/src/gui/gui_data_support.h")
    message(WARNING "GIS_TOOL_SOURCE_DIR does not appear to be a valid GIS_TOOL source tree")
    return()
endif()

set(_alignment_errors "")
set(_alignment_warnings "")

set(_shared_gui_files
    "gui_data_support.h"
    "gui_data_support.cpp"
    "mainwindow.h"
    "mainwindow.cpp"
    "task_runner.h"
    "task_runner.cpp"
    "execute_worker.h"
    "execute_worker.cpp"
    "task_manager.h"
    "task_manager.cpp"
    "task_database.h"
    "task_database.cpp"
    "nav_panel.h"
    "nav_panel.cpp"
    "param_card_widget.h"
    "param_card_widget.cpp"
    "param_widget.h"
    "param_widget.cpp"
)

foreach(_file ${_shared_gui_files})
    set(_dl_file "${CMAKE_SOURCE_DIR}/src/gui/${_file}")
    set(_tool_file "${GIS_TOOL_SOURCE_DIR}/src/gui/${_file}")

    if(NOT EXISTS "${_dl_file}")
        list(APPEND _alignment_errors "MISSING in GIS_DL_TOOL: src/gui/${_file}")
    endif()

    if(NOT EXISTS "${_tool_file}")
        list(APPEND _alignment_errors "MISSING in GIS_TOOL: src/gui/${_file}")
    endif()
endforeach()

set(_allowed_tool_only_files
    "custom_index_preset_store.h"
    "custom_index_preset_store.cpp"
)

set(_dl_gui_files)
file(GLOB _dl_gui_glob LIST_DIRECTORY false "${CMAKE_SOURCE_DIR}/src/gui/*.h" "${CMAKE_SOURCE_DIR}/src/gui/*.cpp")
foreach(_f ${_dl_gui_glob})
    get_filename_component(_name "${_f}" NAME)
    list(APPEND _dl_gui_files "${_name}")
endforeach()

set(_tool_gui_files)
file(GLOB _tool_gui_glob LIST_DIRECTORY false "${GIS_TOOL_SOURCE_DIR}/src/gui/*.h" "${GIS_TOOL_SOURCE_DIR}/src/gui/*.cpp")
foreach(_f ${_tool_gui_glob})
    get_filename_component(_name "${_f}" NAME)
    list(APPEND _tool_gui_files "${_name}")
endforeach()

foreach(_f ${_tool_gui_files})
    if(NOT _f IN_LIST _dl_gui_files)
        if(_f IN_LIST _allowed_tool_only_files)
            list(APPEND _alignment_warnings "GIS_TOOL ONLY (allowed): src/gui/${_f}")
        else()
            list(APPEND _alignment_errors "GIS_TOOL ONLY (unexpected): src/gui/${_f}")
        endif()
    endif()
endforeach()

foreach(_f ${_dl_gui_files})
    if(NOT _f IN_LIST _tool_gui_files)
        list(APPEND _alignment_errors "GIS_DL_TOOL ONLY (unexpected): src/gui/${_f}")
    endif()
endforeach()

add_test(
    NAME cross_tool_alignment_check
    COMMAND ${CMAKE_COMMAND} -E echo "Cross-tool alignment check passed"
)

if(_alignment_errors)
    message(WARNING "Cross-tool alignment ERRORS detected:\n${_alignment_errors}")
    set_property(TEST cross_tool_alignment_check PROPERTY WILL_FAIL TRUE)
else()
    message(STATUS "Cross-tool alignment check: all shared GUI files present in both trees")
    if(_alignment_warnings)
        message(STATUS "Cross-tool alignment warnings (allowed differences):\n${_alignment_warnings}")
    endif()
endif()
