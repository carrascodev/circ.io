#!/bin/bash
# Build script for macOS (run this on a Mac)

set -e  # Exit on error

echo "========================================"
echo "Building Circ.io Client for macOS"
echo "========================================"

# Check if we're on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo "WARNING: This script should be run on macOS"
    echo "For cross-compilation from Linux, you need OSXCross (advanced)"
fi

# Check for required tools
if ! command -v cmake &> /dev/null; then
    echo "ERROR: CMake not found!"
    echo "Install with: brew install cmake"
    exit 1
fi

# Create build directory
BUILD_DIR="build-macos"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo ""
echo "Configuring CMake for macOS..."
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 \
      ..

echo ""
echo "Building game client..."
cmake --build . --target game_client -j$(sysctl -n hw.ncpu)

echo ""
echo "========================================"
echo "Build complete!"
echo "========================================"
echo "Client executable: $BUILD_DIR/client/game_client"
echo ""
echo "To create a .app bundle:"
echo "  mkdir -p CircIO.app/Contents/MacOS"
echo "  cp $BUILD_DIR/client/game_client CircIO.app/Contents/MacOS/"
echo "  # Add Info.plist, icon, etc."
echo ""
echo "To package for distribution:"
echo "  cd $BUILD_DIR/client"
echo "  tar -czf ../../circ-io-macos.tar.gz game_client"
echo ""
