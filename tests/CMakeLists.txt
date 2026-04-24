function(gis_ai_configure_test_environment test_name)
    set(_proj_dir "")

    if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
        set(_proj_dir "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/proj")
    elseif(DEFINED VCPKG_TARGET_TRIPLET)
        set(_proj_dir "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/share/proj")
    endif()

    if(EXISTS "${_proj_dir}/proj.db")
        set_property(TEST ${test_name} PROPERTY ENVIRONMENT "PROJ_LIB=${_proj_dir}")
    endif()
endfunction()

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
gis_ai_configure_test_environment(test_gis_ai)

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
gis_ai_configure_test_environment(test_io_integration)

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

if(WIN32)
    # ONNX Debug dependencies are not stable on Windows; keep AI integration on Release there.
    add_test(NAME test_ai_integration CONFIGURATIONS Release COMMAND test_ai_integration WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
else()
    add_test(NAME test_ai_integration COMMAND test_ai_integration WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif()
set_tests_properties(test_ai_integration PROPERTIES TIMEOUT 300)
gis_ai_configure_test_environment(test_ai_integration)
