# NOT Gate Sandbox Game

대규모 논리 회로를 NOT 게이트만으로 구성하는 고성능 샌드박스 게임

## 프로젝트 구조

```
notgame3/
├── CMakeLists.txt       # 빌드 설정
├── src/                 # 소스 코드
│   ├── core/           # 회로 시뮬레이션 엔진
│   ├── render/         # SDL2/OpenGL 렌더링
│   ├── game/           # 게임 로직
│   └── ui/             # Dear ImGui 인터페이스
├── assets/             # 리소스 파일
├── docs/               # 문서
│   ├── GAME_SPEC.md    # 게임 기획서
│   └── CPP_ARCHITECTURE.md  # 기술 구조
└── README.md           # 이 파일
```

## 빌드 방법

### 필요 사항
- C++20 컴파일러
- CMake 3.20+
- SDL2
- OpenGL 3.3+

### Windows
```bash
# vcpkg로 SDL2 설치
vcpkg install sdl2

# 빌드
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

### Linux/Mac
```bash
# 의존성 설치
sudo apt install libsdl2-dev  # Linux
brew install sdl2              # Mac

# 빌드
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
```

## 주요 기능

- **대규모 시뮬레이션**: 100,000+ 게이트 실시간 처리
- **샌드박스 모드**: 컴퓨터 하드웨어 수준 회로 구성 가능
- **퍼즐 모드**: 단계별 논리 회로 문제
- **성능 최적화**: SIMD, 멀티스레딩, GPU 가속

## 개발 현황

- [ ] 핵심 엔진 구현
- [ ] 렌더링 시스템
- [ ] 게임 모드
- [ ] UI/UX
- [ ] 최적화