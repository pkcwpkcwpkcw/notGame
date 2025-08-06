# 기본 이벤트 루프 기술 명세서

## 1. 시스템 아키텍처

### 1.1 컴포넌트 다이어그램
```
┌─────────────────────────────────────────────────────────┐
│                      Application                         │
│  ┌──────────────────────────────────────────────────┐   │
│  │                  Event System                     │   │
│  │  ┌────────────┐  ┌────────────┐  ┌────────────┐ │   │
│  │  │   Event    │  │   Input    │  │   Event    │ │   │
│  │  │   Queue    │→ │   Handler  │→ │ Dispatcher │ │   │
│  │  └────────────┘  └────────────┘  └────────────┘ │   │
│  └──────────────────────────────────────────────────┘   │
│                           ↓                              │
│  ┌──────────────────────────────────────────────────┐   │
│  │                  Game Loop                        │   │
│  │  ┌────────────┐  ┌────────────┐  ┌────────────┐ │   │
│  │  │   Timer    │→ │   Update   │→ │   Render   │ │   │
│  │  └────────────┘  └────────────┘  └────────────┘ │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
                            ↓
         ┌──────────────────────────────────┐
         │      SDL2 / OpenGL Backend       │
         └──────────────────────────────────┘
```

### 1.2 클래스 계층 구조
```
Application
├── EventSystem
│   ├── EventQueue
│   ├── InputHandler
│   │   ├── KeyboardHandler
│   │   ├── MouseHandler
│   │   └── WindowHandler
│   └── EventDispatcher
├── GameLoop
│   ├── Timer
│   │   ├── FrameRateController
│   │   └── DeltaTimeCalculator
│   ├── StateManager
│   └── Renderer
└── ResourceManager
    ├── Window (SDL_Window*)
    └── GLContext (SDL_GLContext)
```

## 2. 상세 설계

### 2.1 Application 클래스

#### 2.1.1 헤더 파일 (Application.h)
```cpp
#pragma once

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <memory>
#include <string>

class EventSystem;
class StateManager;
class Renderer;
class Timer;

enum class AppState {
    INITIALIZING,
    MENU,
    PLAYING,
    PAUSED,
    EDITOR,
    SHUTTING_DOWN
};

struct AppConfig {
    int windowWidth = 1280;
    int windowHeight = 720;
    std::string windowTitle = "NOT Gate Game";
    bool fullscreen = false;
    bool vsync = true;
    int targetFPS = 60;
    int glMajorVersion = 3;
    int glMinorVersion = 3;
};

class Application {
public:
    Application();
    ~Application();
    
    // 생명주기 메서드
    bool initialize(const AppConfig& config);
    void run();
    void shutdown();
    
    // 상태 관리
    void setState(AppState newState);
    AppState getState() const { return m_currentState; }
    
    // 유틸리티
    bool isRunning() const { return m_running; }
    void quit() { m_running = false; }
    
    // 접근자
    SDL_Window* getWindow() const { return m_window; }
    SDL_GLContext getGLContext() const { return m_glContext; }
    
private:
    // 초기화 헬퍼
    bool initializeSDL();
    bool createWindow(const AppConfig& config);
    bool createGLContext(const AppConfig& config);
    bool initializeGLEW();
    bool initializeImGui();
    
    // 메인 루프 헬퍼
    void handleEvents();
    void update(float deltaTime);
    void render();
    void regulateFrameRate();
    
    // 정리 헬퍼
    void cleanupImGui();
    void cleanupGL();
    void cleanupSDL();
    
private:
    // SDL 리소스
    SDL_Window* m_window;
    SDL_GLContext m_glContext;
    
    // 설정
    AppConfig m_config;
    
    // 상태
    bool m_running;
    AppState m_currentState;
    
    // 서브시스템
    std::unique_ptr<EventSystem> m_eventSystem;
    std::unique_ptr<StateManager> m_stateManager;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<Timer> m_timer;
    
    // 성능 모니터링
    uint32_t m_frameCount;
    float m_fpsUpdateTimer;
    float m_currentFPS;
};
```

#### 2.1.2 구현 파일 (Application.cpp)
```cpp
#include "Application.h"
#include "EventSystem.h"
#include "StateManager.h"
#include "Renderer.h"
#include "Timer.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <iostream>

Application::Application()
    : m_window(nullptr)
    , m_glContext(nullptr)
    , m_running(false)
    , m_currentState(AppState::INITIALIZING)
    , m_frameCount(0)
    , m_fpsUpdateTimer(0.0f)
    , m_currentFPS(0.0f) {
}

Application::~Application() {
    shutdown();
}

bool Application::initialize(const AppConfig& config) {
    m_config = config;
    
    // SDL 초기화
    if (!initializeSDL()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, 
                    "SDL 초기화 실패");
        return false;
    }
    
    // 윈도우 생성
    if (!createWindow(config)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, 
                    "윈도우 생성 실패");
        return false;
    }
    
    // OpenGL 컨텍스트 생성
    if (!createGLContext(config)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, 
                    "OpenGL 컨텍스트 생성 실패");
        return false;
    }
    
    // GLEW 초기화
    if (!initializeGLEW()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, 
                    "GLEW 초기화 실패");
        return false;
    }
    
    // ImGui 초기화
    if (!initializeImGui()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, 
                    "ImGui 초기화 실패");
        return false;
    }
    
    // 서브시스템 생성
    m_eventSystem = std::make_unique<EventSystem>();
    m_stateManager = std::make_unique<StateManager>();
    m_renderer = std::make_unique<Renderer>();
    m_timer = std::make_unique<Timer>(config.targetFPS);
    
    // OpenGL 기본 상태 설정
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    m_currentState = AppState::MENU;
    m_running = true;
    
    SDL_Log("Application 초기화 완료");
    return true;
}

void Application::run() {
    SDL_Log("메인 루프 시작");
    
    while (m_running) {
        m_timer->beginFrame();
        
        handleEvents();
        update(m_timer->getDeltaTime());
        render();
        
        m_timer->endFrame();
        regulateFrameRate();
        
        // FPS 업데이트
        m_frameCount++;
        m_fpsUpdateTimer += m_timer->getDeltaTime();
        if (m_fpsUpdateTimer >= 1.0f) {
            m_currentFPS = m_frameCount / m_fpsUpdateTimer;
            m_frameCount = 0;
            m_fpsUpdateTimer = 0.0f;
        }
    }
    
    SDL_Log("메인 루프 종료");
}

void Application::handleEvents() {
    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
        // ImGui 이벤트 처리
        ImGui_ImplSDL2_ProcessEvent(&event);
        
        // ImGui가 입력을 캡처하면 게임에 전달하지 않음
        ImGuiIO& io = ImGui::GetIO();
        bool imguiCapturedMouse = io.WantCaptureMouse;
        bool imguiCapturedKeyboard = io.WantCaptureKeyboard;
        
        // 시스템 이벤트는 항상 처리
        if (event.type == SDL_QUIT) {
            m_running = false;
            return;
        }
        
        // 이벤트 시스템에 전달
        if (!imguiCapturedMouse || !imguiCapturedKeyboard) {
            m_eventSystem->processEvent(event);
        }
    }
}

void Application::update(float deltaTime) {
    // ImGui 프레임 시작
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    
    // 상태별 업데이트
    switch (m_currentState) {
        case AppState::MENU:
            // 메뉴 업데이트
            break;
            
        case AppState::PLAYING:
            // 게임 로직 업데이트
            m_stateManager->update(deltaTime);
            break;
            
        case AppState::PAUSED:
            // UI만 업데이트
            break;
            
        case AppState::EDITOR:
            // 에디터 업데이트
            break;
            
        default:
            break;
    }
}

void Application::render() {
    // 화면 클리어
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 게임 렌더링
    if (m_renderer) {
        m_renderer->render();
    }
    
    // ImGui 렌더링
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    // 버퍼 스왑
    SDL_GL_SwapWindow(m_window);
}

void Application::regulateFrameRate() {
    m_timer->waitForTargetFPS();
}
```

### 2.2 EventSystem 클래스

#### 2.2.1 헤더 파일 (EventSystem.h)
```cpp
#pragma once

#include <SDL2/SDL.h>
#include <functional>
#include <unordered_map>
#include <vector>
#include <queue>

// 입력 상태 구조체
struct KeyboardState {
    bool keys[SDL_NUM_SCANCODES];
    bool prevKeys[SDL_NUM_SCANCODES];
    
    void update();
    bool isPressed(SDL_Scancode key) const;
    bool isJustPressed(SDL_Scancode key) const;
    bool isJustReleased(SDL_Scancode key) const;
    
    // 수정자 키 상태
    bool isCtrlPressed() const;
    bool isShiftPressed() const;
    bool isAltPressed() const;
};

struct MouseState {
    int x, y;
    int prevX, prevY;
    int deltaX, deltaY;
    bool buttons[3];
    bool prevButtons[3];
    int wheelDelta;
    
    void update();
    bool isButtonPressed(int button) const;
    bool isButtonJustPressed(int button) const;
    bool isButtonJustReleased(int button) const;
    bool isDragging(int button) const;
};

// 이벤트 리스너 인터페이스
class IEventListener {
public:
    virtual ~IEventListener() = default;
    
    virtual void onKeyPress(SDL_Scancode key) {}
    virtual void onKeyRelease(SDL_Scancode key) {}
    virtual void onMousePress(int button, int x, int y) {}
    virtual void onMouseRelease(int button, int x, int y) {}
    virtual void onMouseMove(int x, int y, int dx, int dy) {}
    virtual void onMouseWheel(int delta) {}
    virtual void onWindowResize(int width, int height) {}
    virtual void onWindowEvent(SDL_WindowEventID event) {}
};

// 이벤트 시스템
class EventSystem {
public:
    EventSystem();
    ~EventSystem();
    
    // 이벤트 처리
    void processEvent(const SDL_Event& event);
    void update();
    
    // 리스너 관리
    void addEventListener(IEventListener* listener);
    void removeEventListener(IEventListener* listener);
    
    // 상태 접근
    const KeyboardState& getKeyboard() const { return m_keyboard; }
    const MouseState& getMouse() const { return m_mouse; }
    
    // 유틸리티
    void clearState();
    void setEventCapture(bool capture) { m_captureEvents = capture; }
    
private:
    void handleKeyboardEvent(const SDL_KeyboardEvent& e);
    void handleMouseButtonEvent(const SDL_MouseButtonEvent& e);
    void handleMouseMotionEvent(const SDL_MouseMotionEvent& e);
    void handleMouseWheelEvent(const SDL_MouseWheelEvent& e);
    void handleWindowEvent(const SDL_WindowEvent& e);
    
    void notifyKeyPress(SDL_Scancode key);
    void notifyKeyRelease(SDL_Scancode key);
    void notifyMousePress(int button, int x, int y);
    void notifyMouseRelease(int button, int x, int y);
    void notifyMouseMove(int x, int y, int dx, int dy);
    void notifyMouseWheel(int delta);
    void notifyWindowResize(int width, int height);
    void notifyWindowEvent(SDL_WindowEventID event);
    
private:
    KeyboardState m_keyboard;
    MouseState m_mouse;
    std::vector<IEventListener*> m_listeners;
    bool m_captureEvents;
    
    // 이벤트 큐 (옵션)
    std::queue<SDL_Event> m_eventQueue;
    static const size_t MAX_EVENT_QUEUE_SIZE = 100;
};
```

### 2.3 Timer 클래스

#### 2.3.1 헤더 파일 (Timer.h)
```cpp
#pragma once

#include <cstdint>
#include <chrono>

class Timer {
public:
    explicit Timer(uint32_t targetFPS = 60);
    ~Timer();
    
    // 프레임 타이밍
    void beginFrame();
    void endFrame();
    void waitForTargetFPS();
    
    // 시간 정보
    float getDeltaTime() const { return m_deltaTime; }
    float getTimeSinceStart() const;
    uint32_t getCurrentTicks() const;
    
    // FPS 정보
    float getCurrentFPS() const { return m_currentFPS; }
    uint32_t getTargetFPS() const { return m_targetFPS; }
    void setTargetFPS(uint32_t fps);
    
    // 성능 통계
    float getAverageFrameTime() const { return m_avgFrameTime; }
    float getMaxFrameTime() const { return m_maxFrameTime; }
    float getMinFrameTime() const { return m_minFrameTime; }
    void resetStats();
    
private:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using Duration = std::chrono::duration<float>;
    
    // 타이밍
    TimePoint m_startTime;
    TimePoint m_frameStartTime;
    TimePoint m_lastFrameTime;
    float m_deltaTime;
    
    // FPS 제어
    uint32_t m_targetFPS;
    float m_targetFrameTime;
    
    // FPS 측정
    uint32_t m_frameCount;
    float m_fpsAccumulator;
    float m_currentFPS;
    
    // 성능 통계
    float m_avgFrameTime;
    float m_maxFrameTime;
    float m_minFrameTime;
    float m_frameTimeAccumulator;
    uint32_t m_statFrameCount;
};
```

#### 2.3.2 구현 파일 (Timer.cpp)
```cpp
#include "Timer.h"
#include <SDL2/SDL.h>
#include <algorithm>
#include <thread>

Timer::Timer(uint32_t targetFPS)
    : m_targetFPS(targetFPS)
    , m_targetFrameTime(1.0f / targetFPS)
    , m_deltaTime(0.0f)
    , m_frameCount(0)
    , m_fpsAccumulator(0.0f)
    , m_currentFPS(0.0f)
    , m_avgFrameTime(0.0f)
    , m_maxFrameTime(0.0f)
    , m_minFrameTime(1000.0f)
    , m_frameTimeAccumulator(0.0f)
    , m_statFrameCount(0) {
    m_startTime = Clock::now();
    m_lastFrameTime = m_startTime;
}

Timer::~Timer() = default;

void Timer::beginFrame() {
    m_frameStartTime = Clock::now();
    
    // 델타 타임 계산
    Duration delta = m_frameStartTime - m_lastFrameTime;
    m_deltaTime = delta.count();
    
    // 델타 타임 제한 (큰 프레임 드롭 방지)
    m_deltaTime = std::min(m_deltaTime, 0.1f);  // 최대 100ms
    
    m_lastFrameTime = m_frameStartTime;
}

void Timer::endFrame() {
    TimePoint frameEndTime = Clock::now();
    Duration frameTime = frameEndTime - m_frameStartTime;
    float frameDuration = frameTime.count();
    
    // FPS 계산
    m_frameCount++;
    m_fpsAccumulator += m_deltaTime;
    
    if (m_fpsAccumulator >= 1.0f) {
        m_currentFPS = m_frameCount / m_fpsAccumulator;
        m_frameCount = 0;
        m_fpsAccumulator = 0.0f;
    }
    
    // 통계 업데이트
    m_frameTimeAccumulator += frameDuration;
    m_statFrameCount++;
    m_maxFrameTime = std::max(m_maxFrameTime, frameDuration);
    m_minFrameTime = std::min(m_minFrameTime, frameDuration);
    
    if (m_statFrameCount >= 100) {
        m_avgFrameTime = m_frameTimeAccumulator / m_statFrameCount;
        m_frameTimeAccumulator = 0.0f;
        m_statFrameCount = 0;
    }
}

void Timer::waitForTargetFPS() {
    TimePoint currentTime = Clock::now();
    Duration elapsedTime = currentTime - m_frameStartTime;
    float elapsed = elapsedTime.count();
    
    if (elapsed < m_targetFrameTime) {
        float sleepTime = m_targetFrameTime - elapsed;
        
        // 정밀한 대기를 위해 SDL_Delay와 busy wait 조합
        if (sleepTime > 0.002f) {  // 2ms 이상이면 SDL_Delay 사용
            SDL_Delay(static_cast<uint32_t>((sleepTime - 0.001f) * 1000));
        }
        
        // 남은 시간은 busy wait
        while (true) {
            currentTime = Clock::now();
            elapsedTime = currentTime - m_frameStartTime;
            if (elapsedTime.count() >= m_targetFrameTime) {
                break;
            }
        }
    }
}

void Timer::setTargetFPS(uint32_t fps) {
    m_targetFPS = fps;
    m_targetFrameTime = 1.0f / fps;
}

float Timer::getTimeSinceStart() const {
    TimePoint currentTime = Clock::now();
    Duration elapsed = currentTime - m_startTime;
    return elapsed.count();
}

uint32_t Timer::getCurrentTicks() const {
    return SDL_GetTicks();
}

void Timer::resetStats() {
    m_avgFrameTime = 0.0f;
    m_maxFrameTime = 0.0f;
    m_minFrameTime = 1000.0f;
    m_frameTimeAccumulator = 0.0f;
    m_statFrameCount = 0;
}
```

## 3. 빌드 설정

### 3.1 CMakeLists.txt 추가 내용
```cmake
# 소스 파일
set(EVENT_LOOP_SOURCES
    src/core/Application.cpp
    src/core/EventSystem.cpp
    src/core/Timer.cpp
    src/core/StateManager.cpp
    src/render/Renderer.cpp
)

set(EVENT_LOOP_HEADERS
    src/core/Application.h
    src/core/EventSystem.h
    src/core/Timer.h
    src/core/StateManager.h
    src/render/Renderer.h
)

# 실행 파일에 추가
add_executable(notgate3
    src/main.cpp
    ${EVENT_LOOP_SOURCES}
    ${EVENT_LOOP_HEADERS}
)

# 컴파일 옵션
if(MSVC)
    target_compile_options(notgate3 PRIVATE /W4 /WX)
else()
    target_compile_options(notgate3 PRIVATE -Wall -Wextra -Werror)
endif()
```

### 3.2 main.cpp
```cpp
#include "core/Application.h"
#include <iostream>

#ifdef _WIN32
    #include <windows.h>
    #ifdef main
        #undef main
    #endif
#endif

int main(int argc, char* argv[]) {
    // 로깅 초기화
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
    
    // 애플리케이션 설정
    AppConfig config;
    config.windowWidth = 1280;
    config.windowHeight = 720;
    config.windowTitle = "NOT Gate Game";
    config.fullscreen = false;
    config.vsync = true;
    config.targetFPS = 60;
    
    // 명령줄 인자 처리
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--fullscreen") {
            config.fullscreen = true;
        } else if (arg == "--no-vsync") {
            config.vsync = false;
        } else if (arg == "--fps" && i + 1 < argc) {
            config.targetFPS = std::stoi(argv[++i]);
        }
    }
    
    // 애플리케이션 실행
    {
        Application app;
        
        if (!app.initialize(config)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, 
                        "애플리케이션 초기화 실패");
            return -1;
        }
        
        app.run();
    }
    
    SDL_Log("프로그램 정상 종료");
    return 0;
}
```

## 4. 플랫폼별 고려사항

### 4.1 Windows
```cpp
// Windows 특정 설정
#ifdef _WIN32
    // 콘솔 창 숨기기 (Release 빌드)
    #ifndef _DEBUG
        #pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
    #endif
    
    // 고해상도 타이머 설정
    timeBeginPeriod(1);
    
    // DPI 인식 설정
    SetProcessDPIAware();
#endif
```

### 4.2 Linux
```cpp
// Linux 특정 설정
#ifdef __linux__
    // X11 스레드 초기화
    XInitThreads();
    
    // 시그널 핸들러 설정
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
#endif
```

### 4.3 macOS
```cpp
// macOS 특정 설정
#ifdef __APPLE__
    // Retina 디스플레이 지원
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    
    // 메인 스레드에서 실행 보장
    dispatch_async(dispatch_get_main_queue(), ^{
        // UI 코드
    });
#endif
```

## 5. 에러 처리 및 디버깅

### 5.1 에러 코드 정의
```cpp
enum class ErrorCode {
    SUCCESS = 0,
    SDL_INIT_FAILED = -1,
    WINDOW_CREATE_FAILED = -2,
    GL_CONTEXT_FAILED = -3,
    GL_VERSION_UNSUPPORTED = -4,
    GLEW_INIT_FAILED = -5,
    IMGUI_INIT_FAILED = -6,
    RESOURCE_LOAD_FAILED = -7,
    INVALID_CONFIG = -8
};
```

### 5.2 디버그 매크로
```cpp
#ifdef _DEBUG
    #define DEBUG_LOG(msg) SDL_Log("[DEBUG] %s", msg)
    #define DEBUG_BREAK() __debugbreak()
    #define ASSERT(cond) if(!(cond)) { DEBUG_BREAK(); }
#else
    #define DEBUG_LOG(msg)
    #define DEBUG_BREAK()
    #define ASSERT(cond)
#endif
```

### 5.3 OpenGL 디버그 콜백
```cpp
void GLAPIENTRY GLDebugCallback(GLenum source, GLenum type, GLuint id,
                                GLenum severity, GLsizei length,
                                const GLchar* message, const void* userParam) {
    if (severity == GL_DEBUG_SEVERITY_HIGH) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, 
                    "OpenGL Error: %s", message);
        DEBUG_BREAK();
    }
}

// 초기화 시 설정
glEnable(GL_DEBUG_OUTPUT);
glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
glDebugMessageCallback(GLDebugCallback, nullptr);
```

## 6. 성능 최적화

### 6.1 컴파일러 최적화
```cmake
# Release 빌드 최적화
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        target_compile_options(notgate3 PRIVATE /O2 /GL)
        set_target_properties(notgate3 PROPERTIES LINK_FLAGS "/LTCG")
    else()
        target_compile_options(notgate3 PRIVATE -O3 -march=native)
    endif()
endif()
```

### 6.2 메모리 정렬
```cpp
// 캐시 라인 정렬
alignas(64) struct CacheAlignedData {
    float position[16];
    float velocity[16];
};
```

### 6.3 분기 예측 힌트
```cpp
#ifdef __GNUC__
    #define LIKELY(x) __builtin_expect(!!(x), 1)
    #define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define LIKELY(x) (x)
    #define UNLIKELY(x) (x)
#endif

// 사용 예
if (LIKELY(m_running)) {
    update(deltaTime);
}
```

## 7. 테스트 코드

### 7.1 단위 테스트 예제
```cpp
#include <gtest/gtest.h>
#include "Timer.h"

TEST(TimerTest, DeltaTimeCalculation) {
    Timer timer(60);
    
    timer.beginFrame();
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
    timer.endFrame();
    
    float deltaTime = timer.getDeltaTime();
    EXPECT_NEAR(deltaTime, 0.016f, 0.002f);
}

TEST(TimerTest, FPSCalculation) {
    Timer timer(60);
    
    for (int i = 0; i < 120; i++) {
        timer.beginFrame();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        timer.endFrame();
    }
    
    float fps = timer.getCurrentFPS();
    EXPECT_NEAR(fps, 60.0f, 5.0f);
}
```

## 8. 프로파일링 지원

### 8.1 프로파일링 매크로
```cpp
class ProfileTimer {
public:
    ProfileTimer(const char* name) : m_name(name) {
        m_start = std::chrono::high_resolution_clock::now();
    }
    
    ~ProfileTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>
                       (end - m_start).count();
        SDL_Log("[PROFILE] %s: %lld us", m_name, duration);
    }
    
private:
    const char* m_name;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
};

#ifdef ENABLE_PROFILING
    #define PROFILE_SCOPE(name) ProfileTimer _timer##__LINE__(name)
#else
    #define PROFILE_SCOPE(name)
#endif
```

## 9. 메모리 관리

### 9.1 스마트 포인터 사용
```cpp
// RAII 패턴 적용
class SDLWindowDeleter {
public:
    void operator()(SDL_Window* window) {
        if (window) SDL_DestroyWindow(window);
    }
};

using SDLWindowPtr = std::unique_ptr<SDL_Window, SDLWindowDeleter>;
```

### 9.2 메모리 풀
```cpp
template<typename T, size_t PoolSize = 1024>
class MemoryPool {
public:
    T* allocate() {
        if (m_freeList.empty()) {
            return nullptr;
        }
        T* ptr = m_freeList.back();
        m_freeList.pop_back();
        return ptr;
    }
    
    void deallocate(T* ptr) {
        if (ptr) {
            m_freeList.push_back(ptr);
        }
    }
    
private:
    alignas(T) char m_pool[sizeof(T) * PoolSize];
    std::vector<T*> m_freeList;
};
```

## 10. 구현 체크리스트

### Phase 1: 기본 구조
- [ ] Application 클래스 구현
- [ ] SDL 초기화 코드
- [ ] 윈도우 생성
- [ ] OpenGL 컨텍스트 설정
- [ ] 기본 이벤트 루프

### Phase 2: 이벤트 시스템
- [ ] EventSystem 클래스 구현
- [ ] KeyboardState 구현
- [ ] MouseState 구현
- [ ] 이벤트 디스패처

### Phase 3: 타이밍 시스템
- [ ] Timer 클래스 구현
- [ ] 델타 타임 계산
- [ ] FPS 측정
- [ ] 프레임 레이트 제어

### Phase 4: 디버깅 & 최적화
- [ ] 에러 처리
- [ ] 디버그 로깅
- [ ] 프로파일링 지원
- [ ] 메모리 관리