#pragma once

#include <memory>
#include <string>

class Window;
class Renderer;

class Application {
public:
    Application();
    ~Application();

    bool Initialize();
    void Run();
    void Shutdown();

    static Application* GetInstance() { return s_instance; }

private:
    void HandleEvents();
    void Update(float deltaTime);
    void Render();

    bool m_isRunning;
    bool m_isInitialized;
    
    std::unique_ptr<Window> m_window;
    std::unique_ptr<Renderer> m_renderer;
    
    uint32_t m_lastFrameTime;
    uint32_t m_currentFrameTime;
    float m_deltaTime;
    
    static Application* s_instance;
};