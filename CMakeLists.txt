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

# FreeType, built from source. Disable optional dependencies to keep the build
# minimal — we only need TrueType rasterisation.
FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG        VER-2-13-2
    GIT_SHALLOW    TRUE
)
set(FT_DISABLE_ZLIB     TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2    TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG      TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI   TRUE CACHE BOOL "" FORCE)
# FreeType 2.13.2 declares cmake_minimum_required(VERSION 3.0) which CMake 4.x
# rejects. This variable tells CMake to treat it as 3.5 and configure anyway.
set(CMAKE_POLICY_VERSION_MINIMUM "3.5" CACHE STRING "" FORCE)
FetchContent_MakeAvailable(freetype)

# Download Open Sans Regular (Apache 2.0) if not already present.
set(OPENSANS_TTF "${CMAKE_SOURCE_DIR}/assets/OpenSans-Regular.ttf")
if(NOT EXISTS "${OPENSANS_TTF}")
    message(STATUS "Downloading OpenSans-Regular.ttf...")
    file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/assets")
    file(DOWNLOAD
        "https://raw.githubusercontent.com/googlefonts/opensans/main/fonts/ttf/OpenSans-Regular.ttf"
        "${OPENSANS_TTF}"
        TLS_VERIFY ON
    )
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

target_link_libraries(HandwritingFX PRIVATE freetype)

# ---- Bundle packaging -------------------------------------------------------

set(BUNDLE_DIR "${CMAKE_BINARY_DIR}/HandwritingFX.ofx.bundle")

add_custom_target(bundle
    DEPENDS HandwritingFX
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BUNDLE_DIR}/Contents/MacOS"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BUNDLE_DIR}/Contents/Resources"
    COMMAND ${CMAKE_COMMAND} -E copy
        "$<TARGET_FILE:HandwritingFX>"
        "${BUNDLE_DIR}/Contents/MacOS/HandwritingFX.ofx"
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_SOURCE_DIR}/cmake/Info.plist"
        "${BUNDLE_DIR}/Contents/Info.plist"
    COMMAND ${CMAKE_COMMAND} -E copy
        "${OPENSANS_TTF}"
        "${BUNDLE_DIR}/Contents/Resources/OpenSans-Regular.ttf"
    COMMENT "Assembling HandwritingFX.ofx.bundle"
    VERBATIM
)
