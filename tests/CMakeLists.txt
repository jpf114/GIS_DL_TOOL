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

function(gis_ai_configure_generated_test_data test_name)
    if(TARGET generate_test_data)
        set_tests_properties(${test_name} PROPERTIES
            FIXTURES_REQUIRED gis_test_data
            RESOURCE_LOCK gis_test_data_resource
        )
    endif()
endfunction()

if(TARGET generate_test_data)
    add_test(
        NAME prepare_gis_test_data
        COMMAND $<TARGET_FILE:generate_test_data>
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(prepare_gis_test_data PROPERTIES
        TIMEOUT 120
        FIXTURES_SETUP gis_test_data
        RESOURCE_LOCK gis_test_data_resource
    )
    gis_ai_configure_test_environment(prepare_gis_test_data)
endif()

add_test(
    NAME prepare_ai_test_model
    COMMAND ${CMAKE_COMMAND}
        -DREPO_ROOT=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/prepare_ai_test_model.cmake
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
set_tests_properties(prepare_ai_test_model PROPERTIES
    TIMEOUT 60
    FIXTURES_SETUP ai_test_model
)

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
gis_ai_configure_generated_test_data(test_io_integration)

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
set_tests_properties(test_ai_integration PROPERTIES
    FIXTURES_REQUIRED "gis_test_data;ai_test_model"
    RESOURCE_LOCK gis_test_data_resource
)
gis_ai_configure_test_environment(test_ai_integration)

add_executable(test_cli
    test_cli.cpp
)

target_link_libraries(test_cli
    PRIVATE
        GTest::gtest
        GTest::gtest_main
)

add_test(NAME test_cli COMMAND test_cli WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
set_tests_properties(test_cli PROPERTIES TIMEOUT 60)
if(TARGET gis_ai_cli)
    set_tests_properties(test_cli PROPERTIES ENVIRONMENT
        "GIS_AI_CLI_PATH=$<TARGET_FILE:gis_ai_cli>"
    )
endif()

if(GIS_AI_BUILD_GUI)
    find_package(Qt6 COMPONENTS Core Gui Widgets Svg Sql REQUIRED)
    find_package(SQLite3 REQUIRED)

    add_executable(test_gui_task_database
        test_gui_task_database.cpp
        ../src/gui/task_database.cpp
        ../src/gui/task_manager.cpp
    )

    target_link_libraries(test_gui_task_database
        PRIVATE
            GTest::gtest
            GTest::gtest_main
            Qt6::Core
            Qt6::Sql
            Qt6::Widgets
            SQLite3::SQLite3
    )

    target_include_directories(test_gui_task_database
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/src/gui
    )

    set_target_properties(test_gui_task_database PROPERTIES AUTOMOC ON)

    add_test(NAME test_gui_task_database COMMAND test_gui_task_database WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    set_tests_properties(test_gui_task_database PROPERTIES TIMEOUT 60)

    add_executable(test_gui_queue
        test_gui_queue.cpp
        ../src/gui/mainwindow.cpp
        ../src/gui/nav_panel.cpp
        ../src/gui/icon_manager.cpp
        ../src/gui/param_card_widget.cpp
        ../src/gui/param_widget.cpp
        ../src/gui/crs_dialog.cpp
        ../src/gui/gdal_config.cpp
        ../src/gui/settings_manager.cpp
        ../src/gui/qt_progress_reporter.cpp
        ../src/gui/execute_worker.cpp
        ../src/gui/progress_dialog.cpp
        ../src/gui/task_database.cpp
        ../src/gui/task_manager.cpp
        ../src/gui/task_center_page.cpp
        ../src/gui/gui_data_support.cpp
        ../src/gui/task_runner.cpp
    )

    target_link_libraries(test_gui_queue
        PRIVATE
            gis_ai_static
            GTest::gtest
            GTest::gtest_main
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            Qt6::Svg
            Qt6::Sql
            SQLite3::SQLite3
            PROJ::proj
    )

    target_include_directories(test_gui_queue
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/src/gui
    )

    set_target_properties(test_gui_queue PROPERTIES AUTOMOC ON)

    add_test(NAME test_gui_queue COMMAND test_gui_queue WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    set_tests_properties(test_gui_queue PROPERTIES
        TIMEOUT 120
        FIXTURES_REQUIRED "gis_test_data;ai_test_model"
        RESOURCE_LOCK gis_test_data_resource
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
    gis_ai_configure_test_environment(test_gui_queue)

    add_test(NAME gui_smoke_test
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/run_gui_smoke_test.cmake
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(gui_smoke_test PROPERTIES
        TIMEOUT 60
        FIXTURES_REQUIRED "gis_test_data;ai_test_model"
        RESOURCE_LOCK gis_test_data_resource
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
    gis_ai_configure_test_environment(gui_smoke_test)

    add_test(NAME release_install_tree_test
        COMMAND ${CMAKE_COMMAND}
            -DREPO_ROOT=${CMAKE_SOURCE_DIR}
            -DBUILD_DIR=${CMAKE_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_release_install_tree.cmake
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(release_install_tree_test PROPERTIES
        TIMEOUT 240
    )
endif()
