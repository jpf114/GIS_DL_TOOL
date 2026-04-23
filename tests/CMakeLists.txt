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
        gis_ai_static
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(test_gis_ai
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

add_test(NAME test_gis_ai COMMAND test_gis_ai WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
set_tests_properties(test_gis_ai PROPERTIES TIMEOUT 300)

add_executable(test_io_integration
    test_io_integration.cpp
)

target_link_libraries(test_io_integration
    PRIVATE
        gis_ai_static
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(test_io_integration
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

add_test(NAME test_io_integration COMMAND test_io_integration WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
set_tests_properties(test_io_integration PROPERTIES TIMEOUT 300)

add_executable(test_ai_integration
    test_ai_integration.cpp
)

target_link_libraries(test_ai_integration
    PRIVATE
        gis_ai_static
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(test_ai_integration
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

add_test(NAME test_ai_integration COMMAND test_ai_integration WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
set_tests_properties(test_ai_integration PROPERTIES TIMEOUT 300)
