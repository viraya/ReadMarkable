cmake_minimum_required(VERSION 3.20)
project(ReadMarkableInstaller
    VERSION 1.0.0
    DESCRIPTION "ReadMarkable host installer (Qt Widgets, libssh2)"
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Concurrent Test)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/core/version.h
    @ONLY
)

find_package(Libssh2 CONFIG REQUIRED)

set(INSTALLER_LIB_SOURCES
    src/core/pinned_versions.cpp
    src/payload/bundled_payload.cpp
    src/payload/cache.cpp
    src/payload/github_fetcher.cpp
    src/ssh/known_hosts.cpp
    src/ssh/ssh_client.cpp
    src/ssh/ssh_session.cpp
    src/flow/device_state.cpp
    src/flow/router.cpp
    src/flow/flow_runner.cpp
    src/flow/install_flow.cpp
    src/credentials/keypair.cpp
    src/credentials/device_store.cpp
    src/credentials/auth_handler.cpp
    src/wizard/installer_wizard.cpp
    src/wizard/page_welcome.cpp
    src/wizard/page_prereqs.cpp
    src/wizard/page_connect.cpp
    src/wizard/page_action_summary.cpp
    src/wizard/page_installing.cpp
    src/wizard/page_done.cpp
    src/diagnostics/logger.cpp
    src/diagnostics/diagnostics_zip.cpp
)

add_library(installer-lib OBJECT ${INSTALLER_LIB_SOURCES})
target_include_directories(installer-lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/generated
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)
find_package(OpenSSL REQUIRED)

target_link_libraries(installer-lib PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
    Libssh2::libssh2
    OpenSSL::Crypto
)
if(WIN32)
    target_link_libraries(installer-lib PUBLIC Ws2_32 Crypt32 Advapi32)
endif()

set(INSTALLER_SOURCES
    src/main.cpp
)

set(PAYLOAD_FETCH_STAMP ${CMAKE_CURRENT_BINARY_DIR}/.payload-fetched.stamp)
add_custom_command(
    OUTPUT ${PAYLOAD_FETCH_STAMP}
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/scripts/fetch-bundled-payload.sh
    COMMAND ${CMAKE_COMMAND} -E touch ${PAYLOAD_FETCH_STAMP}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/config/pinned-versions.toml
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Fetching pinned XOVI + AppLoad payload"
    VERBATIM
)
add_custom_target(installer-payload-fetch DEPENDS ${PAYLOAD_FETCH_STAMP})

qt_add_resources(INSTALLER_RESOURCES resources/installer.qrc)
target_sources(installer-lib PRIVATE ${INSTALLER_RESOURCES})
add_dependencies(installer-lib installer-payload-fetch)

if(WIN32)
    add_executable(readmarkable-installer WIN32 ${INSTALLER_SOURCES})
elseif(APPLE)
    add_executable(readmarkable-installer MACOSX_BUNDLE ${INSTALLER_SOURCES})
    set_target_properties(readmarkable-installer PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "ReadMarkable Installer"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.readmarkable.installer"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )
else()
    add_executable(readmarkable-installer ${INSTALLER_SOURCES})
endif()

target_link_libraries(readmarkable-installer PRIVATE installer-lib)

enable_testing()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
    add_subdirectory(tests)
endif()
