# 기본 이벤트 루프 요구사항 명세서

## 개요
SDL2 기반 게임의 핵심 이벤트 처리 시스템 구현을 위한 요구사항 정의

## 목적
- 안정적인 프레임 레이트 유지 (60 FPS 목표)
- 사용자 입력의 즉각적인 반응
- 게임 상태 업데이트와 렌더링의 분리
- 리소스 효율적인 이벤트 처리

## 기능 요구사항

### 1. 이벤트 처리 (Event Handling)

#### 1.1 윈도우 이벤트
- **SDL_QUIT**: 프로그램 종료 처리
- **SDL_WINDOWEVENT**: 
  - RESIZED: 윈도우 크기 변경 시 뷰포트 업데이트
  - MINIMIZED/MAXIMIZED: 최소화/최대화 상태 처리
  - FOCUS_GAINED/LOST: 포커스 상태 추적

#### 1.2 키보드 입력
- **SDL_KEYDOWN/KEYUP**: 키 입력 상태 추적
- **필수 단축키**:
  - ESC: 일시정지 또는 메뉴 열기
  - F11: 전체화면 토글
  - Space: 시뮬레이션 일시정지/재개
  - Delete: 선택된 개체 삭제
  - Ctrl+Z/Y: 실행 취소/다시 실행 (향후 구현)

#### 1.3 마우스 입력
- **SDL_MOUSEMOTION**: 마우스 위치 추적
- **SDL_MOUSEBUTTONDOWN/UP**: 클릭 상태 처리
  - 좌클릭: 선택/배치
  - 우클릭: 컨텍스트 메뉴 또는 취소
  - 중간 버튼: 화면 이동 (팬)
- **SDL_MOUSEWHEEL**: 줌 인/아웃

### 2. 게임 루프 구조

#### 2.1 기본 구조
```cpp
class Application {
    bool running = true;
    uint32_t targetFPS = 60;
    uint32_t frameDelay = 1000 / targetFPS;
    
    void run() {
        uint32_t frameStart;
        int frameTime;
        
        while (running) {
            frameStart = SDL_GetTicks();
            
            handleEvents();
            update(deltaTime);
            render();
            
            frameTime = SDL_GetTicks() - frameStart;
            if (frameDelay > frameTime) {
                SDL_Delay(frameDelay - frameTime);
            }
        }
    }
};
```

#### 2.2 타이밍 시스템
- **고정 타임스텝**: 시뮬레이션 일관성 보장
- **가변 렌더링**: 부드러운 시각적 표현
- **델타 타임**: 프레임 독립적 애니메이션

### 3. 상태 관리

#### 3.1 애플리케이션 상태
```cpp
enum class AppState {
    MENU,       // 메인 메뉴
    PLAYING,    // 게임 진행 중
    PAUSED,     // 일시정지
    EDITOR      // 레벨 에디터
};
```

#### 3.2 입력 상태
```cpp
struct InputState {
    bool keys[SDL_NUM_SCANCODES];
    bool mouseButtons[3];
    int mouseX, mouseY;
    int mouseDeltaX, mouseDeltaY;
    int wheelDelta;
};
```

### 4. 성능 요구사항

#### 4.1 프레임 레이트
- **목표**: 60 FPS 일정 유지
- **최소**: 30 FPS (대규모 회로 시)
- **측정**: FPS 카운터 구현

#### 4.2 입력 지연
- **최대 지연**: 16.67ms (1 프레임)
- **이벤트 큐**: 오버플로우 방지

#### 4.3 CPU 사용률
- **유휴 상태**: < 5% CPU 사용
- **활성 상태**: 효율적인 코어 활용

## 구현 세부사항

### 1. 클래스 설계

```cpp
// Application.h
class Application {
private:
    SDL_Window* window;
    SDL_GLContext glContext;
    
    bool running;
    AppState currentState;
    InputState input;
    
    uint32_t lastFrameTime;
    float deltaTime;
    
public:
    Application();
    ~Application();
    
    bool initialize();
    void run();
    void shutdown();
    
private:
    void handleEvents();
    void update(float dt);
    void render();
    
    void processWindowEvent(const SDL_WindowEvent& e);
    void processKeyEvent(const SDL_KeyboardEvent& e);
    void processMouseEvent(const SDL_MouseButtonEvent& e);
    void processMouseMotion(const SDL_MouseMotionEvent& e);
    void processMouseWheel(const SDL_MouseWheelEvent& e);
};
```

### 2. 이벤트 디스패처

```cpp
// EventDispatcher.h
class EventDispatcher {
public:
    using EventCallback = std::function<void(const SDL_Event&)>;
    
    void subscribe(SDL_EventType type, EventCallback callback);
    void dispatch(const SDL_Event& event);
    
private:
    std::unordered_map<SDL_EventType, std::vector<EventCallback>> listeners;
};
```

### 3. 프레임 레이트 제어

```cpp
// FrameRateController.h
class FrameRateController {
private:
    uint32_t targetFPS;
    uint32_t frameDelay;
    uint32_t lastFrameTime;
    float deltaTime;
    
    // 통계
    uint32_t frameCount;
    float fpsTimer;
    float currentFPS;
    
public:
    void setTargetFPS(uint32_t fps);
    void beginFrame();
    void endFrame();
    
    float getDeltaTime() const;
    float getCurrentFPS() const;
};
```

## 테스트 요구사항

### 1. 단위 테스트
- 이벤트 처리 정확성
- 타이밍 시스템 정밀도
- 상태 전환 무결성

### 2. 통합 테스트
- 다양한 입력 조합
- 장시간 실행 안정성
- 메모리 누수 검사

### 3. 성능 테스트
- FPS 일관성 측정
- 입력 지연 시간 측정
- CPU/메모리 사용률 모니터링

## 에러 처리

### 1. 이벤트 큐 오버플로우
- 경고 로그 출력
- 오래된 이벤트 드롭

### 2. 프레임 드롭
- 적응형 품질 조절
- 경고 표시

### 3. 입력 충돌
- 우선순위 기반 처리
- 상태별 입력 필터링

## 향후 확장 고려사항

### 1. 멀티스레딩
- 렌더링과 로직 분리
- 이벤트 처리 전용 스레드

### 2. 입력 녹화/재생
- 디버깅 지원
- 자동 테스트

### 3. 커스텀 이벤트
- 게임 내부 이벤트 시스템
- 플러그인 아키텍처

## 구현 우선순위

1. **필수 (Step 1.3)**
   - 기본 이벤트 루프
   - SDL_QUIT 처리
   - 키보드/마우스 기본 입력
   - 고정 프레임 레이트

2. **중요 (향후)**
   - 전체 입력 시스템
   - 상태 관리
   - FPS 카운터

3. **선택 (향후)**
   - 이벤트 디스패처
   - 입력 녹화/재생
   - 성능 모니터링

## 완료 기준

- [ ] SDL 이벤트 폴링 구현
- [ ] 기본 키보드/마우스 입력 처리
- [ ] 60 FPS 타겟 프레임 레이트 유지
- [ ] 프로그램 종료 처리 (SDL_QUIT)
- [ ] 델타 타임 계산
- [ ] 기본 에러 처리
- [ ] 코드 주석 및 문서화