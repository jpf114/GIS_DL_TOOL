add_executable(generate_test_data
    ${CMAKE_SOURCE_DIR}/tools/generate_test_data.cpp
)

target_link_libraries(generate_test_data PRIVATE
    GDAL::GDAL
)

add_executable(benchmark
    ${CMAKE_SOURCE_DIR}/tools/benchmark.cpp
    ${CMAKE_SOURCE_DIR}/src/io/raster_io.cpp
    ${CMAKE_SOURCE_DIR}/src/io/vector_io.cpp
    ${CMAKE_SOURCE_DIR}/src/io/pointcloud_io.cpp
    ${CMAKE_SOURCE_DIR}/src/io/io_factory.cpp
    ${CMAKE_SOURCE_DIR}/src/gis/vector_buffer.cpp
    ${CMAKE_SOURCE_DIR}/src/gis/vector_intersect.cpp
    ${CMAKE_SOURCE_DIR}/src/gis/vector_clip.cpp
    ${CMAKE_SOURCE_DIR}/src/gis/vector_simplify.cpp
    ${CMAKE_SOURCE_DIR}/src/gis/vector_topology.cpp
    ${CMAKE_SOURCE_DIR}/src/gis/raster_resample.cpp
    ${CMAKE_SOURCE_DIR}/src/gis/raster_normalize.cpp
    ${CMAKE_SOURCE_DIR}/src/gis/raster_clip.cpp
    ${CMAKE_SOURCE_DIR}/src/gis/raster_threshold.cpp
    ${CMAKE_SOURCE_DIR}/src/gis/raster_mosaic.cpp
    ${CMAKE_SOURCE_DIR}/src/gis/pc_filter.cpp
    ${CMAKE_SOURCE_DIR}/src/gis/pc_downsample.cpp
    ${CMAKE_SOURCE_DIR}/src/gis/coord_transform.cpp
    ${CMAKE_SOURCE_DIR}/src/core/logger.cpp
    ${CMAKE_SOURCE_DIR}/src/core/exception.cpp
    ${CMAKE_SOURCE_DIR}/src/core/config.cpp
    ${CMAKE_SOURCE_DIR}/src/core/memory.cpp
    ${CMAKE_SOURCE_DIR}/src/core/platform.cpp
)

target_link_libraries(benchmark PRIVATE
    GDAL::GDAL
    PROJ::proj
    GEOS::geos_c
    spdlog::spdlog
)

target_compile_definitions(benchmark PRIVATE
    GIS_AI_STATIC_BUILD
)

target_include_directories(benchmark PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)
