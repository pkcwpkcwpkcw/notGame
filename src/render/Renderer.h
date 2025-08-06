#pragma once

#include <memory>
#include <string>

class Window;

class Renderer {
public:
    Renderer();
    ~Renderer();
    
    bool Initialize(Window* window);
    void Shutdown();
    
    void BeginFrame();
    void EndFrame();
    void Clear(float r, float g, float b, float a = 1.0f);
    
    void SetViewport(int x, int y, int width, int height);
    
    void PrintGLInfo();
    bool CheckGLErrors(const std::string& location);
    
private:
    bool InitializeOpenGL();
    void SetupDefaultGLState();
    
private:
    Window* m_window;
    bool m_initialized;
    
    int m_viewportX;
    int m_viewportY;
    int m_viewportWidth;
    int m_viewportHeight;
};