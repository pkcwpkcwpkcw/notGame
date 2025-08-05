# Building NOT Gate Game

## Prerequisites

### All Platforms
- CMake 3.20 or higher
- C++20 compatible compiler
- Git

### Platform Specific

#### Windows
- Visual Studio 2022 (or MSVC compiler)
- vcpkg (recommended) or manual SDL2 installation

#### Linux
```bash
sudo apt update
sudo apt install build-essential cmake libsdl2-dev libgl1-mesa-dev
```

#### macOS
```bash
brew install cmake sdl2
```

## Build Instructions

### 1. Clone Repository
```bash
git clone --recursive https://github.com/pkcwpkcwpkcw/notGame.git
cd notGame
```

### 2. Install Dependencies

#### Using vcpkg (Windows)
```bash
vcpkg install sdl2:x64-windows
```

#### Manual Installation
Download SDL2 development libraries from https://www.libsdl.org/

### 3. Configure and Build

#### Windows (Visual Studio)
```bash
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

#### Windows (MinGW)
```bash
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

#### Linux/macOS
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### 4. Run
```bash
# Windows
./build/Release/notgate3.exe

# Linux/macOS  
./build/notgate3
```

## Build Options

### Debug Build
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
```

### Enable Profiling
```bash
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DENABLE_PROFILING=ON
```

### Disable AVX2 (for older CPUs)
```bash
cmake -B build -DENABLE_AVX2=OFF
```

## Troubleshooting

### SDL2 Not Found
- Ensure SDL2 development libraries are installed
- Set `SDL2_DIR` environment variable to SDL2 installation path

### OpenGL Issues
- Update graphics drivers
- Ensure OpenGL 3.3+ support

### Compiler Errors
- Verify C++20 support
- Update to latest compiler version