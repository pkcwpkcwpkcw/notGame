# NOT Gate Game - 프로젝트 폴더 구조

```
notgame3/
├── CMakeLists.txt                 # 최상위 빌드 설정
├── README.md                      # 프로젝트 개요
├── CLAUDE.md                      # Claude Code 가이드
├── ROADMAP.md                     # 개발 로드맵
├── GAME_SPEC.md                   # 게임 기획서
├── CPP_ARCHITECTURE.md            # 기술 아키텍처
├── .gitignore                     # Git 제외 파일
├── .clang-format                  # 코드 스타일 설정
│
├── src/                           # 소스 코드
│   ├── main.cpp                   # 프로그램 진입점
│   │
│   ├── core/                      # 핵심 엔진
│   │   ├── Circuit.h              # 회로 관리
│   │   ├── Circuit.cpp
│   │   ├── Gate.h                 # NOT 게이트
│   │   ├── Gate.cpp
│   │   ├── Wire.h                 # 와이어 연결
│   │   ├── Wire.cpp
│   │   ├── Signal.h               # 신호 전파
│   │   ├── Signal.cpp
│   │   ├── Grid.h                 # 그리드 시스템
│   │   ├── Grid.cpp
│   │   └── SimulationEngine.h     # 시뮬레이션 엔진
│   │       └── SimulationEngine.cpp
│   │
│   ├── render/                    # 렌더링 시스템
│   │   ├── Renderer.h             # 메인 렌더러
│   │   ├── Renderer.cpp
│   │   ├── Camera.h               # 카메라/뷰포트
│   │   ├── Camera.cpp
│   │   ├── GridRenderer.h         # 그리드 렌더링
│   │   ├── GridRenderer.cpp
│   │   ├── GateRenderer.h         # 게이트 렌더링
│   │   ├── GateRenderer.cpp
│   │   ├── WireRenderer.h         # 와이어 렌더링
│   │   ├── WireRenderer.cpp
│   │   ├── BatchRenderer.h        # 배치 렌더링
│   │   ├── BatchRenderer.cpp
│   │   └── Shaders.h              # 셰이더 관리
│   │       └── Shaders.cpp
│   │
│   ├── game/                      # 게임 로직
│   │   ├── GameState.h            # 게임 상태 관리
│   │   ├── GameState.cpp
│   │   ├── InputHandler.h         # 입력 처리
│   │   ├── InputHandler.cpp
│   │   ├── Level.h                # 레벨 시스템
│   │   ├── Level.cpp
│   │   ├── LevelLoader.h          # 레벨 로딩
│   │   ├── LevelLoader.cpp
│   │   ├── PuzzleMode.h           # 퍼즐 모드
│   │   ├── PuzzleMode.cpp
│   │   ├── SandboxMode.h          # 샌드박스 모드
│   │   ├── SandboxMode.cpp
│   │   └── SaveSystem.h           # 저장/불러오기
│   │       └── SaveSystem.cpp
│   │
│   ├── ui/                        # 사용자 인터페이스
│   │   ├── UIManager.h            # UI 관리자
│   │   ├── UIManager.cpp
│   │   ├── MainMenu.h             # 메인 메뉴
│   │   ├── MainMenu.cpp
│   │   ├── GameHUD.h              # 게임 HUD
│   │   ├── GameHUD.cpp
│   │   ├── ToolPalette.h          # 도구 팔레트
│   │   ├── ToolPalette.cpp
│   │   ├── PropertyPanel.h        # 속성 패널
│   │   ├── PropertyPanel.cpp
│   │   └── LevelEditor.h          # 레벨 에디터 UI
│   │       └── LevelEditor.cpp
│   │
│   └── utils/                     # 유틸리티
│       ├── Types.h                # 공통 타입 정의
│       ├── Math.h                 # 수학 함수/벡터
│       ├── Math.cpp
│       ├── ThreadPool.h           # 스레드 풀
│       ├── ThreadPool.cpp
│       ├── MemoryPool.h           # 메모리 풀
│       ├── MemoryPool.cpp
│       ├── Profiler.h             # 성능 프로파일러
│       ├── Profiler.cpp
│       └── Logger.h               # 로깅 시스템
│           └── Logger.cpp
│
├── external/                      # 외부 라이브러리
│   ├── imgui/                     # Dear ImGui (서브모듈)
│   ├── json/                      # JSON 파서 (nlohmann/json)
│   └── stb/                       # stb 이미지 로더
│
├── assets/                        # 게임 리소스
│   ├── textures/                  # 텍스처
│   │   ├── gates/                 # 게이트 스프라이트
│   │   │   └── not_gate.png
│   │   ├── ui/                    # UI 요소
│   │   └── grid.png               # 그리드 텍스처
│   │
│   ├── shaders/                   # GLSL 셰이더
│   │   ├── basic.vert             # 기본 vertex 셰이더
│   │   ├── basic.frag             # 기본 fragment 셰이더
│   │   ├── instanced.vert         # 인스턴싱 셰이더
│   │   └── wire.frag              # 와이어 셰이더
│   │
│   ├── levels/                    # 레벨 데이터
│   │   ├── tutorial/              # 튜토리얼 레벨
│   │   │   ├── level_01.json
│   │   │   └── ...
│   │   ├── beginner/              # 초급 레벨
│   │   ├── intermediate/          # 중급 레벨
│   │   └── advanced/              # 고급 레벨
│   │
│   ├── fonts/                     # 폰트
│   │   └── default.ttf
│   │
│   └── config/                    # 설정 파일
│       └── default_settings.json
│
├── tests/                         # 테스트 코드
│   ├── CMakeLists.txt
│   ├── test_main.cpp
│   ├── core/                      # 코어 테스트
│   │   ├── test_circuit.cpp
│   │   ├── test_gate.cpp
│   │   └── test_signal.cpp
│   ├── render/                    # 렌더링 테스트
│   └── performance/               # 성능 테스트
│       ├── stress_test.cpp
│       └── benchmark.cpp
│
├── docs/                          # 추가 문서
│   ├── BUILDING.md                # 빌드 가이드
│   ├── CONTRIBUTING.md            # 기여 가이드
│   └── API.md                     # API 문서
│
├── scripts/                       # 빌드/유틸리티 스크립트
│   ├── setup_dependencies.sh      # 의존성 설치
│   ├── build_release.sh           # 릴리스 빌드
│   └── package.sh                 # 패키징 스크립트
│
└── build/                         # 빌드 출력 (gitignore)
    ├── Debug/
    └── Release/
```

## 주요 설계 포인트

### 1. 모듈화
- 각 시스템을 독립적인 폴더로 분리
- 헤더와 구현 파일 쌍으로 구성
- 의존성 최소화

### 2. 확장성
- 새로운 게이트 타입 추가 고려
- 렌더러 분리로 다양한 렌더링 방식 지원
- 게임 모드 확장 가능

### 3. 성능
- 핫 패스 코드는 core/에 집중
- utils/에 최적화 도구 배치
- 테스트에 성능 벤치마크 포함

### 4. 개발 편의성
- 명확한 폴더 구조
- 기능별 그룹화
- 테스트 코드 분리