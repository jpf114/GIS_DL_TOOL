cmake_minimum_required(VERSION 3.20)

add_executable(gis-ai-gui
    main.cpp
    mainwindow.cpp
)

target_link_libraries(gis-ai-gui PRIVATE
    gis_ai_static
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
)

set_target_properties(gis-ai-gui PROPERTIES
    AUTOMOC ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

set(GIS_AI_QT_WINDOWS_DLL "$<$<CONFIG:Debug>:qwindowsd.dll>$<$<NOT:$<CONFIG:Debug>>:qwindows.dll>")
set(GIS_AI_QT_OFFSCREEN_DLL "$<$<CONFIG:Debug>:qoffscreend.dll>$<$<NOT:$<CONFIG:Debug>>:qoffscreen.dll>")
set(GIS_AI_COPY_QT_PLATFORM_DLL OFF)

set(GIS_AI_VCPKG_DEBUG_QT_PLATFORMS_DIR "${GIS_AI_VCPKG_INSTALLED_DIR}/debug/plugins/platforms")
set(GIS_AI_VCPKG_RELEASE_QT_PLATFORMS_DIR "${GIS_AI_VCPKG_INSTALLED_DIR}/plugins/platforms")
set(GIS_AI_VCPKG_QT_PLATFORMS_DIR "$<$<CONFIG:Debug>:${GIS_AI_VCPKG_DEBUG_QT_PLATFORMS_DIR}>$<$<NOT:$<CONFIG:Debug>>:${GIS_AI_VCPKG_RELEASE_QT_PLATFORMS_DIR}>")

if(EXISTS "${GIS_AI_VCPKG_DEBUG_QT_PLATFORMS_DIR}/qwindowsd.dll" OR
   EXISTS "${GIS_AI_VCPKG_RELEASE_QT_PLATFORMS_DIR}/qwindows.dll" OR
   EXISTS "${GIS_AI_VCPKG_DEBUG_QT_PLATFORMS_DIR}/qoffscreend.dll" OR
   EXISTS "${GIS_AI_VCPKG_RELEASE_QT_PLATFORMS_DIR}/qoffscreen.dll")
    set(GIS_AI_COPY_QT_PLATFORM_DLL ON)
endif()

add_custom_command(TARGET gis-ai-gui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:gis-ai-gui>/platforms
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

install(TARGETS gis-ai-gui
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(GIS_AI_COPY_QT_PLATFORM_DLL)
    install(FILES
        "${GIS_AI_VCPKG_RELEASE_QT_PLATFORMS_DIR}/qwindows.dll"
        "${GIS_AI_VCPKG_RELEASE_QT_PLATFORMS_DIR}/qoffscreen.dll"
        CONFIGURATIONS Release
        DESTINATION ${CMAKE_INSTALL_BINDIR}/platforms
    )
endif()
