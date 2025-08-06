# SDL2 종속성 관리 가이드

## 배포를 위한 SDL2 관리 방법

### 방법 1: vcpkg 사용 (권장) ⭐
프로젝트와 함께 종속성을 자동 관리하는 가장 깔끔한 방법입니다.

#### 1.1 vcpkg 매니페스트 모드 설정
```json
// vcpkg.json (프로젝트 루트에 생성)
{
  "name": "notgame",
  "version": "1.0.0",
  "dependencies": [
    "sdl2",
    "opengl",
    "imgui[sdl2-binding,opengl3-binding]",
    "glm",
    "nlohmann-json"
  ]
}
```

#### 1.2 CMake 설정
```cmake
# CMakeLists.txt에 추가
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()
```

#### 1.3 빌드 및 배포
```bash
# vcpkg가 자동으로 종속성 다운로드 및 빌드
cmake -B build -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build

# 배포 시 필요한 DLL이 자동으로 bin 폴더에 복사됨
```

### 방법 2: FetchContent (CMake 내장) 🔧
CMake가 빌드 시 자동으로 SDL2 소스를 다운로드하고 빌드합니다.

```cmake
# cmake/FetchSDL2.cmake
include(FetchContent)

FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.28.5  # 최신 안정 버전
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(SDL2)

# SDL2를 정적 라이브러리로 빌드 (단일 실행 파일)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
```

### 방법 3: 서브모듈 (Git Submodule) 📦
프로젝트 저장소에 SDL2를 서브모듈로 포함합니다.

```bash
# SDL2를 서브모듈로 추가
git submodule add https://github.com/libsdl-org/SDL.git extern/SDL2
git submodule update --init --recursive

# CMakeLists.txt에서
add_subdirectory(extern/SDL2)
```

### 방법 4: 사전 빌드된 바이너리 포함 📁
가장 간단하지만 플랫폼별로 관리해야 합니다.

```
extern/
├── SDL2/
│   ├── include/       # 헤더 파일
│   ├── lib/
│   │   ├── x64/      # 64비트 라이브러리
│   │   │   ├── SDL2.lib
│   │   │   └── SDL2main.lib
│   │   └── x86/      # 32비트 라이브러리
│   └── bin/
│       ├── x64/
│       │   └── SDL2.dll
│       └── x86/
│           └── SDL2.dll
```

## 배포 시나리오별 권장 방법

### 1. Steam/itch.io 배포 (일반 사용자)
- **권장**: 방법 4 (사전 빌드 바이너리)
- **이유**: 빠른 빌드, 안정적, DLL 포함 간단
- **배포**: 실행 파일과 SDL2.dll을 함께 배포

### 2. 오픈소스 프로젝트 (개발자 대상)
- **권장**: 방법 1 (vcpkg) 또는 방법 2 (FetchContent)
- **이유**: 자동화된 빌드, 크로스 플랫폼
- **배포**: 소스 코드만 배포, 사용자가 빌드

### 3. 단일 실행 파일 배포
- **권장**: 방법 2 (FetchContent) + 정적 링크
- **이유**: DLL 없이 단일 .exe 파일
- **주의**: 실행 파일 크기 증가 (약 2-3MB)

## 실제 구현 예제

### CMakeLists.txt 수정
```cmake
# 옵션 추가
option(USE_BUNDLED_SDL2 "Use bundled SDL2" ON)
option(SDL2_STATIC_LINK "Static link SDL2" OFF)

if(USE_BUNDLED_SDL2)
    # FetchContent 사용
    include(cmake/FetchSDL2.cmake)
else()
    # 시스템 SDL2 찾기
    find_package(SDL2 REQUIRED)
endif()

# 배포 빌드 설정
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # Windows: DLL 복사
    if(WIN32 AND NOT SDL2_STATIC_LINK)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL2::SDL2>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        )
    endif()
endif()
```

### 배포 스크립트
```cmake
# cmake/Package.cmake
install(TARGETS notgame DESTINATION .)

if(WIN32)
    # SDL2 DLL 설치
    install(FILES $<TARGET_FILE:SDL2::SDL2> DESTINATION .)
    
    # Visual C++ 재배포 가능 패키지
    include(InstallRequiredSystemLibraries)
    
    # 리소스 파일
    install(DIRECTORY assets/ DESTINATION assets)
endif()

# CPack 설정
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-win64")
include(CPack)
```

## 빌드 및 패키징 명령

```bash
# 릴리즈 빌드
cmake -B build -DCMAKE_BUILD_TYPE=Release -DUSE_BUNDLED_SDL2=ON
cmake --build build --config Release

# 패키지 생성 (ZIP)
cd build
cpack -C Release

# 결과: notgame-1.0.0-win64.zip
#   ├── notgame.exe
#   ├── SDL2.dll
#   └── assets/
```

## 라이선스 고려사항

### SDL2 라이선스 (zlib)
- 상업적 사용 가능 ✅
- 수정 가능 ✅
- 라이선스 표시 필요 ✅

라이선스 파일을 배포판에 포함:
```
licenses/
├── SDL2-LICENSE.txt
└── README.txt
```

## 권장 사항

1. **개발 중**: 시스템 SDL2 사용 (빠른 빌드)
2. **CI/CD**: vcpkg 또는 FetchContent (자동화)
3. **최종 배포**: 
   - Windows: DLL 포함 배포
   - Linux: 패키지 관리자 의존성
   - macOS: 앱 번들에 포함

## 체크리스트

- [ ] SDL2 포함 방법 결정
- [ ] CMakeLists.txt 수정
- [ ] 배포 스크립트 작성
- [ ] 라이선스 파일 포함
- [ ] 플랫폼별 테스트
- [ ] 패키지 생성 자동화