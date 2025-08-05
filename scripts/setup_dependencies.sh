#!/bin/bash

echo "Setting up dependencies for NOT Gate Game..."

# Detect OS
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "Detected Linux"
    sudo apt update
    sudo apt install -y build-essential cmake libsdl2-dev libgl1-mesa-dev git
    
elif [[ "$OSTYPE" == "darwin"* ]]; then
    echo "Detected macOS"
    if ! command -v brew &> /dev/null; then
        echo "Homebrew not found. Please install from https://brew.sh/"
        exit 1
    fi
    brew install cmake sdl2
    
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
    echo "Detected Windows"
    echo "Please install:"
    echo "1. Visual Studio 2022 with C++ development tools"
    echo "2. CMake from https://cmake.org/"
    echo "3. SDL2 using vcpkg: vcpkg install sdl2:x64-windows"
    
else
    echo "Unknown OS: $OSTYPE"
    exit 1
fi

# Clone external dependencies
echo "Setting up external libraries..."
mkdir -p external

# Dear ImGui
if [ ! -d "external/imgui" ]; then
    git clone https://github.com/ocornut/imgui.git external/imgui
    cd external/imgui
    git checkout v1.89.5
    cd ../..
fi

# JSON library
if [ ! -d "external/json" ]; then
    git clone https://github.com/nlohmann/json.git external/json
    cd external/json
    git checkout v3.11.2
    cd ../..
fi

# STB libraries
if [ ! -d "external/stb" ]; then
    git clone https://github.com/nothings/stb.git external/stb
fi

echo "Dependencies setup complete!"