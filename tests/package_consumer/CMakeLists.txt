cmake_minimum_required(VERSION 3.21)

project(gis_ai_package_consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(gis_ai CONFIG REQUIRED)

add_executable(consumer_shared main.cpp)
target_link_libraries(consumer_shared PRIVATE gis_ai::gis_ai)

add_executable(consumer_static main.cpp)
target_link_libraries(consumer_static PRIVATE gis_ai::gis_ai_static)
