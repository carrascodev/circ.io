#!/bin/bash
# Build script for Windows (cross-compile from Linux using MinGW)

set -e  # Exit on error

echo "========================================"
echo "Building Circ.io Client for Windows"
echo "========================================"

# Check if MinGW is installed
if ! command -v x86_64-w64-mingw32-gcc &> /dev/null; then
    echo "ERROR: MinGW-w64 not found!"
    echo "Install with: sudo apt-get install mingw-w64"
    exit 1
fi

# Create build directory
BUILD_DIR="build-windows"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo ""
echo "Configuring CMake for Windows..."
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=../cmake/mingw-w64-toolchain.cmake \
      ..

echo ""
echo "Building game client..."
cmake --build . --target game_client -j$(nproc)

echo ""
echo "========================================"
echo "Build complete!"
echo "========================================"
echo "Client executable: $BUILD_DIR/client/game_client.exe"
echo ""
echo "To package for distribution:"
echo "  cd $BUILD_DIR/client"
echo "  zip -r ../../circ-io-windows.zip game_client.exe"
echo ""
