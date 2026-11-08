cmake_minimum_required(VERSION 4.1)

if (MSVC)
    add_compile_options("/utf-8")
endif()

project(UVEMT
    LANGUAGES C CXX
    VERSION 3.7.0.0
    DESCRIPTION "Ultra VGA Engine Mod Tool, previously known as IAMT, CIMT."
    HOMEPAGE_URL "https://github.com/VerMishelb/UVEMT"
)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# There's no restricted build anymore.
# option(UVEMT_RESTRICTED_BUILD "Build IAMTa" OFF)
option(DEBUG_OUTPUT "Print out debug information" OFF)

# find_package(simdutf REQUIRED)

add_executable(UVEMT)
add_subdirectory(src/UVEMT)
# add_executable(UVEMI)
# add_subdirectory(src/UVEMI)

