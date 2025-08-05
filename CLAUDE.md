# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project: NOT Gate Sandbox Game

A high-performance logic circuit sandbox where players can build complex systems using NOT gates, potentially simulating computer hardware.

## Tech Stack

### Core Technologies
- **Language**: C++20
- **Graphics**: SDL2
- **UI**: Dear ImGui
- **Build**: CMake
- **Target**: Native desktop (Windows/Mac/Linux)

### Performance Requirements
- Handle 100,000+ gates at 60 FPS
- Support sandbox mode with 1M+ gates
- SIMD optimization (AVX2)
- Multithreaded simulation
- Memory-efficient data structures

### Development Commands
```bash
# Windows (MSVC)
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release

# Linux/Mac
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8

# Run
./build/notgate3         # Linux/Mac
./build/Release/notgate3.exe  # Windows
```

## Code Structure

```
src/
├── core/        # Circuit simulation engine
├── render/      # SDL2 rendering system
├── game/        # Game logic and modes
├── ui/          # ImGui interface
└── utils/       # Threading, memory, profiling
```

## Key Optimization Patterns

1. **SIMD Processing**: Use AVX2 intrinsics for parallel signal processing
2. **Cache Optimization**: Align data structures to cache lines (64 bytes)
3. **Multithreading**: Divide circuit into chunks for parallel simulation
4. **Spatial Indexing**: QuadTree for efficient rendering culling
5. **Memory Pooling**: Pre-allocate gates and wires to avoid allocation overhead

## Building and Dependencies

### Required:
- C++20 compatible compiler
- SDL2 development libraries
- CMake 3.20+

### Platform Notes:
- Windows: Use vcpkg or manually install SDL2
- Linux: `sudo apt install libsdl2-dev`
- Mac: `brew install sdl2`

## Testing Approach

- Unit tests with simple assertions
- Performance benchmarks for critical paths
- Stress tests for large circuits (100k+ gates)

## Important Notes

- Prioritize performance over convenience
- Use profiler before optimizing
- Keep rendering and simulation separate
- Document SIMD code thoroughly