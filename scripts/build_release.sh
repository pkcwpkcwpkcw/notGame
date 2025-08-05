#!/bin/bash

echo "Building NOT Gate Game (Release)..."

# Clean previous build
if [ -d "build" ]; then
    echo "Cleaning previous build..."
    rm -rf build
fi

# Create build directory
mkdir -p build
cd build

# Configure
echo "Configuring..."
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
    cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
else
    cmake .. -DCMAKE_BUILD_TYPE=Release
fi

# Build
echo "Building..."
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
    cmake --build . --config Release
else
    cmake --build . -j$(nproc)
fi

echo "Build complete!"
echo "Executable location:"
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
    echo "  build/Release/notgate3.exe"
else
    echo "  build/notgate3"
fi