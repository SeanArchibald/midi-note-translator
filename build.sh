#!/bin/bash

# Build script for macOS and Linux

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
INSTALL_PREFIX="${SCRIPT_DIR}/install"

# Detect platform
if [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="macOS"
    GENERATOR="Unix Makefiles"
    NATIVE_EXTRA=""
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    PLATFORM="Linux"
    GENERATOR="Unix Makefiles"
    NATIVE_EXTRA=""
else
    echo "Unsupported platform: $OSTYPE"
    exit 1
fi

echo "Building Scale Scaler CLAP Plugin for ${PLATFORM}..."
echo "================================================"

# Clean previous build
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"

# Configure
echo "Configuring with CMake..."
cd "${BUILD_DIR}"
cmake .. \
    -G "${GENERATOR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}"

# Build
echo "Building..."
cmake --build . --config Release -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Install (local copy)
echo "Installing locally..."
cmake --install . --config Release

echo ""
echo "================================================"
echo "Build complete!"
echo ""
echo "Plugin location:"
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "  ${INSTALL_PREFIX}/scale-scaler.clap"
    echo ""
    echo "To install system-wide, copy to:"
    echo "  ~/Library/Audio/Plug-Ins/CLAP/"
else
    echo "  ${INSTALL_PREFIX}/scale-scaler.clap"
    echo ""
    echo "To install system-wide, copy to:"
    echo "  ~/.clap/"
fi
echo ""
echo "Or run: cmake --install ${BUILD_DIR}"
echo "================================================"