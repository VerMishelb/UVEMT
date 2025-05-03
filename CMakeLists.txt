cmake_minimum_required(VERSION 3.30)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

option(UVEMT_RESTRICTED_BUILD "Build IAMTa" OFF)
option(DEBUG_OUTPUT "Print out debug information" OFF)

if (MSVC)
    add_compile_options("/utf-8")
endif()

project(UVEMT-project
    LANGUAGES CXX
    VERSION 3.7.0.0
    DESCRIPTION "Ultra VGA Engine Mod Tool, previously known as IAMT, CIMT."
    HOMEPAGE_URL "https://github.com/VerMishelb/UVEMT"
)

add_subdirectory(src/UVEMT)
# add_subdirectory(src/UVEMI)

