cmake_minimum_required(VERSION 3.16)
project(gis_ai_lib
    VERSION 0.1.0
    DESCRIPTION "A high-performance, cross-platform C++ GIS AI algorithm library"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(GIS_AI_BUILD_TESTS "Build unit tests" ON)
option(GIS_AI_BUILD_EXAMPLES "Build example programs" ON)
option(GIS_AI_ENABLE_CUDA "Enable CUDA support for GPU inference" OFF)
option(GIS_AI_BUILD_SHARED "Build shared library" ON)
option(GIS_AI_BUILD_STATIC "Build static library" ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(GDAL CONFIG REQUIRED)
find_package(PROJ CONFIG REQUIRED)
find_package(GEOS CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(onnxruntime CONFIG REQUIRED)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

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
)

if(GIS_AI_BUILD_SHARED)
    add_library(gis_ai_shared SHARED ${LIB_SOURCES})
    target_link_libraries(gis_ai_shared PUBLIC ${COMMON_LINK_LIBS})
    target_compile_definitions(gis_ai_shared PRIVATE GIS_AI_SHARED_BUILD)
    set_target_properties(gis_ai_shared PROPERTIES
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
    add_library(gis_ai ALIAS gis_ai_static)
endif()

if(GIS_AI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(GIS_AI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

option(GIS_AI_BUILD_TOOLS "Build utility tools" ON)
if(GIS_AI_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

install(DIRECTORY include/gis_ai/ DESTINATION include/gis_ai)
if(GIS_AI_BUILD_SHARED)
    install(TARGETS gis_ai_shared
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )
endif()
if(GIS_AI_BUILD_STATIC)
    install(TARGETS gis_ai_static
        ARCHIVE DESTINATION lib
    )
endif()

message(STATUS "")
message(STATUS "=== GIS AI Library Configuration ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Build Shared: ${GIS_AI_BUILD_SHARED}")
message(STATUS "Build Static: ${GIS_AI_BUILD_STATIC}")
message(STATUS "Build Tests: ${GIS_AI_BUILD_TESTS}")
message(STATUS "Build Examples: ${GIS_AI_BUILD_EXAMPLES}")
message(STATUS "CUDA Support: ${GIS_AI_ENABLE_CUDA}")
message(STATUS "====================================")
message(STATUS "")
