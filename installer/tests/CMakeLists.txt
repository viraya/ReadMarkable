# Unit tests (Tier 1) for the installer library.
#
# Integration tests (Tier 2, against the Docker mock-chiappa) live under
# tests/integration/ and are driven by run-smoke.sh + the C++ tests added later.

find_package(Qt6 REQUIRED COMPONENTS Test)

set(RM_INSTALLER_UNIT_TESTS
    unit/test_pinned_versions.cpp
    unit/test_bundled_payload.cpp
    unit/test_known_hosts.cpp
    unit/test_device_state.cpp
    unit/test_router.cpp
    unit/test_keypair.cpp
    unit/test_device_store.cpp
    unit/test_cache.cpp
    unit/test_install_flow_structure.cpp
    unit/test_logger.cpp
)

foreach(test_src IN LISTS RM_INSTALLER_UNIT_TESTS)
    get_filename_component(test_name "${test_src}" NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE installer-lib Qt6::Test)
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/unit
    )
    target_compile_definitions(${test_name} PRIVATE
        RM_INSTALLER_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
    # On Windows the Qt6 DLLs live in $QTDIR/bin  -  put them on the test's PATH
    # so ctest doesn't crash with exit code 0xC0000135 (DLL not found).
    if(WIN32)
        get_target_property(_qt_bin Qt6::Core IMPORTED_LOCATION_RELEASE)
        if(NOT _qt_bin)
            get_target_property(_qt_bin Qt6::Core IMPORTED_LOCATION_DEBUG)
        endif()
        get_filename_component(_qt_bin_dir "${_qt_bin}" DIRECTORY)
        set_tests_properties(${test_name} PROPERTIES
            ENVIRONMENT "PATH=${_qt_bin_dir};$ENV{PATH}"
        )
    endif()
endforeach()
