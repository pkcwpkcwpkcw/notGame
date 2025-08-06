# 기본 이벤트 루프 기능 명세서

## 1. 개요

### 1.1 목적
NOT Gate 게임의 핵심 이벤트 처리 시스템을 구현하여 사용자 입력과 게임 상태를 관리하고, 안정적인 게임 실행 환경을 제공한다.

### 1.2 범위
- SDL2 이벤트 시스템 통합
- 게임 루프 아키텍처 구현
- 입력 처리 및 상태 관리
- 프레임 레이트 제어

### 1.3 용어 정의
- **이벤트 루프**: 사용자 입력과 시스템 이벤트를 지속적으로 처리하는 프로그램의 메인 루프
- **델타 타임**: 이전 프레임과 현재 프레임 사이의 시간 간격
- **프레임 레이트**: 초당 화면을 갱신하는 횟수 (FPS)

## 2. 기능 상세

### 2.1 애플리케이션 생명주기 관리

#### 2.1.1 초기화 (Initialize)
**기능 ID**: APP-001  
**설명**: SDL2 및 OpenGL 컨텍스트 초기화, 게임 리소스 로드

**입력**:
- 윈도우 설정 (크기, 제목, 모드)
- OpenGL 버전 요구사항

**처리**:
1. SDL2 서브시스템 초기화 (VIDEO, EVENTS)
2. OpenGL 컨텍스트 생성 (3.3+ Core Profile)
3. GLEW 초기화
4. 기본 OpenGL 상태 설정
5. Dear ImGui 컨텍스트 생성

**출력**:
- 성공: true 반환, 윈도우 표시
- 실패: false 반환, 에러 로그

**예외 처리**:
- SDL 초기화 실패 → 에러 메시지 출력 후 종료
- OpenGL 버전 미지원 → 최소 요구사항 안내

#### 2.1.2 실행 (Run)
**기능 ID**: APP-002  
**설명**: 메인 게임 루프 실행

**입력**: 없음

**처리**:
```
while (running) {
    1. 프레임 시작 시간 기록
    2. 이벤트 처리 (handleEvents)
    3. 게임 상태 업데이트 (update)
    4. 화면 렌더링 (render)
    5. 프레임 레이트 조절
}
```

**출력**: 
- 정상 종료 코드 (0)
- 에러 종료 코드 (1)

#### 2.1.3 종료 (Shutdown)
**기능 ID**: APP-003  
**설명**: 리소스 정리 및 프로그램 종료

**처리**:
1. Dear ImGui 정리
2. OpenGL 컨텍스트 삭제
3. SDL 윈도우 파괴
4. SDL 종료

### 2.2 이벤트 처리 시스템

#### 2.2.1 이벤트 폴링
**기능 ID**: EVT-001  
**설명**: SDL 이벤트 큐에서 이벤트를 가져와 처리

**처리 흐름**:
```cpp
while (SDL_PollEvent(&event)) {
    // ImGui 이벤트 처리
    ImGui_ImplSDL2_ProcessEvent(&event);
    
    // 게임 이벤트 처리
    switch(event.type) {
        case SDL_QUIT: ...
        case SDL_KEYDOWN: ...
        case SDL_MOUSEBUTTONDOWN: ...
        // ...
    }
}
```

#### 2.2.2 윈도우 이벤트 처리
**기능 ID**: EVT-002  
**설명**: 윈도우 관련 이벤트 처리

| 이벤트 | 동작 |
|--------|------|
| SDL_QUIT | running = false |
| WINDOW_RESIZED | 뷰포트 업데이트, glViewport() 호출 |
| WINDOW_MINIMIZED | 렌더링 일시 중지 |
| WINDOW_RESTORED | 렌더링 재개 |
| WINDOW_FOCUS_GAINED | 입력 활성화 |
| WINDOW_FOCUS_LOST | 입력 비활성화 |

#### 2.2.3 키보드 입력 처리
**기능 ID**: EVT-003  
**설명**: 키보드 입력 이벤트 처리 및 상태 관리

**키 매핑**:
| 키 | 동작 | 상태 |
|----|------|------|
| ESC | 메뉴 토글 | 모든 상태 |
| F11 | 전체화면 토글 | 모든 상태 |
| Space | 시뮬레이션 일시정지 | PLAYING |
| Delete | 선택 개체 삭제 | PLAYING, EDITOR |
| F1 | 도움말 표시 | 모든 상태 |
| Tab | UI 토글 | PLAYING |

**상태 추적**:
```cpp
struct KeyboardState {
    bool keys[SDL_NUM_SCANCODES];
    bool prevKeys[SDL_NUM_SCANCODES];
    
    bool isPressed(SDL_Scancode key);
    bool isJustPressed(SDL_Scancode key);
    bool isJustReleased(SDL_Scancode key);
};
```

#### 2.2.4 마우스 입력 처리
**기능 ID**: EVT-004  
**설명**: 마우스 이벤트 처리 및 좌표 변환

**마우스 동작**:
| 버튼/동작 | 기능 | 조건 |
|-----------|------|------|
| 좌클릭 | 개체 선택/배치 | - |
| 좌드래그 | 와이어 그리기 | 포트에서 시작 |
| 우클릭 | 컨텍스트 메뉴 | - |
| 중간버튼 드래그 | 화면 이동 | - |
| 휠 스크롤 | 줌 인/아웃 | Ctrl 미사용 |
| Ctrl+휠 | 빠른 줌 | Ctrl 사용 |

**좌표 변환**:
```cpp
// 스크린 좌표 → 월드 좌표
Vec2 screenToWorld(int screenX, int screenY) {
    // 카메라 변환 적용
    return camera.screenToWorld(screenX, screenY);
}

// 월드 좌표 → 그리드 좌표
GridPos worldToGrid(Vec2 worldPos) {
    return GridPos(
        floor(worldPos.x / GRID_SIZE),
        floor(worldPos.y / GRID_SIZE)
    );
}
```

### 2.3 게임 상태 업데이트

#### 2.3.1 델타 타임 계산
**기능 ID**: UPD-001  
**설명**: 프레임 간 시간 차이 계산

**계산 방식**:
```cpp
uint32_t currentTime = SDL_GetTicks();
deltaTime = (currentTime - lastFrameTime) / 1000.0f;
lastFrameTime = currentTime;

// 델타 타임 제한 (프레임 드롭 방지)
deltaTime = min(deltaTime, 0.1f);  // 최대 100ms
```

#### 2.3.2 상태별 업데이트
**기능 ID**: UPD-002  
**설명**: 현재 게임 상태에 따른 로직 업데이트

```cpp
switch(currentState) {
    case AppState::MENU:
        updateMenu(deltaTime);
        break;
    case AppState::PLAYING:
        updateGame(deltaTime);
        updateSimulation(deltaTime);
        break;
    case AppState::PAUSED:
        // UI만 업데이트, 시뮬레이션 정지
        updateUI(deltaTime);
        break;
    case AppState::EDITOR:
        updateEditor(deltaTime);
        break;
}
```

### 2.4 렌더링 시스템

#### 2.4.1 프레임 렌더링
**기능 ID**: RND-001  
**설명**: 화면 렌더링 및 버퍼 스왑

**렌더링 순서**:
1. 프레임버퍼 클리어
2. 게임 월드 렌더링
3. UI 렌더링 (Dear ImGui)
4. 버퍼 스왑

```cpp
void render() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    // 게임 렌더링
    renderGrid();
    renderGates();
    renderWires();
    
    // ImGui 렌더링
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    SDL_GL_SwapWindow(window);
}
```

### 2.5 프레임 레이트 제어

#### 2.5.1 고정 프레임 레이트
**기능 ID**: FPS-001  
**설명**: 60 FPS 목표로 프레임 시간 조절

**구현 방법**:
```cpp
const uint32_t TARGET_FPS = 60;
const uint32_t FRAME_DELAY = 1000 / TARGET_FPS;  // 16.67ms

void regulateFrameRate() {
    uint32_t frameTime = SDL_GetTicks() - frameStart;
    
    if (frameTime < FRAME_DELAY) {
        SDL_Delay(FRAME_DELAY - frameTime);
    }
}
```

#### 2.5.2 FPS 측정
**기능 ID**: FPS-002  
**설명**: 실시간 FPS 계산 및 표시

```cpp
class FPSCounter {
    uint32_t frameCount = 0;
    float accumulator = 0;
    float currentFPS = 0;
    
    void update(float deltaTime) {
        frameCount++;
        accumulator += deltaTime;
        
        if (accumulator >= 1.0f) {
            currentFPS = frameCount / accumulator;
            frameCount = 0;
            accumulator = 0;
        }
    }
};
```

## 3. 데이터 구조

### 3.1 Application 클래스
```cpp
class Application {
public:
    // 생명주기
    bool initialize(const AppConfig& config);
    void run();
    void shutdown();
    
    // 상태 관리
    void setState(AppState newState);
    AppState getState() const;
    
private:
    // SDL 리소스
    SDL_Window* window;
    SDL_GLContext glContext;
    
    // 상태
    bool running;
    AppState currentState;
    
    // 타이밍
    uint32_t lastFrameTime;
    float deltaTime;
    FPSCounter fpsCounter;
    
    // 입력
    KeyboardState keyboard;
    MouseState mouse;
    
    // 내부 메서드
    void handleEvents();
    void update(float dt);
    void render();
};
```

### 3.2 입력 상태 구조체
```cpp
struct MouseState {
    int x, y;                    // 현재 위치
    int deltaX, deltaY;          // 이동량
    bool buttons[3];             // 좌/우/중간 버튼
    bool prevButtons[3];         // 이전 프레임 상태
    int wheelDelta;              // 휠 스크롤량
    
    bool isButtonPressed(int button);
    bool isButtonJustPressed(int button);
    bool isButtonJustReleased(int button);
};
```

## 4. 인터페이스

### 4.1 외부 인터페이스
```cpp
// main.cpp에서 사용
int main(int argc, char* argv[]) {
    Application app;
    
    AppConfig config;
    config.windowWidth = 1280;
    config.windowHeight = 720;
    config.windowTitle = "NOT Gate Game";
    config.fullscreen = false;
    config.vsync = true;
    
    if (!app.initialize(config)) {
        return -1;
    }
    
    app.run();
    app.shutdown();
    
    return 0;
}
```

### 4.2 이벤트 콜백 인터페이스
```cpp
class IEventListener {
public:
    virtual void onKeyPress(SDL_Scancode key) {}
    virtual void onKeyRelease(SDL_Scancode key) {}
    virtual void onMousePress(int button, int x, int y) {}
    virtual void onMouseRelease(int button, int x, int y) {}
    virtual void onMouseMove(int x, int y) {}
    virtual void onMouseWheel(int delta) {}
    virtual void onWindowResize(int width, int height) {}
};
```

## 5. 성능 사양

### 5.1 타이밍 요구사항
| 항목 | 목표값 | 최소값 | 측정 방법 |
|------|--------|--------|-----------|
| 프레임 레이트 | 60 FPS | 30 FPS | FPSCounter |
| 입력 지연 | < 16ms | < 33ms | 타임스탬프 |
| 이벤트 처리 | < 1ms | < 5ms | 프로파일러 |

### 5.2 메모리 사용
| 컴포넌트 | 예상 사용량 | 최대 사용량 |
|----------|------------|-------------|
| 이벤트 큐 | 1 KB | 10 KB |
| 입력 상태 | 2 KB | 2 KB |
| 프레임 버퍼 | 10 MB | 50 MB |

## 6. 에러 처리

### 6.1 초기화 실패
```cpp
enum class InitError {
    SDL_INIT_FAILED,
    WINDOW_CREATE_FAILED,
    GL_CONTEXT_FAILED,
    GL_VERSION_UNSUPPORTED,
    IMGUI_INIT_FAILED
};

// 에러 발생 시 상세 정보 로그
void logInitError(InitError error) {
    switch(error) {
        case InitError::SDL_INIT_FAILED:
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, 
                        "SDL 초기화 실패: %s", SDL_GetError());
            break;
        // ...
    }
}
```

### 6.2 런타임 에러
| 에러 상황 | 처리 방법 |
|-----------|-----------|
| 프레임 드롭 | 경고 로그, 품질 자동 조절 |
| 이벤트 큐 오버플로우 | 오래된 이벤트 드롭 |
| OpenGL 에러 | 디버그 콜백으로 로그 |
| 메모리 부족 | 안전 모드 전환 |

## 7. 테스트 계획

### 7.1 단위 테스트
- [ ] FPS 카운터 정확도
- [ ] 델타 타임 계산
- [ ] 입력 상태 관리
- [ ] 좌표 변환 함수

### 7.2 통합 테스트
- [ ] 다양한 해상도에서 실행
- [ ] 전체화면 전환
- [ ] 장시간 실행 안정성
- [ ] 다중 입력 동시 처리

### 7.3 성능 테스트
- [ ] 목표 FPS 유지율
- [ ] 입력 응답 시간
- [ ] 메모리 사용량 추이
- [ ] CPU 사용률

## 8. 구현 체크리스트

### Phase 1: 기본 구조 (현재)
- [ ] Application 클래스 생성
- [ ] SDL 초기화 및 윈도우 생성
- [ ] 기본 이벤트 루프
- [ ] SDL_QUIT 처리

### Phase 2: 입력 처리
- [ ] 키보드 상태 관리
- [ ] 마우스 상태 관리
- [ ] 이벤트 디스패처

### Phase 3: 타이밍
- [ ] 델타 타임 계산
- [ ] FPS 카운터
- [ ] 프레임 레이트 제어

### Phase 4: 완성
- [ ] 에러 처리
- [ ] 로깅 시스템
- [ ] 디버그 오버레이

## 9. 참고사항

### 9.1 플랫폼별 고려사항
- **Windows**: WinMain 엔트리 포인트 처리
- **Linux**: X11 이벤트 충돌 방지
- **macOS**: Retina 디스플레이 대응

### 9.2 향후 확장
- 게임패드 지원
- 멀티 윈도우
- 커스텀 이벤트 시스템
- 녹화/재생 기능