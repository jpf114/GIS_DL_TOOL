cmake_minimum_required(VERSION 3.16)
project(gis_ai_installed_package_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(gis_ai CONFIG REQUIRED)

if(NOT TARGET gis_ai::gis_ai)
    message(FATAL_ERROR "未找到导出的 gis_ai::gis_ai 目标，请先检查安装产物是否完整。")
endif()

add_executable(demo_segmentation_installed
    ../demo_segmentation.cpp
)

target_link_libraries(demo_segmentation_installed
    PRIVATE
        gis_ai::gis_ai
)
