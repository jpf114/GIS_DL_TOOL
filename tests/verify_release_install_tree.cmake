if(NOT DEFINED REPO_ROOT)
    message(FATAL_ERROR "REPO_ROOT is required")
endif()

if(NOT DEFINED BUILD_DIR)
    message(FATAL_ERROR "BUILD_DIR is required")
endif()

set(INSTALL_ROOT "${REPO_ROOT}/install")

if(NOT DEFINED INSTALL_CONFIG)
    if(EXISTS "${BUILD_DIR}/bin/Release/gis-ai-gui.exe")
        set(INSTALL_CONFIG "Release")
    elseif(EXISTS "${BUILD_DIR}/bin/Debug/gis-ai-gui.exe")
        set(INSTALL_CONFIG "Debug")
    else()
        set(INSTALL_CONFIG "Release")
    endif()
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --install "${BUILD_DIR}" --config ${INSTALL_CONFIG}
    WORKING_DIRECTORY "${REPO_ROOT}"
    RESULT_VARIABLE INSTALL_EXIT_CODE
    TIMEOUT 180
)

if(NOT INSTALL_EXIT_CODE EQUAL 0)
    message(FATAL_ERROR "${INSTALL_CONFIG} install failed with exit code ${INSTALL_EXIT_CODE}")
endif()

function(require_path path_label path_value)
    if(NOT EXISTS "${path_value}")
        message(FATAL_ERROR "Missing ${path_label}: ${path_value}")
    endif()
endfunction()

require_path("install root" "${INSTALL_ROOT}")
require_path("bin directory" "${INSTALL_ROOT}/bin")
require_path("include directory" "${INSTALL_ROOT}/include")
require_path("lib directory" "${INSTALL_ROOT}/lib")
require_path("share directory" "${INSTALL_ROOT}/share")

require_path("GUI executable" "${INSTALL_ROOT}/bin/gis-ai-gui.exe")
require_path("CLI executable" "${INSTALL_ROOT}/bin/gis_ai_cli.exe")

if(INSTALL_CONFIG STREQUAL "Debug")
    require_path("Qt windows platform plugin (debug)" "${INSTALL_ROOT}/bin/platforms/qwindowsd.dll")
    require_path("Qt offscreen platform plugin (debug)" "${INSTALL_ROOT}/bin/platforms/qoffscreend.dll")
    require_path("Qt SQLite driver (debug)" "${INSTALL_ROOT}/bin/sqldrivers/qsqlited.dll")
else()
    require_path("Qt windows platform plugin" "${INSTALL_ROOT}/bin/platforms/qwindows.dll")
    require_path("Qt offscreen platform plugin" "${INSTALL_ROOT}/bin/platforms/qoffscreen.dll")
    require_path("Qt SQLite driver" "${INSTALL_ROOT}/bin/sqldrivers/qsqlite.dll")
endif()

require_path("public header" "${INSTALL_ROOT}/include/gis_ai/gis_ai.h")
require_path("package config" "${INSTALL_ROOT}/lib/cmake/gis_ai/gis_aiConfig.cmake")
require_path("PROJ database" "${INSTALL_ROOT}/share/proj/proj.db")
require_path("GDAL logo" "${INSTALL_ROOT}/share/gdal/GDALLogoColor.svg")
require_path("icon resource" "${INSTALL_ROOT}/share/icons/regular/brain-regular.svg")

file(GLOB install_bin_dirs RELATIVE "${INSTALL_ROOT}/bin" "${INSTALL_ROOT}/bin/*")
set(unexpected_bin_dirs "")
foreach(entry IN LISTS install_bin_dirs)
    if(IS_DIRECTORY "${INSTALL_ROOT}/bin/${entry}")
        if(NOT entry STREQUAL "platforms" AND
           NOT entry STREQUAL "sqldrivers" AND
           NOT entry STREQUAL "plugins" AND
           NOT entry STREQUAL "task_data")
            list(APPEND unexpected_bin_dirs "${entry}")
        endif()
    endif()
endforeach()

if(unexpected_bin_dirs)
    string(JOIN ", " unexpected_text ${unexpected_bin_dirs})
    message(FATAL_ERROR "Unexpected install/bin subdirectories: ${unexpected_text}")
endif()

message(STATUS "Release install tree verification passed")
