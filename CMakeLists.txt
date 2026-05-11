cmake_minimum_required(VERSION 3.21)

# vcpkg toolchain 集成必须在 project() 之前
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()
set(VCPKG_MANIFEST_MODE OFF CACHE BOOL "使用全局 vcpkg 安装" FORCE)

project(gis_ai_lib
    VERSION 0.1.0
    DESCRIPTION "A high-performance, cross-platform C++ GIS AI algorithm library"
    LANGUAGES CXX
)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "默认安装路径" FORCE)
endif()

include(GNUInstallDirs)

if(POLICY CMP0207)
    cmake_policy(SET CMP0207 NEW)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

option(GIS_AI_BUILD_GUI "Build Qt GUI" OFF)
option(GIS_AI_BUILD_TESTS "Build unit tests" ON)
option(GIS_AI_BUILD_EXAMPLES "Build example programs" ON)
option(GIS_AI_ENABLE_CUDA "Enable CUDA support for GPU inference" OFF)
option(GIS_AI_BUILD_SHARED "Build shared library" ON)
option(GIS_AI_BUILD_STATIC "Build static library" ON)
option(GIS_AI_BUILD_TOOLS "Build utility tools" ON)
option(GIS_AI_BUILD_DOCS "Build API documentation with Doxygen" OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ── vcpkg 全局安装目录推导 ──
set(GIS_AI_VCPKG_ROOT "")
if(DEFINED ENV{VCPKG_ROOT} AND NOT "$ENV{VCPKG_ROOT}" STREQUAL "")
    set(GIS_AI_VCPKG_ROOT "$ENV{VCPKG_ROOT}")
elseif(DEFINED CMAKE_TOOLCHAIN_FILE AND EXISTS "${CMAKE_TOOLCHAIN_FILE}")
    get_filename_component(GIS_AI_VCPKG_ROOT "${CMAKE_TOOLCHAIN_FILE}" DIRECTORY)
    get_filename_component(GIS_AI_VCPKG_ROOT "${GIS_AI_VCPKG_ROOT}/../.." ABSOLUTE)
endif()

if(GIS_AI_VCPKG_ROOT STREQUAL "")
    message(FATAL_ERROR "未设置环境变量 VCPKG_ROOT，且无法从 CMAKE_TOOLCHAIN_FILE 推断全局 vcpkg 目录。")
endif()

set(GIS_AI_VCPKG_TRIPLET "x64-windows" CACHE STRING "全局 vcpkg triplet")
set(GIS_AI_VCPKG_INSTALLED_DIR "${GIS_AI_VCPKG_ROOT}/installed/${GIS_AI_VCPKG_TRIPLET}")
set(GIS_AI_VCPKG_RELEASE_BIN_DIR "${GIS_AI_VCPKG_INSTALLED_DIR}/bin")
set(GIS_AI_VCPKG_DEBUG_BIN_DIR "${GIS_AI_VCPKG_INSTALLED_DIR}/debug/bin")
set(GIS_AI_VCPKG_SHARE_DIR "${GIS_AI_VCPKG_INSTALLED_DIR}/share")
set(GIS_AI_VCPKG_PROJ_DATA_DIR "${GIS_AI_VCPKG_SHARE_DIR}/proj")
set(GIS_AI_VCPKG_GDAL_DATA_DIR "${GIS_AI_VCPKG_SHARE_DIR}/gdal")

file(TO_CMAKE_PATH "${GIS_AI_VCPKG_INSTALLED_DIR}" GIS_AI_VCPKG_INSTALLED_DIR)
file(TO_CMAKE_PATH "${GIS_AI_VCPKG_RELEASE_BIN_DIR}" GIS_AI_VCPKG_RELEASE_BIN_DIR)
file(TO_CMAKE_PATH "${GIS_AI_VCPKG_DEBUG_BIN_DIR}" GIS_AI_VCPKG_DEBUG_BIN_DIR)
file(TO_CMAKE_PATH "${GIS_AI_VCPKG_SHARE_DIR}" GIS_AI_VCPKG_SHARE_DIR)
file(TO_CMAKE_PATH "${GIS_AI_VCPKG_PROJ_DATA_DIR}" GIS_AI_VCPKG_PROJ_DATA_DIR)
file(TO_CMAKE_PATH "${GIS_AI_VCPKG_GDAL_DATA_DIR}" GIS_AI_VCPKG_GDAL_DATA_DIR)

set(GIS_AI_VCPKG_RUNTIME_BIN_DIR "$<$<CONFIG:Debug>:${GIS_AI_VCPKG_DEBUG_BIN_DIR}>$<$<NOT:$<CONFIG:Debug>>:${GIS_AI_VCPKG_RELEASE_BIN_DIR}>")

# 配置安装时运行时依赖脚本
configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/install_runtime_deps.cmake.in"
    "${CMAKE_BINARY_DIR}/install_runtime_deps.cmake"
    @ONLY
)

if(NOT EXISTS "${GIS_AI_VCPKG_INSTALLED_DIR}")
    message(FATAL_ERROR "未找到全局 vcpkg 安装目录: ${GIS_AI_VCPKG_INSTALLED_DIR}")
endif()

if(NOT EXISTS "${GIS_AI_VCPKG_PROJ_DATA_DIR}")
    message(FATAL_ERROR "未找到 PROJ 数据目录: ${GIS_AI_VCPKG_PROJ_DATA_DIR}")
endif()

if(NOT EXISTS "${GIS_AI_VCPKG_GDAL_DATA_DIR}")
    message(FATAL_ERROR "未找到 GDAL 数据目录: ${GIS_AI_VCPKG_GDAL_DATA_DIR}")
endif()

# ── 依赖查找 ──
find_package(GDAL CONFIG REQUIRED)
find_package(PROJ CONFIG REQUIRED)
find_package(GEOS CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(onnxruntime CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

if(GIS_AI_BUILD_TESTS)
    find_package(GTest CONFIG REQUIRED)
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# ── 源文件 ──
set(CORE_SOURCES
    src/core/logger.cpp
    src/core/exception.cpp
    src/core/platform.cpp
    src/core/memory.cpp
    src/core/config.cpp
)

set(IO_SOURCES
    src/io/raster_io.cpp
    src/io/vector_io.cpp
    src/io/pointcloud_io.cpp
    src/io/io_factory.cpp
)

set(GIS_SOURCES
    src/gis/vector_buffer.cpp
    src/gis/vector_intersect.cpp
    src/gis/vector_clip.cpp
    src/gis/vector_simplify.cpp
    src/gis/vector_topology.cpp
    src/gis/raster_resample.cpp
    src/gis/raster_normalize.cpp
    src/gis/raster_clip.cpp
    src/gis/raster_threshold.cpp
    src/gis/raster_mosaic.cpp
    src/gis/pc_filter.cpp
    src/gis/pc_downsample.cpp
    src/gis/coord_transform.cpp
)

set(AI_SOURCES
    src/ai/ort_context.cpp
    src/ai/model_manager.cpp
    src/ai/inference_engine.cpp
    src/ai/preprocess.cpp
    src/ai/postprocess.cpp
    src/ai/mask_to_polygon.cpp
)

set(FUSION_SOURCES
    src/fusion/raster_seg.cpp
    src/fusion/batch_processor.cpp
    src/fusion/large_image_seg.cpp
    src/fusion/task_config.cpp
)

set(LIB_SOURCES
    ${CORE_SOURCES}
    ${IO_SOURCES}
    ${GIS_SOURCES}
    ${AI_SOURCES}
    ${FUSION_SOURCES}
    src/c_api.cpp
)

set(COMMON_LINK_LIBS
    GDAL::GDAL
    PROJ::proj
    GEOS::geos_c
    spdlog::spdlog
    onnxruntime::onnxruntime
    nlohmann_json::nlohmann_json
)

# ── 库目标 ──
if(GIS_AI_BUILD_SHARED)
    add_library(gis_ai_shared SHARED ${LIB_SOURCES})
    target_link_libraries(gis_ai_shared PUBLIC ${COMMON_LINK_LIBS})
    target_compile_definitions(gis_ai_shared PRIVATE GIS_AI_SHARED_BUILD)
    set_target_properties(gis_ai_shared PROPERTIES
        EXPORT_NAME gis_ai
        OUTPUT_NAME "gis_ai"
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
    )
    if(WIN32)
        target_compile_definitions(gis_ai_shared PRIVATE GIS_AI_PLATFORM_WINDOWS)
    elseif(UNIX)
        target_compile_definitions(gis_ai_shared PRIVATE GIS_AI_PLATFORM_LINUX)
    endif()
endif()

if(GIS_AI_BUILD_STATIC)
    add_library(gis_ai_static STATIC ${LIB_SOURCES})
    target_link_libraries(gis_ai_static PUBLIC ${COMMON_LINK_LIBS})
    target_compile_definitions(gis_ai_static
        PRIVATE GIS_AI_STATIC_BUILD
        PUBLIC GIS_AI_STATIC_BUILD
    )
    set_target_properties(gis_ai_static PROPERTIES
        EXPORT_NAME gis_ai_static
        OUTPUT_NAME "gis_ai"
        PREFIX "lib"
    )
    if(WIN32)
        target_compile_definitions(gis_ai_static PRIVATE GIS_AI_PLATFORM_WINDOWS)
    elseif(UNIX)
        target_compile_definitions(gis_ai_static PRIVATE GIS_AI_PLATFORM_LINUX)
    endif()
endif()

if(GIS_AI_BUILD_SHARED)
    add_library(gis_ai ALIAS gis_ai_shared)
else()
    set_target_properties(gis_ai_static PROPERTIES EXPORT_NAME gis_ai)
    add_library(gis_ai ALIAS gis_ai_static)
endif()

# ── 运行时 DLL 复制 ──
include("${CMAKE_SOURCE_DIR}/cmake/gis_ai_runtime_dlls.cmake")

add_custom_target(copy_runtime_dlls ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/share/proj
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/share/gdal
)

gis_ai_copy_minimal_runtime(copy_runtime_dlls DEST_DIR "${CMAKE_BINARY_DIR}/bin")

add_custom_command(TARGET copy_runtime_dlls POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${GIS_AI_VCPKG_PROJ_DATA_DIR}
        ${CMAKE_BINARY_DIR}/share/proj
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${GIS_AI_VCPKG_GDAL_DATA_DIR}
        ${CMAKE_BINARY_DIR}/share/gdal
    COMMENT "复制 PROJ/GDAL 数据目录到构建目录"
)

# ── 测试 ──
if(GIS_AI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ── 示例 ──
if(GIS_AI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ── 工具和 CLI ──
if(GIS_AI_BUILD_TOOLS)
    add_subdirectory(tools)

    add_executable(gis_ai_cli src/cli/main.cpp)
    target_link_libraries(gis_ai_cli PRIVATE gis_ai_static ${COMMON_LINK_LIBS})
    set_target_properties(gis_ai_cli PROPERTIES OUTPUT_NAME "gis_ai_cli")

    if(GIS_AI_BUILD_TESTS)
        set_property(TEST test_cli PROPERTY ENVIRONMENT
            "GIS_AI_CLI_PATH=$<TARGET_FILE:gis_ai_cli>"
        )
    endif()
endif()

# ── GUI ──
if(GIS_AI_BUILD_GUI)
    find_package(Qt6 COMPONENTS Core Gui Widgets Svg Sql REQUIRED)
    find_package(SQLite3 REQUIRED)
    add_subdirectory(src/gui)
endif()

# ── 安装规则 ──
install(DIRECTORY include/gis_ai/ DESTINATION include/gis_ai)
install(DIRECTORY src/core/ DESTINATION include/gis_ai/internal/core
    FILES_MATCHING PATTERN "*.h")
install(DIRECTORY src/io/ DESTINATION include/gis_ai/internal/io
    FILES_MATCHING PATTERN "*.h")
install(DIRECTORY src/gis/ DESTINATION include/gis_ai/internal/gis
    FILES_MATCHING PATTERN "*.h")
install(DIRECTORY src/ai/ DESTINATION include/gis_ai/internal/ai
    FILES_MATCHING PATTERN "*.h")
install(DIRECTORY src/fusion/ DESTINATION include/gis_ai/internal/fusion
    FILES_MATCHING PATTERN "*.h")

install(DIRECTORY
    "${GIS_AI_VCPKG_PROJ_DATA_DIR}/"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/proj"
)

install(DIRECTORY
    "${GIS_AI_VCPKG_GDAL_DATA_DIR}/"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/gdal"
)

install(DIRECTORY
    "${CMAKE_SOURCE_DIR}/resources/icons/"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/icons"
)

if(GIS_AI_BUILD_TOOLS)
    install(TARGETS gis_ai_cli
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

if(GIS_AI_BUILD_SHARED)
    install(TARGETS gis_ai_shared
        EXPORT gis_aiTargets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
    )
endif()
if(GIS_AI_BUILD_STATIC)
    install(TARGETS gis_ai_static
        EXPORT gis_aiTargets
        ARCHIVE DESTINATION lib
        INCLUDES DESTINATION include
    )
endif()

install(EXPORT gis_aiTargets
    FILE gis_aiTargets.cmake
    NAMESPACE gis_ai::
    DESTINATION lib/cmake/gis_ai
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/gis_aiConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/gis_aiConfig.cmake
    INSTALL_DESTINATION lib/cmake/gis_ai
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/gis_aiConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/gis_aiConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/gis_aiConfigVersion.cmake
    DESTINATION lib/cmake/gis_ai
)

install(SCRIPT "${CMAKE_BINARY_DIR}/install_runtime_deps.cmake")

# ── 文档 ──
if(GIS_AI_BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile)
        set(DOXYGEN_OUT ${CMAKE_BINARY_DIR}/docs/Doxyfile)
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
        message(STATUS "Doxygen found: docs target available")
    else()
        message(WARNING "Doxygen not found: docs target not available")
    endif()
endif()

message(STATUS "")
message(STATUS "=== GIS AI Library Configuration ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Build Shared: ${GIS_AI_BUILD_SHARED}")
message(STATUS "Build Static: ${GIS_AI_BUILD_STATIC}")
message(STATUS "Build Tests: ${GIS_AI_BUILD_TESTS}")
message(STATUS "Build Examples: ${GIS_AI_BUILD_EXAMPLES}")
message(STATUS "Build GUI: ${GIS_AI_BUILD_GUI}")
message(STATUS "CUDA Support: ${GIS_AI_ENABLE_CUDA}")
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "vcpkg Root: ${GIS_AI_VCPKG_ROOT}")
message(STATUS "vcpkg Triplet: ${GIS_AI_VCPKG_TRIPLET}")
message(STATUS "====================================")
message(STATUS "")
