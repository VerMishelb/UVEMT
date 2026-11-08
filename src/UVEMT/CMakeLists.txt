set(UVEMT_VERSION_MAJOR 3)
set(UVEMT_VERSION_MINOR 7)
set(UVEMT_VERSION_PATCH 0)
set(UVEMT_VERSION_TWEAK 0)

configure_file(
    CmakeDefinitions.h.in
    CmakeDefinitions.h
)
configure_file(
    UVEMT.exe.manifest.in
    UVEMT.exe.manifest
)
configure_file(
    UVEMT.rc.in
    UVEMT.rc
)



target_sources(UVEMT
PRIVATE
    main.cpp
    Modes.cpp
    FileOperations-${CMAKE_SYSTEM_NAME}.cpp
    Functions.cpp
)

if (WIN32)
    target_sources(UVEMT
    PRIVATE
        ${PROJECT_BINARY_DIR}/src/UVEMT/UVEMT.exe.manifest
        ${PROJECT_BINARY_DIR}/src/UVEMT/UVEMT.rc
    )
endif()

target_include_directories(UVEMT PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_BINARY_DIR}/src/UVEMT)

# target_link_libraries(UVEMT PUBLIC)
