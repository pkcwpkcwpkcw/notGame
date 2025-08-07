#pragma once

#include <string>

#ifdef HAS_SDL2
struct SDL_Window;
typedef void* SDL_GLContext;
#endif

class Window {
public:
    Window();
    ~Window();

    bool Initialize(const std::string& title, int width, int height, bool fullscreen = false);
    bool Create(const std::string& title, int width, int height);
    void Destroy();
    void SwapBuffers();
    void HandleResize(int width, int height);
    
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    void GetSize(int& width, int& height) const { width = m_width; height = m_height; }
    bool IsFullscreen() const { return m_fullscreen; }
    
    void SetFullscreen(bool fullscreen);
    void SetVSync(bool enabled);
    
#ifdef HAS_SDL2
    SDL_Window* GetSDLWindow() const { return m_window; }
    SDL_GLContext GetGLContext() const { return m_glContext; }
    void SetSDLWindow(SDL_Window* window);
#endif

private:
    bool CreateSDLWindow(const std::string& title, int width, int height);
    bool CreateGLContext();
    
private:
#ifdef HAS_SDL2
    SDL_Window* m_window;
    SDL_GLContext m_glContext;
#endif
    
    std::string m_title;
    int m_width;
    int m_height;
    bool m_fullscreen;
    bool m_vsync;
    bool m_initialized;
};