cmake_minimum_required(VERSION 4.1)

# Static linking of vcruntime
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

project(IAMI LANGUAGES CXX
    VERSION 1.3.9.0
    DESCRIPTION "InterAction Mod Installer"
    HOMEPAGE_URL "https://github.com/VerMishelb/UVEMT"
)

add_executable(IAMI main.cpp legacy.cpp CIModInstaller.rc)
target_compile_options(IAMI
    PUBLIC
    "$<$<C_COMPILER_ID:MSVC>:/utf-8>"
    "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>"
)
