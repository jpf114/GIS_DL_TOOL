cmake_minimum_required(VERSION 3.20)

add_library(gis_gui_common STATIC
    style_constants.h
    icon_manager.cpp
    crs_dialog.cpp
    settings_manager.cpp
    progress_dialog.cpp
    gdal_config.cpp
)

find_package(Qt6 COMPONENTS Core Gui Widgets Svg REQUIRED)
find_package(PROJ REQUIRED)

target_link_libraries(gis_gui_common PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
    PROJ::proj
    gis_ai_static
)

target_include_directories(gis_gui_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(gis_gui_common PROPERTIES
    AUTOMOC ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

add_library(gis_ai_gui_lib STATIC
    mainwindow.cpp
    execution_controller.cpp
    batch_mode_controller.cpp
    nav_panel.cpp
    param_card_widget.cpp
    param_widget.cpp
    qt_progress_reporter.cpp
    execute_worker.cpp
    task_database.cpp
    task_manager.cpp
    task_center_page.cpp
    gui_data_support.cpp
    data_inspector.cpp
    action_dispatcher.cpp
    style_constants.cpp
    task_runner.cpp
)

find_package(Qt6 COMPONENTS Sql REQUIRED)
find_package(SQLite3 REQUIRED)

target_link_libraries(gis_ai_gui_lib PUBLIC
    gis_ai_static
    gis_gui_common
    Qt6::Sql
    SQLite3::SQLite3
)

set_target_properties(gis_ai_gui_lib PROPERTIES
    AUTOMOC ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

add_executable(gis-ai-gui
    main.cpp
    "${CMAKE_BINARY_DIR}/generated/app.rc"
)

target_link_libraries(gis-ai-gui PRIVATE gis_ai_gui_lib)

set_target_properties(gis-ai-gui PROPERTIES
    AUTOMOC ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

set(GIS_AI_COPY_QT_PLATFORM_DLL OFF)

if(EXISTS "${GIS_AI_VCPKG_DEBUG_QT_PLATFORMS_DIR}/qwindowsd.dll" OR
   EXISTS "${GIS_AI_VCPKG_RELEASE_QT_PLATFORMS_DIR}/qwindows.dll" OR
   EXISTS "${GIS_AI_VCPKG_DEBUG_QT_PLATFORMS_DIR}/qoffscreend.dll" OR
   EXISTS "${GIS_AI_VCPKG_RELEASE_QT_PLATFORMS_DIR}/qoffscreen.dll")
    set(GIS_AI_COPY_QT_PLATFORM_DLL ON)
endif()

gis_ai_copy_gui_runtime(gis-ai-gui)

install(TARGETS gis-ai-gui
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(GIS_AI_COPY_QT_PLATFORM_DLL)
    install(DIRECTORY "${GIS_AI_VCPKG_RELEASE_QT_PLATFORMS_DIR}/"
        CONFIGURATIONS Release
        DESTINATION ${GIS_AI_INSTALL_PLATFORM_DIR}
        FILES_MATCHING PATTERN "*.dll"
    )
    install(DIRECTORY "${GIS_AI_VCPKG_DEBUG_QT_PLATFORMS_DIR}/"
        CONFIGURATIONS Debug
        DESTINATION ${GIS_AI_INSTALL_PLATFORM_DIR}
        FILES_MATCHING PATTERN "*.dll"
    )
endif()

set(GIS_AI_VCPKG_RELEASE_QT_SQLDRIVERS_DIR "${GIS_AI_VCPKG_INSTALLED_DIR}/Qt6/plugins/sqldrivers")
set(GIS_AI_VCPKG_DEBUG_QT_SQLDRIVERS_DIR "${GIS_AI_VCPKG_INSTALLED_DIR}/debug/Qt6/plugins/sqldrivers")

install(DIRECTORY "${GIS_AI_VCPKG_RELEASE_QT_SQLDRIVERS_DIR}/"
    CONFIGURATIONS Release
    DESTINATION ${GIS_AI_INSTALL_SQLDRIVERS_DIR}
    FILES_MATCHING PATTERN "*.dll"
)
install(DIRECTORY "${GIS_AI_VCPKG_DEBUG_QT_SQLDRIVERS_DIR}/"
    CONFIGURATIONS Debug
    DESTINATION ${GIS_AI_INSTALL_SQLDRIVERS_DIR}
    FILES_MATCHING PATTERN "*.dll"
)
