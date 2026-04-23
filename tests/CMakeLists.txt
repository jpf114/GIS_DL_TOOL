# Tests CMakeLists.txt

cmake_minimum_required(VERSION 3.16)

# Test executable
add_executable(test_gis_ai
    test_main.cpp
    test_core.cpp
    test_io.cpp
    test_gis.cpp
    test_ai.cpp
    test_fusion.cpp
)

target_link_libraries(test_gis_ai
    PRIVATE
        gis_ai_shared
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(test_gis_ai
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

# Register test with CTest
add_test(
    NAME test_gis_ai
    COMMAND test_gis_ai
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Set test properties
set_tests_properties(test_gis_ai PROPERTIES
    TIMEOUT 300
    ENVIRONMENT "TEST_DATA_DIR=${CMAKE_SOURCE_DIR}/tests/data"
)
