# Examples CMakeLists.txt

cmake_minimum_required(VERSION 3.16)

# Demo segmentation example
add_executable(demo_segmentation
    demo_segmentation.cpp
)

target_link_libraries(demo_segmentation
    PRIVATE
        gis_ai_shared
)

target_include_directories(demo_segmentation
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# Demo GIS operations example
add_executable(demo_gis_ops
    demo_gis_ops.cpp
)

target_link_libraries(demo_gis_ops
    PRIVATE
        gis_ai_shared
)

target_include_directories(demo_gis_ops
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# Demo configuration usage example
add_executable(demo_config
    demo_config.cpp
)

target_link_libraries(demo_config
    PRIVATE
        gis_ai_shared
)

target_include_directories(demo_config
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)
