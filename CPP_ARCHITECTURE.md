# NOT Gate 게임 - C++/SDL2 아키텍처

## 프로젝트 목표
- **최소 100,000+ 게이트** 실시간 시뮬레이션
- CPU 에뮬레이션 수준의 복잡도 지원
- 60 FPS 유지하면서 대규모 회로 처리

## 핵심 최적화 전략

### 1. 병렬 처리 아키텍처
```cpp
// SIMD + 멀티스레딩으로 극한 성능
class ParallelCircuitSimulator {
private:
    // 64바이트 정렬 (캐시 라인)
    alignas(64) std::atomic<uint64_t> signals[MAX_SIGNAL_BLOCKS];
    
    // 스레드 풀
    ThreadPool workers;
    
    // 회로를 청크로 분할
    struct CircuitChunk {
        uint32_t startIdx;
        uint32_t endIdx;
        std::vector<GateConnection> connections;
    };
    
    std::vector<CircuitChunk> chunks;
};
```

### 2. 메모리 레이아웃 최적화
```cpp
// Structure of Arrays (SoA) 패턴
struct CircuitData {
    // 게이트 데이터 (캐시 친화적)
    std::vector<uint32_t> gateTypes;      // 0 = NOT
    std::vector<uint32_t> gateInputs;     // 입력 신호 인덱스
    std::vector<uint32_t> gateOutputs;    // 출력 신호 인덱스
    
    // 신호 데이터 (SIMD 최적화)
    alignas(32) uint8_t* signalValues;    // AVX2 정렬
    uint32_t signalCount;
    
    // 공간 분할 (렌더링 최적화)
    QuadTree<uint32_t> spatialIndex;
};
```

### 3. 렌더링 파이프라인
```cpp
class OptimizedRenderer {
private:
    SDL_Renderer* renderer;
    SDL_Texture* atlasTexture;     // 게이트 스프라이트 아틀라스
    SDL_Texture* wireTexture;      // 와이어 렌더 타겟
    
    // 렌더 배칭
    struct RenderBatch {
        std::vector<SDL_Rect> srcRects;
        std::vector<SDL_Rect> dstRects;
        SDL_Texture* texture;
    };
    
    // 프러스텀 컬링
    void renderVisible(const Camera& cam) {
        auto visible = spatialIndex.query(cam.bounds);
        // 보이는 것만 렌더링
    }
};
```

## 프로젝트 구조

```
notgame3/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── core/
│   │   ├── Circuit.h/cpp          # 회로 핵심 로직
│   │   ├── SimulationEngine.h/cpp # SIMD 최적화 시뮬레이션
│   │   ├── GateTypes.h            # 게이트 정의
│   │   └── SignalProcessor.h/cpp  # 신호 처리 (AVX2)
│   ├── render/
│   │   ├── Renderer.h/cpp         # SDL2 렌더러
│   │   ├── Camera.h/cpp           # 뷰포트 관리
│   │   ├── BatchRenderer.h/cpp    # 드로우콜 최적화
│   │   └── TextureAtlas.h/cpp     # 텍스처 관리
│   ├── game/
│   │   ├── GameState.h/cpp        # 게임 상태
│   │   ├── LevelSystem.h/cpp      # 레벨 관리
│   │   ├── Sandbox.h/cpp          # 샌드박스 모드
│   │   └── SaveSystem.h/cpp       # 저장/불러오기
│   ├── ui/
│   │   ├── ImGuiIntegration.h/cpp # Dear ImGui UI
│   │   ├── ToolPalette.h/cpp      # 게이트 팔레트
│   │   └── PropertyPanel.h/cpp    # 속성 패널
│   └── utils/
│       ├── ThreadPool.h/cpp       # 멀티스레딩
│       ├── MemoryPool.h/cpp       # 메모리 풀
│       ├── Profiler.h/cpp         # 성능 프로파일러
│       └── Math.h                 # SIMD 수학 함수
├── tests/
│   ├── CircuitTests.cpp
│   ├── PerformanceTests.cpp
│   └── SandboxTests.cpp
└── assets/
    ├── sprites/
    ├── levels/
    └── shaders/
```

## 빌드 시스템 (CMake)

```cmake
cmake_minimum_required(VERSION 3.20)
project(NotGate3 VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 최적화 플래그
if(MSVC)
    add_compile_options(/O2 /arch:AVX2 /fp:fast)
else()
    add_compile_options(-O3 -march=native -ffast-math)
endif()

# 의존성
find_package(SDL2 REQUIRED)
find_package(Threads REQUIRED)

# Dear ImGui (서브모듈)
add_subdirectory(extern/imgui)

# 실행파일
add_executable(notgate3 ${SOURCES})
target_link_libraries(notgate3 
    SDL2::SDL2 
    Threads::Threads
    imgui
)
```

## 성능 목표

### 최소 사양
- 10,000 게이트 @ 60 FPS
- RAM: 2GB
- CPU: 듀얼코어 2.0GHz

### 권장 사양
- 100,000 게이트 @ 60 FPS
- RAM: 8GB
- CPU: 쿼드코어 3.0GHz (AVX2 지원)

### 극한 성능 (샌드박스)
- 1,000,000+ 게이트
- 멀티스레드 시뮬레이션
- GPU 컴퓨트 셰이더 활용 옵션

