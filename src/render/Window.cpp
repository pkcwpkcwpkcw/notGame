#include "Window.h"
#include "../utils/Logger.h"

#ifdef HAS_OPENGL
#ifdef USE_GLAD
#include <glad/glad.h>
#endif
#endif

#ifdef HAS_SDL2
#include <SDL.h>
#endif

Window::Window()
    : m_width(0)
    , m_height(0)
    , m_fullscreen(false)
    , m_vsync(true)
    , m_initialized(false)
#ifdef HAS_SDL2
    , m_window(nullptr)
    , m_glContext(nullptr)
#endif
{
}

Window::~Window() {
    if (m_initialized) {
        Destroy();
    }
}

bool Window::Create(const std::string& title, int width, int height) {
    if (m_initialized) {
        Logger::Warning("Window already created");
        return true;
    }

    m_title = title;
    m_width = width;
    m_height = height;

#ifdef HAS_SDL2
    if (!CreateSDLWindow(title, width, height)) {
        return false;
    }

#ifdef HAS_OPENGL
    if (!CreateGLContext()) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
        return false;
    }
#endif

    m_initialized = true;
    Logger::Info("Window created: " + std::to_string(width) + "x" + std::to_string(height));
    return true;
#else
    Logger::Error("SDL2 not available - cannot create window");
    return false;
#endif
}

bool Window::CreateSDLWindow(const std::string& title, int width, int height) {
#ifdef HAS_SDL2
    uint32_t windowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
    
#ifdef HAS_OPENGL
    windowFlags |= SDL_WINDOW_OPENGL;
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
#endif

    m_window = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        windowFlags
    );

    if (!m_window) {
        Logger::Error("Failed to create SDL window: " + std::string(SDL_GetError()));
        return false;
    }

    return true;
#else
    return false;
#endif
}

bool Window::CreateGLContext() {
#if defined(HAS_SDL2) && defined(HAS_OPENGL)
    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext) {
        Logger::Error("Failed to create OpenGL context: " + std::string(SDL_GetError()));
        return false;
    }

    if (SDL_GL_MakeCurrent(m_window, m_glContext) < 0) {
        Logger::Error("Failed to make OpenGL context current: " + std::string(SDL_GetError()));
        SDL_GL_DeleteContext(m_glContext);
        m_glContext = nullptr;
        return false;
    }

#ifdef USE_GLAD
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        Logger::Error("Failed to initialize GLAD");
        SDL_GL_DeleteContext(m_glContext);
        m_glContext = nullptr;
        return false;
    }
#endif

    SetVSync(m_vsync);

    const char* glVersion = (const char*)glGetString(GL_VERSION);
    const char* glRenderer = (const char*)glGetString(GL_RENDERER);
    const char* glVendor = (const char*)glGetString(GL_VENDOR);
    
    Logger::Info("OpenGL Version: " + std::string(glVersion ? glVersion : "Unknown"));
    Logger::Info("OpenGL Renderer: " + std::string(glRenderer ? glRenderer : "Unknown"));
    Logger::Info("OpenGL Vendor: " + std::string(glVendor ? glVendor : "Unknown"));

    return true;
#else
    return true;
#endif
}

void Window::Destroy() {
    if (!m_initialized) {
        return;
    }

#ifdef HAS_SDL2
#ifdef HAS_OPENGL
    if (m_glContext) {
        SDL_GL_DeleteContext(m_glContext);
        m_glContext = nullptr;
    }
#endif

    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
#endif

    m_initialized = false;
    Logger::Info("Window destroyed");
}

void Window::SwapBuffers() {
#if defined(HAS_SDL2) && defined(HAS_OPENGL)
    if (m_window) {
        SDL_GL_SwapWindow(m_window);
    }
#endif
}

void Window::HandleResize(int width, int height) {
    m_width = width;
    m_height = height;
    Logger::Info("Window resized: " + std::to_string(width) + "x" + std::to_string(height));
}

void Window::SetFullscreen(bool fullscreen) {
#ifdef HAS_SDL2
    if (!m_window || m_fullscreen == fullscreen) {
        return;
    }

    uint32_t flags = fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
    if (SDL_SetWindowFullscreen(m_window, flags) == 0) {
        m_fullscreen = fullscreen;
        Logger::Info(fullscreen ? "Entered fullscreen mode" : "Exited fullscreen mode");
    } else {
        Logger::Error("Failed to change fullscreen mode: " + std::string(SDL_GetError()));
    }
#endif
}

void Window::SetVSync(bool enabled) {
#if defined(HAS_SDL2) && defined(HAS_OPENGL)
    m_vsync = enabled;
    int result = SDL_GL_SetSwapInterval(enabled ? 1 : 0);
    if (result < 0) {
        Logger::Warning("Failed to set VSync: " + std::string(SDL_GetError()));
    } else {
        Logger::Info(enabled ? "VSync enabled" : "VSync disabled");
    }
#endif
}