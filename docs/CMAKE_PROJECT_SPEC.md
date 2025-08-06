# CMake 프로젝트 구조 상세 명세서

## 1. 프로젝트 개요

### 1.1 기본 정보
- **프로젝트명**: NotGame
- **버전**: 1.0.0
- **CMake 최소 버전**: 3.20
- **C++ 표준**: C++20
- **타겟 플랫폼**: Windows, Linux, macOS

### 1.2 프로젝트 목표
- 크로스 플랫폼 빌드 지원
- 외부 라이브러리 의존성 관리 자동화
- 디버그/릴리즈 빌드 구성
- 단위 테스트 통합
- CPack을 통한 패키징 지원

## 2. 디렉토리 구조

```
notgame3/
├── CMakeLists.txt                 # 루트 CMake 파일
├── cmake/                          # CMake 모듈 및 설정
│   ├── FindSDL2.cmake             # SDL2 찾기 모듈
│   ├── FindOpenGL.cmake           # OpenGL 찾기 모듈  
│   ├── CompilerOptions.cmake      # 컴파일러 옵션 설정
│   ├── Dependencies.cmake         # 의존성 관리
│   └── Packaging.cmake            # CPack 패키징 설정
├── extern/                         # 외부 라이브러리 (서브모듈)
│   ├── imgui/                     # Dear ImGui
│   │   └── CMakeLists.txt        
│   ├── glm/                       # GLM (수학 라이브러리)
│   │   └── CMakeLists.txt
│   └── json/                      # JSON 파서
│       └── CMakeLists.txt
├── src/                            # 소스 코드
│   ├── CMakeLists.txt             # src 디렉토리 CMake
│   ├── main.cpp                   # 진입점
│   ├── core/                      # 핵심 로직
│   │   ├── CMakeLists.txt        # core 라이브러리 빌드
│   │   ├── Circuit.h
│   │   ├── Circuit.cpp
│   │   ├── Gate.h
│   │   ├── Gate.cpp
│   │   ├── Wire.h
│   │   ├── Wire.cpp
│   │   ├── Signal.h
│   │   ├── Signal.cpp
│   │   └── SimulationEngine.h/cpp
│   ├── render/                    # 렌더링 시스템
│   │   ├── CMakeLists.txt        # render 라이브러리 빌드
│   │   ├── Renderer.h
│   │   ├── Renderer.cpp
│   │   ├── Camera.h
│   │   ├── Camera.cpp
│   │   ├── Grid.h
│   │   ├── Grid.cpp
│   │   ├── Shader.h
│   │   └── Shader.cpp
│   ├── game/                      # 게임 로직
│   │   ├── CMakeLists.txt        # game 라이브러리 빌드
│   │   ├── GameState.h
│   │   ├── GameState.cpp
│   │   ├── Level.h
│   │   ├── Level.cpp
│   │   ├── PuzzleMode.h
│   │   ├── PuzzleMode.cpp
│   │   ├── SandboxMode.h
│   │   └── SandboxMode.cpp
│   ├── ui/                        # UI 시스템
│   │   ├── CMakeLists.txt        # ui 라이브러리 빌드
│   │   ├── UIManager.h
│   │   ├── UIManager.cpp
│   │   ├── ToolPalette.h
│   │   ├── ToolPalette.cpp
│   │   └── ImGuiHelpers.h/cpp
│   └── utils/                     # 유틸리티
│       ├── CMakeLists.txt        # utils 라이브러리 빌드
│       ├── Math.h
│       ├── Types.h
│       ├── Timer.h
│       ├── Timer.cpp
│       ├── FileSystem.h
│       ├── FileSystem.cpp
│       └── Logger.h/cpp
├── tests/                          # 테스트
│   ├── CMakeLists.txt             # 테스트 빌드 설정
│   ├── test_main.cpp              # 테스트 진입점
│   ├── core/
│   │   ├── CircuitTest.cpp
│   │   ├── GateTest.cpp
│   │   └── SignalTest.cpp
│   ├── render/
│   │   └── CameraTest.cpp
│   └── performance/
│       └── SimulationBenchmark.cpp
├── assets/                         # 게임 리소스
│   ├── sprites/
│   │   ├── gates/
│   │   └── ui/
│   ├── shaders/
│   │   ├── basic.vert
│   │   ├── basic.frag
│   │   ├── grid.vert
│   │   └── grid.frag
│   ├── levels/
│   │   └── *.json
│   └── fonts/
│       └── *.ttf
├── docs/                           # 문서
│   └── *.md
└── scripts/                        # 빌드 스크립트
    ├── build_windows.bat
    ├── build_linux.sh
    └── build_macos.sh
```

## 3. CMakeLists.txt 상세 명세

### 3.1 루트 CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.20)
project(NotGate3 
    VERSION 1.0.0
    DESCRIPTION "High-performance NOT gate logic circuit sandbox"
    LANGUAGES CXX C
)

# C++ 표준 설정
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 빌드 타입 기본값 설정
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# 옵션 설정
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_DOCS "Build documentation" OFF)
option(ENABLE_PROFILING "Enable profiling support" OFF)
option(USE_NATIVE_ARCH "Use native architecture optimizations" ON)

# 모듈 경로 추가
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# 컴파일러 옵션 로드
include(CompilerOptions)

# 의존성 찾기
include(Dependencies)

# 출력 디렉토리 설정
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 서브디렉토리 추가
add_subdirectory(extern)
add_subdirectory(src)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 패키징 설정
include(Packaging)

# 설치 규칙
install(DIRECTORY assets/ DESTINATION ${CMAKE_INSTALL_PREFIX}/assets)
```

### 3.2 cmake/CompilerOptions.cmake
```cmake
# 플랫폼별 컴파일 옵션 설정
if(MSVC)
    # Windows (MSVC)
    add_compile_options(
        /W4                 # 경고 레벨 4
        /WX-                # 경고를 오류로 처리하지 않음
        /MP                 # 멀티프로세서 컴파일
        /permissive-        # 표준 준수 모드
        /Zc:__cplusplus     # __cplusplus 매크로 올바르게 설정
    )
    
    # 릴리즈 최적화
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(
            /O2                 # 속도 최적화
            /GL                 # 전체 프로그램 최적화
            /arch:AVX2          # AVX2 명령어 사용
            /fp:fast            # 빠른 부동소수점 연산
        )
        add_link_options(/LTCG)  # 링크 시간 코드 생성
    endif()
    
    # 디버그 옵션
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(
            /Od                 # 최적화 비활성화
            /RTC1               # 런타임 체크
            /Zi                 # 디버그 정보 생성
        )
    endif()
    
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Linux/macOS (GCC/Clang)
    add_compile_options(
        -Wall               # 모든 경고 활성화
        -Wextra             # 추가 경고
        -Wpedantic          # 표준 준수 경고
        -Wno-unused-parameter
    )
    
    # 릴리즈 최적화
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(
            -O3                 # 최고 수준 최적화
            -flto               # 링크 시간 최적화
            -ffast-math         # 빠른 수학 연산
        )
        
        if(USE_NATIVE_ARCH)
            add_compile_options(-march=native)  # 네이티브 아키텍처 최적화
        else()
            add_compile_options(-march=x86-64-v3)  # AVX2 지원
        endif()
    endif()
    
    # 디버그 옵션
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(
            -O0                 # 최적화 비활성화
            -g3                 # 최대 디버그 정보
            -fsanitize=address  # 주소 새니타이저
            -fsanitize=undefined # UB 새니타이저
        )
        add_link_options(-fsanitize=address -fsanitize=undefined)
    endif()
endif()

# 프로파일링 옵션
if(ENABLE_PROFILING)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-pg -fno-omit-frame-pointer)
        add_link_options(-pg)
    endif()
endif()
```

### 3.3 cmake/Dependencies.cmake
```cmake
# 스레드 라이브러리
find_package(Threads REQUIRED)

# SDL2
find_package(SDL2 REQUIRED)
if(NOT SDL2_FOUND)
    message(FATAL_ERROR "SDL2 not found! Please install SDL2 development libraries.")
endif()

# OpenGL
find_package(OpenGL REQUIRED)
if(NOT OpenGL_FOUND)
    message(FATAL_ERROR "OpenGL not found!")
endif()

# GLAD (OpenGL 로더) - 수동 포함
set(GLAD_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/glad/include")
set(GLAD_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/extern/glad/src/glad.c")

# 플랫폼별 추가 라이브러리
if(WIN32)
    # Windows 특정 라이브러리
    set(PLATFORM_LIBS winmm imm32 version setupapi)
elseif(APPLE)
    # macOS 특정 프레임워크
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    set(PLATFORM_LIBS ${COCOA_LIBRARY} ${IOKIT_LIBRARY} ${COREVIDEO_LIBRARY})
elseif(UNIX)
    # Linux 특정 라이브러리
    find_package(X11 REQUIRED)
    set(PLATFORM_LIBS ${X11_LIBRARIES} dl)
endif()

# 테스트용 라이브러리 (옵션)
if(BUILD_TESTS)
    # Google Test 또는 Catch2
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.4.0
    )
    FetchContent_MakeAvailable(Catch2)
endif()
```

### 3.4 src/CMakeLists.txt
```cmake
# 소스 파일 수집
file(GLOB_RECURSE CORE_SOURCES "core/*.cpp")
file(GLOB_RECURSE RENDER_SOURCES "render/*.cpp")
file(GLOB_RECURSE GAME_SOURCES "game/*.cpp")
file(GLOB_RECURSE UI_SOURCES "ui/*.cpp")
file(GLOB_RECURSE UTILS_SOURCES "utils/*.cpp")

# 정적 라이브러리 생성
add_library(notgate_core STATIC ${CORE_SOURCES})
add_library(notgate_render STATIC ${RENDER_SOURCES})
add_library(notgate_game STATIC ${GAME_SOURCES})
add_library(notgate_ui STATIC ${UI_SOURCES})
add_library(notgate_utils STATIC ${UTILS_SOURCES})

# 각 라이브러리에 포함 디렉토리 설정
target_include_directories(notgate_core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/extern
)

target_include_directories(notgate_render PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SDL2_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
    ${GLAD_INCLUDE_DIR}
)

target_include_directories(notgate_game PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/extern/json/include
)

target_include_directories(notgate_ui PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/extern/imgui
)

target_include_directories(notgate_utils PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 라이브러리 링크
target_link_libraries(notgate_render 
    ${SDL2_LIBRARIES}
    ${OPENGL_LIBRARIES}
    notgate_utils
)

target_link_libraries(notgate_game 
    notgate_core
    notgate_utils
)

target_link_libraries(notgate_ui 
    imgui
    notgate_render
)

# 실행 파일 생성
add_executable(notgate3 main.cpp ${GLAD_SOURCE})

# 실행 파일 링크
target_link_libraries(notgate3 
    notgate_core
    notgate_render
    notgate_game
    notgate_ui
    notgate_utils
    ${PLATFORM_LIBS}
    Threads::Threads
)

# 실행 파일 포함 디렉토리
target_include_directories(notgate3 PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GLAD_INCLUDE_DIR}
)

# 리소스 파일 복사 (개발 중)
add_custom_command(TARGET notgate3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:notgate3>/assets
)
```

### 3.5 tests/CMakeLists.txt
```cmake
# 테스트 소스 파일
file(GLOB_RECURSE TEST_SOURCES "*.cpp")

# 테스트 실행 파일
add_executable(notgate_tests ${TEST_SOURCES})

# 테스트 링크
target_link_libraries(notgate_tests
    notgate_core
    notgate_render
    notgate_game
    notgate_utils
    Catch2::Catch2WithMain
)

# 테스트 포함 디렉토리
target_include_directories(notgate_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# CTest 등록
include(CTest)
include(Catch)
catch_discover_tests(notgate_tests)

# 벤치마크 테스트 (별도)
if(ENABLE_PROFILING)
    add_executable(notgate_benchmark 
        performance/SimulationBenchmark.cpp
    )
    target_link_libraries(notgate_benchmark
        notgate_core
        notgate_utils
    )
endif()
```

### 3.6 extern/imgui/CMakeLists.txt
```cmake
# Dear ImGui 라이브러리
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})

target_include_directories(imgui PUBLIC 
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(imgui PUBLIC ${SDL2_LIBRARIES})

# OpenGL 3.3 정의
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)
```

## 4. 빌드 명령어

### 4.1 Windows (Visual Studio)
```batch
# 빌드 디렉토리 생성 및 프로젝트 생성
cmake -B build -G "Visual Studio 17 2022" -A x64

# 디버그 빌드
cmake --build build --config Debug

# 릴리즈 빌드
cmake --build build --config Release

# 테스트 실행
ctest --test-dir build -C Release

# 설치
cmake --install build --config Release
```

### 4.2 Linux/macOS
```bash
# 빌드 디렉토리 생성 및 프로젝트 생성
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 병렬 빌드
cmake --build build -j$(nproc)

# 테스트 실행
ctest --test-dir build

# 설치
sudo cmake --install build
```

### 4.3 고급 옵션
```bash
# 네이티브 최적화 비활성화
cmake -B build -DUSE_NATIVE_ARCH=OFF

# 테스트 비활성화
cmake -B build -DBUILD_TESTS=OFF

# 프로파일링 활성화
cmake -B build -DENABLE_PROFILING=ON

# Clang 컴파일러 사용
cmake -B build -DCMAKE_CXX_COMPILER=clang++
```

## 5. 의존성 설치 가이드

### 5.1 Windows
```powershell
# vcpkg 사용
vcpkg install sdl2:x64-windows

# 또는 수동 다운로드
# SDL2: https://www.libsdl.org/download-2.0.php
# 다운로드 후 CMAKE_PREFIX_PATH 설정
```

### 5.2 Ubuntu/Debian
```bash
# 시스템 패키지 관리자 사용
sudo apt-get update
sudo apt-get install \
    libsdl2-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    build-essential \
    cmake
```

### 5.3 macOS
```bash
# Homebrew 사용
brew install sdl2 cmake

# Xcode Command Line Tools 필요
xcode-select --install
```

### 5.4 Arch Linux
```bash
# Pacman 사용
sudo pacman -S sdl2 mesa cmake base-devel
```

## 6. CI/CD 통합

### 6.1 GitHub Actions 예제
```yaml
name: Build

on: [push, pull_request]

jobs:
  build:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-latest, windows-latest, macos-latest]
        build_type: [Debug, Release]
    
    steps:
    - uses: actions/checkout@v3
      with:
        submodules: recursive
    
    - name: Install Dependencies (Ubuntu)
      if: runner.os == 'Linux'
      run: |
        sudo apt-get update
        sudo apt-get install -y libsdl2-dev libgl1-mesa-dev
    
    - name: Install Dependencies (macOS)
      if: runner.os == 'macOS'
      run: brew install sdl2
    
    - name: Configure CMake
      run: cmake -B build -DCMAKE_BUILD_TYPE=${{ matrix.build_type }}
    
    - name: Build
      run: cmake --build build --config ${{ matrix.build_type }}
    
    - name: Test
      run: ctest --test-dir build -C ${{ matrix.build_type }}
```

## 7. 패키징 (CPack)

### 7.1 cmake/Packaging.cmake
```cmake
include(CPack)

set(CPACK_PACKAGE_NAME "NotGate3")
set(CPACK_PACKAGE_VENDOR "NotGate Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High-performance NOT gate logic circuit sandbox")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "NotGate3")

# 플랫폼별 패키지 생성기
if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_DISPLAY_NAME "NotGate3")
    set(CPACK_NSIS_PACKAGE_NAME "NotGate3")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/pkcwpkcwpkcw/notGame")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop;TGZ")
    set(CPACK_DMG_VOLUME_NAME "NotGate3")
elseif(UNIX)
    set(CPACK_GENERATOR "TGZ;DEB;RPM")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsdl2-2.0-0")
    set(CPACK_RPM_PACKAGE_REQUIRES "SDL2")
endif()

# 포함할 파일
set(CPACK_PACKAGE_FILES
    notgate3
    assets/
    README.md
    LICENSE
)

# 제외할 파일
set(CPACK_SOURCE_IGNORE_FILES
    /\\.git/
    /build/
    /\\.vscode/
    /\\.idea/
    \\.DS_Store
)
```

### 7.2 패키지 생성 명령
```bash
# 빌드 후 패키지 생성
cmake --build build --config Release
cd build
cpack -C Release

# 특정 생성기만 사용
cpack -G ZIP -C Release
cpack -G DEB -C Release
```

## 8. 개발 팁

### 8.1 ccache 사용 (빌드 속도 향상)
```bash
# Linux/macOS
sudo apt-get install ccache  # Ubuntu
brew install ccache          # macOS

# CMake에서 사용
cmake -B build -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
```

### 8.2 Ninja 빌드 시스템 사용
```bash
# 더 빠른 빌드를 위해 Ninja 사용
cmake -B build -G Ninja
ninja -C build
```

### 8.3 미리 컴파일된 헤더 (PCH)
```cmake
# src/CMakeLists.txt에 추가
target_precompile_headers(notgate_core PRIVATE
    <vector>
    <memory>
    <algorithm>
    <cstdint>
)
```

### 8.4 Unity 빌드 (Jumbo 빌드)
```cmake
# 컴파일 시간 단축
set_target_properties(notgate_core PROPERTIES UNITY_BUILD ON)
```

## 9. 문제 해결

### 9.1 SDL2를 찾을 수 없는 경우
```bash
# CMAKE_PREFIX_PATH 설정
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/sdl2

# 또는 환경변수 설정
export SDL2_DIR=/path/to/sdl2
cmake -B build
```

### 9.2 링크 오류
```cmake
# verbose 모드로 빌드하여 상세 정보 확인
cmake --build build --verbose

# 또는 make 사용시
make VERBOSE=1
```

### 9.3 Windows에서 DLL 문제
```cmake
# DLL 자동 복사
add_custom_command(TARGET notgate3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL2::SDL2>
    $<TARGET_FILE_DIR:notgate3>
)
```

## 10. 성능 프로파일링 통합

### 10.1 Tracy Profiler 통합
```cmake
# extern/CMakeLists.txt에 추가
if(ENABLE_PROFILING)
    FetchContent_Declare(
        tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy.git
        GIT_TAG v0.9.1
    )
    FetchContent_MakeAvailable(tracy)
    
    target_link_libraries(notgate_core PUBLIC TracyClient)
    target_compile_definitions(notgate_core PUBLIC TRACY_ENABLE)
endif()
```

### 10.2 프로파일링 매크로
```cpp
// utils/Profiler.h
#ifdef TRACY_ENABLE
    #include <tracy/Tracy.hpp>
    #define PROFILE_SCOPE(name) ZoneScopedN(name)
    #define PROFILE_FRAME() FrameMark
#else
    #define PROFILE_SCOPE(name)
    #define PROFILE_FRAME()
#endif
```

이 명세서는 NotGate3 프로젝트의 완전한 CMake 구조를 정의합니다. 각 파일의 역할과 내용이 명확히 정의되어 있어, 이를 기반으로 실제 프로젝트를 구축할 수 있습니다.