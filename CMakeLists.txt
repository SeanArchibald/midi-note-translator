cmake_minimum_required(VERSION 3.21)
project(MidiNoteTranslator VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# clap-wrapper builds small Objective-C++ shims on Apple platforms.
if(APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)
endif()

include(CTest)

# Generate the version header so the plugin descriptor cannot drift from CMake.
set(MNT_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${MNT_GENERATED_DIR}")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mnt_version.h.in"
    "${MNT_GENERATED_DIR}/mnt_version.h"
    @ONLY
)

# Platform detection
if(WIN32)
    set(PLATFORM_NAME "windows")
elseif(APPLE)
    set(PLATFORM_NAME "macos")
else()
    set(PLATFORM_NAME "linux")
endif()

# Stage installs in ./install unless the caller chose a prefix.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/install" CACHE PATH "Plugin install directory" FORCE)
endif()

if(APPLE AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
endif()

if(WIN32 AND NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# clap-wrapper builds the CLAP and VST3 targets from the same static library.
# The VST3 SDK is downloaded at configure time unless VST3_SDK_ROOT is set.
set(CLAP_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/clap-sdk" CACHE PATH "CLAP SDK directory")
set(CLAP_WRAPPER_DOWNLOAD_DEPENDENCIES ON CACHE BOOL "Download the VST3 SDK at configure time")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/clap-wrapper" EXCLUDE_FROM_ALL)

# Format-independent plugin implementation plus its CLAP entry points.
add_library(MidiNoteTranslator-impl STATIC
    src/entry_impl.cpp
    src/entry_impl.h
    src/plugin.cpp
    src/plugin.h
    src/core.cpp
    src/core.h
)

target_include_directories(MidiNoteTranslator-impl PRIVATE
    ${MNT_GENERATED_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/clap-sdk/include
    ${CMAKE_CURRENT_SOURCE_DIR}/clap-helpers/include
)

target_link_libraries(MidiNoteTranslator-impl PUBLIC
    clap
    clap-wrapper-extensions
)

if(MSVC)
    target_compile_definitions(MidiNoteTranslator-impl PRIVATE
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
    target_compile_options(MidiNoteTranslator-impl PRIVATE /W4 /permissive-)
else()
    target_compile_options(MidiNoteTranslator-impl PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(MNT_BUILD_VST3 "Build the VST3 wrapper" ON)

set(MNT_PLUGIN_FORMATS CLAP)
if(MNT_BUILD_VST3)
    list(APPEND MNT_PLUGIN_FORMATS VST3)
endif()

make_clapfirst_plugins(
    TARGET_NAME MidiNoteTranslator
    IMPL_TARGET MidiNoteTranslator-impl
    OUTPUT_NAME "MidiNoteTranslator"
    ENTRY_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/entry.cpp"
    BUNDLE_IDENTIFIER "com.sevish.midi-note-translator"
    BUNDLE_VERSION "${PROJECT_VERSION}"
    COPY_AFTER_BUILD FALSE
    PLUGIN_FORMATS ${MNT_PLUGIN_FORMATS}
    ASSET_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/assets"
)

if(APPLE)
    set_target_properties(MidiNoteTranslator_clap MidiNoteTranslator_vst3 PROPERTIES
        MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 2026 Sevish"
    )
endif()

# Unit tests
if(BUILD_TESTING)
    add_executable(MidiNoteTranslatorTests
        tests/test_core.cpp
        src/core.cpp
    )

    target_include_directories(MidiNoteTranslatorTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    if(MSVC)
        target_compile_options(MidiNoteTranslatorTests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(MidiNoteTranslatorTests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME MidiNoteTranslatorTests COMMAND MidiNoteTranslatorTests)
endif()

# Staged install: both formats and the license notices.
install(DIRECTORY "${CMAKE_BINARY_DIR}/assets/"
    DESTINATION .
    USE_SOURCE_PERMISSIONS
)
install(FILES LICENSE.md THIRD_PARTY_NOTICES.md DESTINATION .)
install(DIRECTORY LICENSES DESTINATION .)

# Print configuration message
message(STATUS "MIDI Note Translator - CLAP/VST3 Plugin")
message(STATUS "  Platform: ${PLATFORM_NAME}")
message(STATUS "  Formats: ${MNT_PLUGIN_FORMATS}")
message(STATUS "  Install: ${CMAKE_INSTALL_PREFIX}")
