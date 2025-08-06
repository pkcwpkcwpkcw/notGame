# SDL2 + OpenGL 기본 윈도우 기능 명세서

## 1. 시스템 개요

### 1.1 목적
NOT Gate 게임의 렌더링 기반을 제공하는 SDL2/OpenGL 윈도우 시스템을 구현한다.

### 1.2 범위
- SDL2 기반 윈도우 관리
- OpenGL 3.3 Core Profile 렌더링 컨텍스트
- 기본 이벤트 처리 시스템
- 프레임 타이밍 제어

## 2. 기능 상세 명세

### 2.1 Application 클래스

#### 2.1.1 초기화 기능
```cpp
class Application {
public:
    bool Initialize();
    void Run();
    void Shutdown();
    
private:
    bool m_isRunning;
    uint32_t m_frameTime;
    std::unique_ptr<Window> m_window;
    std::unique_ptr<Renderer> m_renderer;
};
```

**Initialize()**
- SDL2 서브시스템 초기화 (VIDEO, EVENTS)
- Window 객체 생성 및 초기화
- Renderer 객체 생성 및 초기화
- 초기화 실패시 에러 로그 및 false 반환

**Run()**
- 메인 게임 루프 실행
- 프레임 시작 시간 기록
- 이벤트 처리
- 렌더링 수행
- 프레임 제한 (16.67ms = 60 FPS)

**Shutdown()**
- 렌더러 정리
- 윈도우 파괴
- SDL 종료

### 2.2 Window 클래스

#### 2.2.1 윈도우 생성 및 관리
```cpp
class Window {
public:
    Window(const std::string& title, int width, int height);
    ~Window();
    
    bool Create();
    void SwapBuffers();
    void HandleResize(int width, int height);
    
    SDL_Window* GetSDLWindow() const { return m_window; }
    SDL_GLContext GetGLContext() const { return m_glContext; }
    
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    
private:
    SDL_Window* m_window;
    SDL_GLContext m_glContext;
    std::string m_title;
    int m_width, m_height;
    bool m_fullscreen;
};
```

**Create()**
- SDL 윈도우 생성 (SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE)
- OpenGL 컨텍스트 생성
- OpenGL 확장 로더 초기화 (GLEW/GLAD)
- VSync 설정 (SDL_GL_SetSwapInterval)

**SwapBuffers()**
- 더블 버퍼 스왑
- 화면 갱신

**HandleResize()**
- 윈도우 크기 변경 처리
- OpenGL 뷰포트 업데이트
- 종횡비 유지

### 2.3 Renderer 클래스

#### 2.3.1 OpenGL 렌더링 관리
```cpp
class Renderer {
public:
    bool Initialize(Window* window);
    void BeginFrame();
    void EndFrame();
    void Clear(float r, float g, float b, float a);
    void SetViewport(int x, int y, int width, int height);
    
    void PrintGLInfo();
    bool CheckGLErrors(const std::string& location);
    
private:
    Window* m_window;
    glm::mat4 m_projection;
    glm::mat4 m_view;
};
```

**Initialize()**
- OpenGL 상태 초기화
- 깊이 테스트 활성화
- 블렌딩 설정
- 기본 셰이더 컴파일 (향후 확장용)

**BeginFrame()**
- 프레임 시작 준비
- 렌더 상태 리셋

**EndFrame()**
- 렌더링 완료 처리
- 버퍼 스왑 요청

**Clear()**
- 컬러 버퍼 클리어
- 깊이 버퍼 클리어

### 2.4 EventHandler 클래스

#### 2.4.1 이벤트 처리
```cpp
class EventHandler {
public:
    struct Event {
        enum Type {
            QUIT,
            WINDOW_RESIZE,
            KEY_DOWN,
            KEY_UP,
            MOUSE_MOVE,
            MOUSE_BUTTON_DOWN,
            MOUSE_BUTTON_UP,
            MOUSE_WHEEL
        };
        
        Type type;
        union {
            struct { int width, height; } resize;
            struct { SDL_Keycode key; } keyboard;
            struct { int x, y, button; } mouse;
            struct { int delta; } wheel;
        };
    };
    
    static bool PollEvents(std::vector<Event>& events);
    static bool IsKeyPressed(SDL_Keycode key);
    static void GetMousePosition(int& x, int& y);
};
```

**PollEvents()**
- SDL 이벤트 큐 처리
- 이벤트를 내부 형식으로 변환
- 이벤트 벡터에 추가

### 2.5 Timer 클래스

#### 2.5.1 프레임 타이밍
```cpp
class Timer {
public:
    void Start();
    void Update();
    
    float GetDeltaTime() const { return m_deltaTime; }
    float GetFPS() const { return m_fps; }
    uint32_t GetFrameCount() const { return m_frameCount; }
    
    void LimitFPS(int targetFPS);
    
private:
    uint64_t m_startTime;
    uint64_t m_lastTime;
    float m_deltaTime;
    float m_fps;
    uint32_t m_frameCount;
    
    // FPS 계산용
    uint64_t m_fpsUpdateTime;
    uint32_t m_fpsFrameCount;
};
```

**LimitFPS()**
- 목표 FPS에 맞춰 프레임 제한
- SDL_Delay() 사용하여 CPU 사용률 감소

## 3. 에러 처리

### 3.1 초기화 에러
```cpp
enum class InitError {
    NONE = 0,
    SDL_INIT_FAILED,
    WINDOW_CREATE_FAILED,
    GL_CONTEXT_FAILED,
    GL_VERSION_TOO_LOW,
    GLEW_INIT_FAILED
};

class ErrorHandler {
public:
    static void LogError(InitError error, const std::string& details);
    static void ShowErrorDialog(const std::string& message);
    static std::string GetSDLError();
    static std::string GetGLError();
};
```

### 3.2 런타임 에러
- OpenGL 에러 체크 (glGetError)
- SDL 에러 체크 (SDL_GetError)
- 로그 파일 출력

## 4. 설정 시스템

### 4.1 Configuration 클래스
```cpp
struct WindowConfig {
    std::string title = "NOT Gate Sandbox v0.1.0";
    int width = 1280;
    int height = 720;
    bool fullscreen = false;
    bool vsync = true;
    int targetFPS = 60;
    int msaaSamples = 0;
};

class Configuration {
public:
    static bool Load(const std::string& filename);
    static void Save(const std::string& filename);
    
    static WindowConfig& GetWindowConfig();
    
private:
    static WindowConfig s_windowConfig;
};
```

## 5. 로깅 시스템

### 5.1 Logger 클래스
```cpp
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

class Logger {
public:
    static void Initialize(const std::string& logFile);
    static void Shutdown();
    
    static void Log(LogLevel level, const std::string& message);
    static void Debug(const std::string& msg);
    static void Info(const std::string& msg);
    static void Warning(const std::string& msg);
    static void Error(const std::string& msg);
    
private:
    static std::ofstream s_logFile;
    static LogLevel s_minLevel;
};
```

## 6. 메인 프로그램 흐름

### 6.1 main.cpp
```cpp
int main(int argc, char* argv[]) {
    // 로거 초기화
    Logger::Initialize("notgate.log");
    Logger::Info("=== NOT Gate Sandbox Starting ===");
    
    // 설정 로드
    Configuration::Load("config.ini");
    
    // 애플리케이션 생성
    Application app;
    
    // 초기화
    if (!app.Initialize()) {
        Logger::Error("Failed to initialize application");
        ErrorHandler::ShowErrorDialog("Initialization failed. Check notgate.log");
        return -1;
    }
    
    // 메인 루프 실행
    app.Run();
    
    // 정리
    app.Shutdown();
    
    // 설정 저장
    Configuration::Save("config.ini");
    
    Logger::Info("=== NOT Gate Sandbox Shutdown ===");
    Logger::Shutdown();
    
    return 0;
}
```

### 6.2 메인 루프
```cpp
void Application::Run() {
    Timer timer;
    timer.Start();
    
    std::vector<EventHandler::Event> events;
    
    while (m_isRunning) {
        timer.Update();
        
        // 이벤트 처리
        events.clear();
        if (EventHandler::PollEvents(events)) {
            for (const auto& event : events) {
                HandleEvent(event);
            }
        }
        
        // 렌더링
        m_renderer->BeginFrame();
        m_renderer->Clear(0.1f, 0.1f, 0.15f, 1.0f);  // 어두운 파란색 배경
        
        // TODO: 실제 렌더링 코드
        
        m_renderer->EndFrame();
        m_window->SwapBuffers();
        
        // FPS 제한
        timer.LimitFPS(60);
        
        // FPS 표시 (디버그)
        if (timer.GetFrameCount() % 60 == 0) {
            Logger::Debug("FPS: " + std::to_string(timer.GetFPS()));
        }
    }
}
```

## 7. 플랫폼별 고려사항

### 7.1 Windows
- WinMain 대신 main 사용 (SDL_main)
- High DPI 지원
- 다중 모니터 처리

### 7.2 Linux
- X11/Wayland 호환성
- 패키지 의존성 체크

### 7.3 macOS
- Retina 디스플레이 지원
- 메뉴바 통합

## 8. 성능 모니터링

### 8.1 PerformanceMonitor 클래스
```cpp
class PerformanceMonitor {
public:
    void BeginFrame();
    void EndFrame();
    
    float GetAverageFPS() const;
    float GetFrameTime() const;
    float GetCPUUsage() const;
    size_t GetMemoryUsage() const;
    
    void PrintStats();
    
private:
    struct FrameData {
        uint64_t startTime;
        uint64_t endTime;
        float cpuUsage;
        size_t memoryUsage;
    };
    
    std::deque<FrameData> m_frameHistory;
    static const size_t MAX_FRAME_HISTORY = 120;
};
```

## 9. 테스트 모드

### 9.1 테스트 기능
```cpp
class TestMode {
public:
    static void RunBasicTests();
    static void StressTest(int duration);
    static void BenchmarkRendering();
    
private:
    static bool TestWindowCreation();
    static bool TestGLContext();
    static bool TestEventHandling();
    static bool TestFrameLimiting();
};
```

## 10. 확장 가능성

### 10.1 향후 추가 기능
- ImGui 통합 준비
- 셰이더 시스템 기반
- 텍스처 관리자 인터페이스
- 오디오 시스템 연동점

### 10.2 모듈화
- 각 클래스는 독립적으로 테스트 가능
- 인터페이스를 통한 느슨한 결합
- 의존성 주입 패턴 사용