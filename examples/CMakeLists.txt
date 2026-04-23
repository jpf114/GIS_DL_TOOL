add_executable(demo_segmentation
    demo_segmentation.cpp
)

target_link_libraries(demo_segmentation
    PRIVATE
        gis_ai_static
)

add_executable(demo_gis_ops
    demo_gis_ops.cpp
)

target_link_libraries(demo_gis_ops
    PRIVATE
        gis_ai_static
)

add_executable(demo_config
    demo_config.cpp
)

target_link_libraries(demo_config
    PRIVATE
        gis_ai_static
)
