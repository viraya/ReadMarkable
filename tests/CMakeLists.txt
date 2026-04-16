cmake_minimum_required(VERSION 3.16)
project(ReadMarkableTests VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# This CMakeLists builds host-native (x86_64) unit tests only.
# It is NOT the cross-compile target for the device binary.
# Build with: cmake tests/ -B build-tests -DQt6_DIR=... && cmake --build build-tests

find_package(Qt6 REQUIRED COMPONENTS Core Test)

enable_testing()

qt_add_executable(XochitlMetadataParserTest
    XochitlMetadataParserTest.cpp
    ../src/library/XochitlMetadataParser.cpp
)

target_include_directories(XochitlMetadataParserTest PRIVATE
    ${CMAKE_SOURCE_DIR}/../src
)

target_link_libraries(XochitlMetadataParserTest PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME XochitlMetadataParserTest COMMAND XochitlMetadataParserTest)
