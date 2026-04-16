cmake_minimum_required(VERSION 3.16)

# pugixml 1.15  -  vendored static library
# MIT license  -  safe for commercial use and bundling

add_library(pugixml STATIC
    pugixml.cpp
)

target_include_directories(pugixml PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(pugixml PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
