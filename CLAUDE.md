# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project: NOT Gate Sandbox Game

A high-performance logic circuit sandbox where players can build complex systems using NOT gates. The game has two main modes:
1. **Puzzle Mode**: Solve logic challenges using minimal gates
2. **Sandbox Mode**: Build massive circuits capable of simulating computer hardware (100k+ gates)

### Documentation
- [Game Specification](docs/GAME_SPEC.md) - 게임 기능 명세 및 UI 디자인
- [C++ Architecture](docs/CPP_ARCHITECTURE.md) - 기술 스택 및 아키텍처 설계
- [Development Roadmap](docs/ROADMAP.md) - 개발 로드맵 및 현재 진행상황

## Project Context

### Original Request
사용자는 NOT 게이트만을 사용하여 원하는 신호를 출력하는 게임을 요청했습니다. 특히:
- Claude Code에 의존하는 개발 환경
- 극도의 성능 최적화 필요
- 샌드박스 모드에서 컴퓨터 하드웨어를 흉내낼 수 있는 규모

### Key Design Decisions
1. **C++ over TypeScript**: 대규모 회로 시뮬레이션을 위해 네이티브 성능 필요
2. **SDL2 + OpenGL**: 크로스 플랫폼 지원과 하드웨어 가속
3. **Grid-based System**: 각 칸에 하나의 개체만 배치 가능
4. **Signal Delay**: NOT 게이트는 0.1초 출력 딜레이

## Tech Stack

### Core Technologies
- **Language**: C++20
- **Graphics**: SDL2 + OpenGL 3.3+
- **UI**: Dear ImGui
- **Build**: CMake 3.20+
- **Version Control**: Git (GitHub: https://github.com/pkcwpkcwpkcw/notGame.git)

### Performance Requirements
- Handle 100,000+ gates at 60 FPS (normal play)
- Support 1,000,000+ gates (sandbox mode)
- SIMD optimization (AVX2)
- Multithreaded simulation
- Memory-efficient data structures

## Game Mechanics

### NOT Gate
- 입력 3개, 출력 1개의 사각형
- 입력 신호를 반전 (0→1, 1→0)
- 0.1초 출력 딜레이
- 입력 포트로는 신호 전파 안함

### Wire System
- 드래그로 설치
- 기본 상태: 가운데 점
- 인접 칸으로 연결시 신호 전파

### Grid System
- 좌표 기반 관리
- 칸당 하나의 개체만 가능

## Development Status

### Current Stage
프로젝트 초기 설정 완료:
- 문서 작성 완료 ([GAME_SPEC.md](docs/GAME_SPEC.md), [CPP_ARCHITECTURE.md](docs/CPP_ARCHITECTURE.md), [ROADMAP.md](docs/ROADMAP.md))
- CMakeLists.txt 생성
- Git 저장소 초기화 및 GitHub 연결

### Next Steps
[ROADMAP.md](docs/ROADMAP.md)의 Step 1부터 순차적으로 구현:
1. SDL2, OpenGL 윈도우 생성
2. 기본 이벤트 루프
3. Dear ImGui 통합

## Development Commands
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

## Code Structure (Planned)
```
src/
├── main.cpp             # Entry point
├── core/               # Circuit simulation
│   ├── Circuit.h/cpp   # Main circuit logic
│   ├── Gate.h/cpp      # NOT gate implementation
│   └── Signal.h/cpp    # Signal propagation
├── render/             # Rendering system
│   ├── Renderer.h/cpp  # SDL2/OpenGL renderer
│   ├── Camera.h/cpp    # View management
│   └── Grid.h/cpp      # Grid rendering
├── game/               # Game logic
│   ├── GameState.h/cpp # Game state management
│   ├── Level.h/cpp     # Level system
│   └── Sandbox.h/cpp   # Sandbox mode
├── ui/                 # User interface
│   └── UIManager.h/cpp # ImGui integration
└── utils/              # Utilities
    ├── Math.h          # Vector math
    └── Types.h         # Common types
```

## Key Implementation Notes

### Performance Critical
1. Use Structure of Arrays (SoA) for cache efficiency
2. Align data to 64-byte cache lines
3. Batch render calls
4. Use spatial indexing for large circuits

### Signal Simulation
```cpp
// 비트 연산으로 신호 처리
alignas(64) uint32_t signals[MAX_SIGNALS/32];
// NOT 연산: signals[i] = ~signals[i];
```

### Memory Management
- Pre-allocate object pools
- Avoid dynamic allocation in hot paths
- Use custom allocators for frequent objects

## Common Development Tasks

### Adding a New Feature
1. Check [ROADMAP.md](docs/ROADMAP.md) for current phase
2. Update relevant header files first
3. Implement in .cpp files
4. Add to CMakeLists.txt if new files
5. Test with small circuits before optimizing

### Performance Testing
```bash
# Build with profiling
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo

# Run with 100k gates stress test
./build/notgate3 --stress-test 100000
```

## Important Reminders

- **한국어 주석 OK**: 게임 명세나 복잡한 로직 설명시 한국어 사용 가능
- **Performance First**: 편의성보다 성능 우선
- **Test Early**: 대규모 회로 전에 작은 회로로 테스트
- **Git Commits**: 작은 단위로 자주 커밋

## Questions from Previous Session

사용자가 고민했던 주요 결정사항:
1. ✓ C++ vs TypeScript → C++ 선택 (성능)
2. ✓ SDL2 필요성 → SDL2 + OpenGL 선택 (크로스플랫폼)
3. ✓ 순수 C++ → 라이브러리 사용 결정 (개발 속도)