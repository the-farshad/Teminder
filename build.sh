#!/bin/bash

# Teminder build script
# This script configures and builds the Teminder application

set -e # Exit on error
echo "========================================"
echo "  Building Teminder"
echo "========================================"
echo ""

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake ..

# Build
# nproc for Linux and hw.ncpu for Mac or BSD
echo "Building..."
cmake --build . -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo "========================================"
echo "  Build complete!"
echo "========================================"
echo ""
echo "To run Teminder:"
echo " ./build/Teminder"
echo ""
echo "Or install it system-wide:"
echo " sudo cmake --install build"
echo ""