cmake_minimum_required(VERSION 3.16)
project(ScaleScaler VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection
if(WIN32)
    set(PLATFORM_NAME "windows")
elseif(APPLE)
    set(PLATFORM_NAME "macos")
else()
    set(PLATFORM_NAME "linux")
endif()

# Create the plugin library
add_library(ScaleScaler MODULE)

# Source files
target_sources(ScaleScaler PRIVATE
    src/entry.cpp
    src/plugin.cpp
    src/plugin.h
)

# Include directories
target_include_directories(ScaleScaler PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/clap-sdk/include
    ${CMAKE_CURRENT_SOURCE_DIR}/clap-helpers/include
)

# Compile definitions - CLAP_EXPORT for visibility
target_compile_definitions(ScaleScaler PRIVATE
    "CLAP_EXPORT=__attribute__((visibility(\"default\")))"
)

if(WIN32)
    target_compile_definitions(ScaleScaler PRIVATE
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
endif()

# Platform-specific settings
if(APPLE)
    # macOS settings
    set_target_properties(ScaleScaler PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION "clap"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.scalescaler.scale-scaler"
        MACOSX_BUNDLE_BUNDLE_NAME "Scale Scaler"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/clap-sdk/src/plugins.plist.in"
        PREFIX ""
        SUFFIX ""
    )
    
    target_compile_options(ScaleScaler PRIVATE
        -Wall -Wextra -Wpedantic
    )
    
    # Set install path for macOS
    set(CLAP_INSTALL_DIR "$ENV{HOME}/Library/Audio/Plug-Ins/CLAP" CACHE PATH "CLAP plugin install directory")
    
elseif(UNIX AND NOT APPLE)
    # Linux settings
    set_target_properties(ScaleScaler PROPERTIES
        PREFIX ""
        SUFFIX ".clap"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
    
    target_compile_options(ScaleScaler PRIVATE
        -Wall -Wextra -Wpedantic -fPIC
    )
    
    target_link_options(ScaleScaler PRIVATE
        -Wl,--no-undefined
    )
    
    # Set install path for Linux
    set(CLAP_INSTALL_DIR "$ENV{HOME}/.clap" CACHE PATH "CLAP plugin install directory")
    
elseif(WIN32)
    # Windows settings
    set_target_properties(ScaleScaler PROPERTIES
        PREFIX ""
        SUFFIX ".clap"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
    
    target_compile_options(ScaleScaler PRIVATE
        /W4 /permissive-
    )
    
    # Set install path for Windows
    set(CLAP_INSTALL_DIR "$ENV{LOCALAPPDATA}/Programs/CLAP" CACHE PATH "CLAP plugin install directory")
endif()

# Installation
install(TARGETS ScaleScaler
    LIBRARY DESTINATION "${CLAP_INSTALL_DIR}"
    RUNTIME DESTINATION "${CLAP_INSTALL_DIR}"
    BUNDLE DESTINATION "${CLAP_INSTALL_DIR}"
)

# Print configuration message
message(STATUS "Scale Scaler - CLAP Plugin")
message(STATUS "  Platform: ${PLATFORM_NAME}")
message(STATUS "  Install: ${CLAP_INSTALL_DIR}")