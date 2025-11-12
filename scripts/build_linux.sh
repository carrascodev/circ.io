#!/bin/bash
# Build script for Linux

set -e  # Exit on error

echo "========================================"
echo "Building Circ.io Client for Linux"
echo "========================================"

# Check for required tools
if ! command -v cmake &> /dev/null; then
    echo "ERROR: CMake not found!"
    echo "Install with: sudo apt-get install cmake"
    exit 1
fi

# Create build directory
BUILD_DIR="build"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo ""
echo "Configuring CMake for Linux..."
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake \
      ..

echo ""
echo "Building game client..."
cmake --build . --target game_client -j$(nproc)

echo ""
echo "========================================"
echo "Build complete!"
echo "========================================"
echo "Client executable: $BUILD_DIR/client/game_client"
echo ""
echo "To package for distribution:"
echo "  cd $BUILD_DIR/client"
echo "  tar -czf ../../circ-io-linux.tar.gz game_client"
echo ""
