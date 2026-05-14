function(gis_gui_prepare_artifact_paths output_path screenshot_path status_path)
    get_filename_component(output_dir "${output_path}" DIRECTORY)
    get_filename_component(screenshot_dir "${screenshot_path}" DIRECTORY)
    get_filename_component(status_dir "${status_path}" DIRECTORY)

    file(REMOVE "${output_path}" "${screenshot_path}" "${status_path}")
    file(MAKE_DIRECTORY "${output_dir}")
    file(MAKE_DIRECTORY "${screenshot_dir}")
    file(MAKE_DIRECTORY "${status_dir}")
endfunction()

function(gis_gui_read_status_file test_name status_path out_success out_cancelled out_message out_raw_message)
    if(NOT EXISTS "${status_path}")
        message(FATAL_ERROR "${test_name} did not produce status file: ${status_path}")
    endif()

    file(READ "${status_path}" status_json)
    string(JSON status_success GET "${status_json}" success)
    string(JSON status_cancelled GET "${status_json}" cancelled)
    string(JSON status_message GET "${status_json}" message)
    string(JSON status_raw_message GET "${status_json}" raw_message)

    set(${out_success} "${status_success}" PARENT_SCOPE)
    set(${out_cancelled} "${status_cancelled}" PARENT_SCOPE)
    set(${out_message} "${status_message}" PARENT_SCOPE)
    set(${out_raw_message} "${status_raw_message}" PARENT_SCOPE)
endfunction()

function(gis_gui_assert_status_bool actual_value expected_value description)
    string(TOLOWER "${actual_value}" actual_normalized)
    string(TOLOWER "${expected_value}" expected_normalized)

    if(expected_normalized STREQUAL "true")
        if(NOT actual_normalized STREQUAL "true" AND
           NOT actual_normalized STREQUAL "on" AND
           NOT actual_normalized STREQUAL "1")
            message(FATAL_ERROR "${description}: expected true, got ${actual_value}")
        endif()
    else()
        if(NOT actual_normalized STREQUAL "false" AND
           NOT actual_normalized STREQUAL "off" AND
           NOT actual_normalized STREQUAL "0")
            message(FATAL_ERROR "${description}: expected false, got ${actual_value}")
        endif()
    endif()
endfunction()
