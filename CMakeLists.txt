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
set(MNT_PLUGIN_SOURCES
    src/entry_impl.cpp
    src/entry_impl.h
    src/plugin.cpp
    src/plugin.h
    src/core.cpp
    src/core.h
)

function(mnt_configure_impl target)
    target_include_directories(${target} PRIVATE
        ${MNT_GENERATED_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/clap-sdk/include
        ${CMAKE_CURRENT_SOURCE_DIR}/clap-helpers/include
    )

    target_link_libraries(${target} PUBLIC
        clap
        clap-wrapper-extensions
    )

    if(MSVC)
        target_compile_definitions(${target} PRIVATE
            NOMINMAX
            _CRT_SECURE_NO_WARNINGS
        )
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_library(MidiNoteTranslator-impl STATIC ${MNT_PLUGIN_SOURCES})
mnt_configure_impl(MidiNoteTranslator-impl)

option(MNT_BUILD_VST3 "Build the VST3 wrapper" ON)

# CLAP: clap-first build from the audio-less note-effect implementation.
make_clapfirst_plugins(
    TARGET_NAME MidiNoteTranslator
    IMPL_TARGET MidiNoteTranslator-impl
    OUTPUT_NAME "MidiNoteTranslator"
    ENTRY_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/entry.cpp"
    BUNDLE_IDENTIFIER "com.sevish.midi-note-translator"
    BUNDLE_VERSION "${PROJECT_VERSION}"
    COPY_AFTER_BUILD FALSE
    PLUGIN_FORMATS CLAP
    ASSET_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/assets"
)

if(MNT_BUILD_VST3)
    # VST3: the same plugin with a silent stereo output so hosts load it as a
    # normal instrument. The CLAP remains an audio-less note effect.
    add_library(MidiNoteTranslator-impl-vst3 STATIC ${MNT_PLUGIN_SOURCES})
    mnt_configure_impl(MidiNoteTranslator-impl-vst3)
    target_compile_definitions(MidiNoteTranslator-impl-vst3 PRIVATE MNT_VST3_SILENT_OUTPUT)

    add_library(MidiNoteTranslator_vst3 MODULE)
    target_sources(MidiNoteTranslator_vst3 PRIVATE src/entry.cpp)
    target_link_libraries(MidiNoteTranslator_vst3 PRIVATE MidiNoteTranslator-impl-vst3)

    set(MNT_VST3_ASSET_DIR "${CMAKE_BINARY_DIR}/assets")
    if(WIN32)
        set(MNT_VST3_ASSET_DIR "${CMAKE_BINARY_DIR}/assets/VST3")
    endif()

    target_add_vst3_wrapper(
        TARGET MidiNoteTranslator_vst3
        OUTPUT_NAME "MidiNoteTranslator"
        BUNDLE_IDENTIFIER "com.sevish.midi-note-translator.vst3"
        BUNDLE_VERSION "${PROJECT_VERSION}"
        ASSET_OUTPUT_DIRECTORY "${MNT_VST3_ASSET_DIR}"
        WINDOWS_FOLDER_VST3 FALSE
    )

    if(TARGET MidiNoteTranslator_all)
        add_dependencies(MidiNoteTranslator_all MidiNoteTranslator_vst3)
    endif()
endif()

if(APPLE)
    set(MNT_APPLE_TARGETS MidiNoteTranslator_clap)
    if(MNT_BUILD_VST3)
        list(APPEND MNT_APPLE_TARGETS MidiNoteTranslator_vst3)
    endif()
    set_target_properties(${MNT_APPLE_TARGETS} PROPERTIES
        MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 2026 Sevish"
    )
endif()

set(MNT_FORMATS "CLAP")
if(MNT_BUILD_VST3)
    string(APPEND MNT_FORMATS " VST3")
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
message(STATUS "  Formats: ${MNT_FORMATS}")
message(STATUS "  Install: ${CMAKE_INSTALL_PREFIX}")
