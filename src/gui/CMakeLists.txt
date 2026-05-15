cmake_minimum_required(VERSION 3.20)

add_library(gis_ai_gui_lib STATIC
    mainwindow.cpp
    nav_panel.cpp
    icon_manager.cpp
    param_card_widget.cpp
    param_widget.cpp
    crs_dialog.cpp
    gdal_config.cpp
    settings_manager.cpp
    qt_progress_reporter.cpp
    execute_worker.cpp
    progress_dialog.cpp
    task_database.cpp
    task_manager.cpp
    task_center_page.cpp
    gui_data_support.cpp
    task_runner.cpp
)

find_package(Qt6 COMPONENTS Sql REQUIRED)
find_package(SQLite3 REQUIRED)

target_link_libraries(gis_ai_gui_lib PUBLIC
    gis_ai_static
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
    Qt6::Sql
    SQLite3::SQLite3
    PROJ::proj
)

set_target_properties(gis_ai_gui_lib PROPERTIES
    AUTOMOC ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

add_executable(gis-ai-gui
    main.cpp
    "${CMAKE_SOURCE_DIR}/resources/app.rc"
)

target_link_libraries(gis-ai-gui PRIVATE gis_ai_gui_lib)

set_target_properties(gis-ai-gui PROPERTIES
    AUTOMOC ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

set(GIS_AI_QT_WINDOWS_DLL "$<$<CONFIG:Debug>:qwindowsd.dll>$<$<NOT:$<CONFIG:Debug>>:qwindows.dll>")
set(GIS_AI_QT_OFFSCREEN_DLL "$<$<CONFIG:Debug>:qoffscreend.dll>$<$<NOT:$<CONFIG:Debug>>:qoffscreen.dll>")
set(GIS_AI_QT_WINDOWS_PDB "$<$<CONFIG:Debug>:qwindowsd.pdb>$<$<NOT:$<CONFIG:Debug>>:qwindows.pdb>")
set(GIS_AI_QT_OFFSCREEN_PDB "$<$<CONFIG:Debug>:qoffscreend.pdb>$<$<NOT:$<CONFIG:Debug>>:qoffscreen.pdb>")
set(GIS_AI_COPY_QT_PLATFORM_DLL OFF)
set(GIS_AI_COPY_QT_PLATFORM_PDB OFF)

if(EXISTS "${GIS_AI_VCPKG_DEBUG_QT_PLATFORMS_DIR}/qwindowsd.dll" OR
   EXISTS "${GIS_AI_VCPKG_RELEASE_QT_PLATFORMS_DIR}/qwindows.dll" OR
   EXISTS "${GIS_AI_VCPKG_DEBUG_QT_PLATFORMS_DIR}/qoffscreend.dll" OR
   EXISTS "${GIS_AI_VCPKG_RELEASE_QT_PLATFORMS_DIR}/qoffscreen.dll")
    set(GIS_AI_COPY_QT_PLATFORM_DLL ON)
endif()

if(EXISTS "${GIS_AI_VCPKG_DEBUG_QT_PLATFORMS_DIR}/qwindowsd.pdb" OR
   EXISTS "${GIS_AI_VCPKG_RELEASE_QT_PLATFORMS_DIR}/qwindows.pdb" OR
   EXISTS "${GIS_AI_VCPKG_DEBUG_QT_PLATFORMS_DIR}/qoffscreend.pdb" OR
   EXISTS "${GIS_AI_VCPKG_RELEASE_QT_PLATFORMS_DIR}/qoffscreen.pdb")
    set(GIS_AI_COPY_QT_PLATFORM_PDB ON)
endif()

add_custom_command(TARGET gis-ai-gui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:gis-ai-gui>/platforms
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:gis-ai-gui>/sqldrivers
)

gis_ai_copy_minimal_runtime(gis-ai-gui)

if(GIS_AI_COPY_QT_PLATFORM_DLL)
    add_custom_command(TARGET gis-ai-gui POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${GIS_AI_VCPKG_QT_PLATFORMS_DIR}/${GIS_AI_QT_WINDOWS_DLL}
            $<TARGET_FILE_DIR:gis-ai-gui>/platforms/${GIS_AI_QT_WINDOWS_DLL}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${GIS_AI_VCPKG_QT_PLATFORMS_DIR}/${GIS_AI_QT_OFFSCREEN_DLL}
            $<TARGET_FILE_DIR:gis-ai-gui>/platforms/${GIS_AI_QT_OFFSCREEN_DLL}
        COMMAND_EXPAND_LISTS
    )
else()
    message(WARNING "未在全局 vcpkg 中找到 Qt 平台插件 DLL，构建将跳过 qwindows 插件拷贝。")
endif()

if(GIS_AI_COPY_QT_PLATFORM_PDB)
    add_custom_command(TARGET gis-ai-gui POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${GIS_AI_VCPKG_QT_PLATFORMS_DIR}/${GIS_AI_QT_WINDOWS_PDB}
            $<TARGET_FILE_DIR:gis-ai-gui>/platforms/${GIS_AI_QT_WINDOWS_PDB}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${GIS_AI_VCPKG_QT_PLATFORMS_DIR}/${GIS_AI_QT_OFFSCREEN_PDB}
            $<TARGET_FILE_DIR:gis-ai-gui>/platforms/${GIS_AI_QT_OFFSCREEN_PDB}
        COMMAND_EXPAND_LISTS
    )
endif()

set(GIS_AI_VCPKG_QT_SQLDRIVERS_DIR "$<$<CONFIG:Debug>:${GIS_AI_VCPKG_DEBUG_QT_DIR}/plugins/sqldrivers>$<$<NOT:$<CONFIG:Debug>>:${GIS_AI_VCPKG_RELEASE_QT_DIR}/plugins/sqldrivers>")
set(GIS_AI_QT_SQLITE_DLL "$<$<CONFIG:Debug>:qsqlited.dll>$<$<NOT:$<CONFIG:Debug>>:qsqlite.dll>")

add_custom_command(TARGET gis-ai-gui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${GIS_AI_VCPKG_QT_SQLDRIVERS_DIR}/${GIS_AI_QT_SQLITE_DLL}
        $<TARGET_FILE_DIR:gis-ai-gui>/sqldrivers/${GIS_AI_QT_SQLITE_DLL}
    COMMAND_EXPAND_LISTS
)

install(TARGETS gis-ai-gui
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(GIS_AI_COPY_QT_PLATFORM_DLL)
    install(FILES
        "${GIS_AI_VCPKG_RELEASE_QT_PLATFORMS_DIR}/qwindows.dll"
        "${GIS_AI_VCPKG_RELEASE_QT_PLATFORMS_DIR}/qoffscreen.dll"
        CONFIGURATIONS Release
        DESTINATION ${GIS_AI_INSTALL_PLATFORM_DIR}
    )
    install(FILES
        "${GIS_AI_VCPKG_DEBUG_QT_PLATFORMS_DIR}/qwindowsd.dll"
        "${GIS_AI_VCPKG_DEBUG_QT_PLATFORMS_DIR}/qoffscreend.dll"
        CONFIGURATIONS Debug
        DESTINATION ${GIS_AI_INSTALL_PLATFORM_DIR}
    )
endif()

set(GIS_AI_VCPKG_RELEASE_QT_SQLDRIVERS_DIR "${GIS_AI_VCPKG_INSTALLED_DIR}/Qt6/plugins/sqldrivers")
set(GIS_AI_VCPKG_DEBUG_QT_SQLDRIVERS_DIR "${GIS_AI_VCPKG_INSTALLED_DIR}/debug/Qt6/plugins/sqldrivers")

install(FILES
    "${GIS_AI_VCPKG_RELEASE_QT_SQLDRIVERS_DIR}/qsqlite.dll"
    CONFIGURATIONS Release
    DESTINATION ${GIS_AI_INSTALL_SQLDRIVERS_DIR}
)
install(FILES
    "${GIS_AI_VCPKG_DEBUG_QT_SQLDRIVERS_DIR}/qsqlited.dll"
    CONFIGURATIONS Debug
    DESTINATION ${GIS_AI_INSTALL_SQLDRIVERS_DIR}
)

add_custom_command(TARGET gis-ai-gui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources/icons"
        "$<TARGET_FILE_DIR:gis-ai-gui>/icons"
    COMMENT "Copying icon resources"
)
