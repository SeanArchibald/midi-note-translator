cmake_minimum_required(VERSION 3.16)
project(MidiNoteTranslator VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# Default to the user's CLAP folder when no install prefix was provided.
# Passing -DCMAKE_INSTALL_PREFIX (as build.sh and CI do) overrides this.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    if(APPLE)
        set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/Library/Audio/Plug-Ins/CLAP" CACHE PATH "CLAP plugin install directory" FORCE)
    elseif(WIN32)
        set(CMAKE_INSTALL_PREFIX "$ENV{LOCALAPPDATA}/Programs/CLAP" CACHE PATH "CLAP plugin install directory" FORCE)
    else()
        set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.clap" CACHE PATH "CLAP plugin install directory" FORCE)
    endif()
endif()

# Create the plugin library
add_library(MidiNoteTranslator MODULE)

# Source files
target_sources(MidiNoteTranslator PRIVATE
    src/entry.cpp
    src/plugin.cpp
    src/plugin.h
    src/core.cpp
    src/core.h
)

# Include directories
target_include_directories(MidiNoteTranslator PRIVATE
    ${MNT_GENERATED_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/clap-sdk/include
    ${CMAKE_CURRENT_SOURCE_DIR}/clap-helpers/include
)

# CLAP_EXPORT is defined per platform by the CLAP SDK (clap/private/macros.h);
# overriding it with a GNU attribute breaks the MSVC build.

if(WIN32)
    target_compile_definitions(MidiNoteTranslator PRIVATE
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
endif()

# Platform-specific settings
if(APPLE)
    # macOS settings
    set_target_properties(MidiNoteTranslator PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION "clap"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.sevish.midi-note-translator"
        MACOSX_BUNDLE_BUNDLE_NAME "MIDI Note Translator"
        MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 2026 Sevish"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/clap-sdk/src/plugins.plist.in"
        PREFIX ""
        SUFFIX ""
    )
    
    target_compile_options(MidiNoteTranslator PRIVATE
        -Wall -Wextra -Wpedantic
    )
    
elseif(UNIX AND NOT APPLE)
    # Linux settings
    set_target_properties(MidiNoteTranslator PROPERTIES
        PREFIX ""
        SUFFIX ".clap"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
    
    target_compile_options(MidiNoteTranslator PRIVATE
        -Wall -Wextra -Wpedantic -fPIC
    )
    
    target_link_options(MidiNoteTranslator PRIVATE
        -Wl,--no-undefined
    )
    
elseif(WIN32)
    # Windows settings
    set_target_properties(MidiNoteTranslator PROPERTIES
        PREFIX ""
        SUFFIX ".clap"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
    
    target_compile_options(MidiNoteTranslator PRIVATE
        /W4 /permissive-
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

# Installation
install(TARGETS MidiNoteTranslator
    LIBRARY DESTINATION .
    RUNTIME DESTINATION .
    BUNDLE DESTINATION .
)

# Print configuration message
message(STATUS "MIDI Note Translator - CLAP Plugin")
message(STATUS "  Platform: ${PLATFORM_NAME}")
message(STATUS "  Install: ${CMAKE_INSTALL_PREFIX}")
