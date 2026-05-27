cmake_minimum_required(VERSION 3.21)

# Must be set before project() so the toolchain picks it up.
# FORCE ensures this overrides any pre-existing (empty) cache entry.
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures" FORCE)

project(HandwritingFX VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# OFX SDK (headers only), pinned to the 1.4 release tag.
# FetchContent_Populate skips add_subdirectory so we don't build OFX examples.
# CMP0169 OLD allows the direct Populate call (deprecated in CMake 3.30).
if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

FetchContent_Declare(
    openfx
    GIT_REPOSITORY https://github.com/ofxa/openfx.git
    GIT_TAG        OFX_Release_1_4_TAG
    GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(openfx)
if(NOT openfx_POPULATED)
    FetchContent_Populate(openfx)
endif()

add_library(HandwritingFX SHARED src/plugin.cpp)

# No lib prefix, no .dylib suffix — the bundle target renames the output to .ofx
set_target_properties(HandwritingFX PROPERTIES
    PREFIX ""
    SUFFIX ""
    OUTPUT_NAME "HandwritingFX"
)

target_include_directories(HandwritingFX PRIVATE
    "${openfx_SOURCE_DIR}/include"
)

# ---- Bundle packaging -------------------------------------------------------

set(BUNDLE_DIR "${CMAKE_BINARY_DIR}/HandwritingFX.ofx.bundle")

add_custom_target(bundle
    DEPENDS HandwritingFX
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BUNDLE_DIR}/Contents/MacOS"
    COMMAND ${CMAKE_COMMAND} -E copy
        "$<TARGET_FILE:HandwritingFX>"
        "${BUNDLE_DIR}/Contents/MacOS/HandwritingFX.ofx"
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_SOURCE_DIR}/cmake/Info.plist"
        "${BUNDLE_DIR}/Contents/Info.plist"
    COMMENT "Assembling HandwritingFX.ofx.bundle"
    VERBATIM
)
