set(GIS_TOOL_SOURCE_DIR "" CACHE PATH "Path to GIS_TOOL source tree for cross-tool UI alignment checks")

if(NOT GIS_TOOL_SOURCE_DIR)
    message(STATUS "GIS_TOOL_SOURCE_DIR not set, skipping cross-tool UI alignment check")
    return()
endif()

if(NOT EXISTS "${GIS_TOOL_SOURCE_DIR}/src/gui/style_constants.h")
    message(WARNING "GIS_TOOL_SOURCE_DIR does not appear to be a valid GIS_TOOL source tree")
    return()
endif()

set(_alignment_errors "")
set(_alignment_warnings "")
set(_alignment_info "")

function(_extract_color_constant content var_name out_value)
    string(REGEX MATCH "${var_name}[ \t]*=[ \t]*\"([^\"]+)\"" _match "${content}")
    if(_match)
        set(${out_value} "${CMAKE_MATCH_1}" PARENT_SCOPE)
    else()
        set(${out_value} "NOT_FOUND" PARENT_SCOPE)
    endif()
endfunction()

function(_extract_size_constant content var_name out_value)
    string(REGEX MATCH "${var_name}[ \t]*=[ \t]*([0-9]+)" _match "${content}")
    if(_match)
        set(${out_value} "${CMAKE_MATCH_1}" PARENT_SCOPE)
    else()
        set(${out_value} "NOT_FOUND" PARENT_SCOPE)
    endif()
endfunction()

set(_dl_style_file "${CMAKE_SOURCE_DIR}/src/gui/style_constants.h")
set(_tool_style_file "${GIS_TOOL_SOURCE_DIR}/src/gui/style_constants.h")

if(EXISTS "${_dl_style_file}" AND EXISTS "${_tool_style_file}")
    file(READ "${_dl_style_file}" _dl_style)
    file(READ "${_tool_style_file}" _tool_style)

    set(_color_vars
        "kWindowBg" "kPagePanelBg" "kCardBg" "kCardBorder" "kSidebarBg"
        "kSidebarSelected" "kSidebarHover" "kSidebarIndicator" "kSidebarText"
        "kPrimary" "kPrimaryHover" "kPrimaryPressed" "kPrimaryLight"
        "kSuccess" "kWarning" "kError"
        "kTextPrimary" "kTextSecondary" "kTextMuted"
        "kInputBorder" "kInputBg" "kInputFocusBorder"
        "kProgressTrack" "kProgressFill"
    )

    foreach(_var ${_color_vars})
        _extract_color_constant("${_dl_style}" "${_var}" _dl_val)
        _extract_color_constant("${_tool_style}" "${_var}" _tool_val)
        if(_dl_val STREQUAL "NOT_FOUND" OR _tool_val STREQUAL "NOT_FOUND")
            continue()
        endif()
        if(NOT _dl_val STREQUAL _tool_val)
            list(APPEND _alignment_errors
                "Color::${_var} mismatch: GIS_DL_TOOL=${_dl_val}, GIS_TOOL=${_tool_val}")
        endif()
    endforeach()

    set(_size_vars
        "kSidebarWidth" "kWindowMinWidth" "kWindowMinHeight"
        "kWindowDefaultWidth" "kWindowDefaultHeight"
        "kCardRadius" "kCardPadding" "kCardSpacing"
        "kInputRadius" "kInputMinHeight" "kButtonRadius" "kButtonMinHeight"
    )

    foreach(_var ${_size_vars})
        _extract_size_constant("${_dl_style}" "${_var}" _dl_val)
        _extract_size_constant("${_tool_style}" "${_var}" _tool_val)
        if(_dl_val STREQUAL "NOT_FOUND" OR _tool_val STREQUAL "NOT_FOUND")
            continue()
        endif()
        if(NOT _dl_val STREQUAL _tool_val)
            list(APPEND _alignment_errors
                "Size::${_var} mismatch: GIS_DL_TOOL=${_dl_val}, GIS_TOOL=${_tool_val}")
        endif()
    endforeach()

    set(_sidebar_keywords
        "QPushButton#navItem"
        "QPushButton#subNavItem"
        "QLabel#sidebarTitle"
        "QLabel#sidebarSection"
        "QFrame#sidebar"
        "QFrame#sidebarTopCard"
    )

    foreach(_kw ${_sidebar_keywords})
        string(FIND "${_dl_style}" "${_kw}" _dl_pos)
        string(FIND "${_tool_style}" "${_kw}" _tool_pos)
        if(_dl_pos LESS 0 AND _tool_pos GREATER_EQUAL 0)
            list(APPEND _alignment_warnings
                "sidebarStyleSheet: '${_kw}' missing in GIS_DL_TOOL but present in GIS_TOOL")
        elseif(_tool_pos LESS 0 AND _dl_pos GREATER_EQUAL 0)
            list(APPEND _alignment_info
                "sidebarStyleSheet: '${_kw}' present in GIS_DL_TOOL but missing in GIS_TOOL (allowed extension)")
        endif()
    endforeach()

    set(_global_keywords
        "QFrame#card"
        "QFrame#heroCard"
        "QFrame#execCard"
        "QLabel#heroBadge"
        "QLabel#heroTitle"
        "QLabel#heroDesc"
        "QLabel#cardTitle"
        "QLabel#paramLabel"
        "QPushButton#primaryButton"
        "QPushButton#secondaryButton"
        "QPushButton#browseButton"
        "QProgressBar"
        "QTabBar::tab"
        "QTreeWidget"
        "QTextEdit#logTerminal"
    )

    foreach(_kw ${_global_keywords})
        string(FIND "${_dl_style}" "${_kw}" _dl_pos)
        string(FIND "${_tool_style}" "${_kw}" _tool_pos)
        if(_dl_pos LESS 0 AND _tool_pos GREATER_EQUAL 0)
            list(APPEND _alignment_warnings
                "globalStyleSheet: '${_kw}' missing in GIS_DL_TOOL but present in GIS_TOOL")
        elseif(_tool_pos LESS 0 AND _dl_pos GREATER_EQUAL 0)
            list(APPEND _alignment_info
                "globalStyleSheet: '${_kw}' present in GIS_DL_TOOL but missing in GIS_TOOL (allowed extension)")
        endif()
    endforeach()
else()
    list(APPEND _alignment_warnings "style_constants.h not found in one or both projects, skipping style alignment check")
endif()

set(_ui_structure_files
    "nav_panel.h:NavPanel"
    "param_card_widget.h:ParamCardWidget"
    "param_widget.h:ParamWidget"
    "mainwindow.h:MainWindow"
    "task_center_page.h:TaskCenterPage"
    "progress_dialog.h:ProgressDialog"
    "icon_manager.h:IconManager"
    "crs_dialog.h:CrsDialog"
    "settings_manager.h:SettingsManager"
    "gdal_config.h:GdalConfig"
    "qt_progress_reporter.h:QtProgressReporter"
    "execute_worker.h:ExecuteWorker"
    "task_runner.h:TaskRunner"
    "task_database.h:TaskDatabase"
    "task_manager.h:TaskManager"
)

foreach(_entry ${_ui_structure_files})
    string(REPLACE ":" ";" _parts ${_entry})
    list(GET _parts 0 _file)
    list(GET _parts 1 _class)

    set(_dl_file "${CMAKE_SOURCE_DIR}/src/gui/${_file}")
    set(_tool_file "${GIS_TOOL_SOURCE_DIR}/src/gui/${_file}")

    if(NOT EXISTS "${_dl_file}" AND NOT EXISTS "${_tool_file}")
        continue()
    endif()

    if(NOT EXISTS "${_dl_file}")
        list(APPEND _alignment_warnings
            "UI component ${_class} (${_file}) missing in GIS_DL_TOOL")
        continue()
    endif()

    if(NOT EXISTS "${_tool_file}")
        list(APPEND _alignment_info
            "UI component ${_class} (${_file}) unique to GIS_DL_TOOL (allowed)")
        continue()
    endif()
endforeach()

set(_layout_markers
    "nav_panel.cpp:QPushButton#navItem:sidebar navigation button"
    "nav_panel.cpp:QPushButton#subNavItem:sidebar sub-navigation button"
    "nav_panel.cpp:QLabel#sidebarTitle:sidebar title label"
    "mainwindow.cpp:QFrame#heroCard:hero information card"
    "mainwindow.cpp:QFrame#execCard:execution control card"
    "mainwindow.cpp:QPushButton#primaryButton:primary execute button"
    "mainwindow.cpp:QTabWidget:tab container"
    "mainwindow.cpp:QProgressBar:status progress bar"
    "param_card_widget.cpp:QFrame#card:param card frame"
    "param_card_widget.cpp:QLabel#paramLabel:param label"
    "param_card_widget.cpp:QPushButton#browseButton:file browse button"
    "task_center_page.cpp:QTreeWidget:task list tree"
    "task_center_page.cpp:QTextEdit#logTerminal:log terminal"
)

foreach(_entry ${_layout_markers})
    string(REPLACE ":" ";" _parts ${_entry})
    list(GET _parts 0 _file)
    list(GET _parts 1 _marker)
    list(GET _parts 2 _desc)

    set(_dl_file "${CMAKE_SOURCE_DIR}/src/gui/${_file}")
    set(_tool_file "${GIS_TOOL_SOURCE_DIR}/src/gui/${_file}")

    if(NOT EXISTS "${_dl_file}" OR NOT EXISTS "${_tool_file}")
        continue()
    endif()

    file(READ "${_dl_file}" _dl_content)
    file(READ "${_tool_file}" _tool_content)

    string(FIND "${_dl_content}" "${_marker}" _dl_pos)
    string(FIND "${_tool_content}" "${_marker}" _tool_pos)

    if(_dl_pos LESS 0 AND _tool_pos GREATER_EQUAL 0)
        list(APPEND _alignment_warnings
            "Layout marker '${_marker}' (${_desc}) missing in GIS_DL_TOOL ${_file}")
    elseif(_tool_pos LESS 0 AND _dl_pos GREATER_EQUAL 0)
        list(APPEND _alignment_info
            "Layout marker '${_marker}' (${_desc}) present in GIS_DL_TOOL but not in GIS_TOOL ${_file} (allowed extension)")
    endif()
endforeach()

add_test(
    NAME cross_tool_alignment_check
    COMMAND ${CMAKE_COMMAND} -E echo "Cross-tool UI alignment check passed"
)

if(_alignment_errors)
    set(_error_text "Cross-tool UI alignment ERRORS detected:\n")
    foreach(_err ${_alignment_errors})
        string(APPEND _error_text "  - ${_err}\n")
    endforeach()
    message(WARNING "${_error_text}")
    set_property(TEST cross_tool_alignment_check PROPERTY WILL_FAIL TRUE)
else()
    message(STATUS "Cross-tool UI alignment check: style constants and layout markers consistent")
    if(_alignment_warnings)
        message(STATUS "Cross-tool UI alignment warnings (review recommended):")
        foreach(_warn ${_alignment_warnings})
            message(STATUS "  - ${_warn}")
        endforeach()
    endif()
    if(_alignment_info)
        message(STATUS "Cross-tool UI alignment info (allowed extensions):")
        foreach(_info ${_alignment_info})
            message(STATUS "  + ${_info}")
        endforeach()
    endif()
endif()
