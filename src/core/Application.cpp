#include "Application.h"
#include "../render/Window.h"
#include "../render/Renderer.h"
#include "../utils/Logger.h"

#include <iostream>

#ifdef HAS_SDL2
#include <SDL.h>
#endif

Application* Application::s_instance = nullptr;

Application::Application()
    : m_isRunning(false)
    , m_isInitialized(false)
    , m_lastFrameTime(0)
    , m_currentFrameTime(0)
    , m_deltaTime(0.0f)
{
    s_instance = this;
}

Application::~Application() {
    if (m_isInitialized) {
        Shutdown();
    }
    s_instance = nullptr;
}

bool Application::Initialize() {
    Logger::Info("Initializing NOT Gate Sandbox v0.1.0");
    
#ifdef HAS_SDL2
    // Initialize SDL2 with error checking
    int sdlResult = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    if (sdlResult < 0) {
        std::string error = SDL_GetError();
        Logger::Error("Failed to initialize SDL2: " + error);
        std::cerr << "SDL_Init failed: " << error << std::endl;
        return false;
    }
    Logger::Info("SDL2 initialized successfully");
#else
    Logger::Warning("SDL2 not available, running in console mode");
#endif

    m_window = std::make_unique<Window>();
    if (!m_window->Create("NOT Gate Sandbox v0.1.0", 1280, 720)) {
        Logger::Error("Failed to create window");
        return false;
    }
    Logger::Info("Window created successfully");

    m_renderer = std::make_unique<Renderer>();
    if (!m_renderer->Initialize(m_window.get())) {
        Logger::Error("Failed to initialize renderer");
        return false;
    }
    Logger::Info("Renderer initialized successfully");

    m_isInitialized = true;
    m_isRunning = true;
    
    Logger::Info("Application initialized successfully");
    return true;
}

void Application::Run() {
    if (!m_isInitialized) {
        Logger::Error("Cannot run application - not initialized");
        return;
    }

    Logger::Info("Starting main loop");
    
#ifdef HAS_SDL2
    m_lastFrameTime = SDL_GetTicks();
    
    while (m_isRunning) {
        m_currentFrameTime = SDL_GetTicks();
        m_deltaTime = (m_currentFrameTime - m_lastFrameTime) / 1000.0f;
        m_lastFrameTime = m_currentFrameTime;
        
        HandleEvents();
        Update(m_deltaTime);
        Render();
        
        // Frame rate limiting (60 FPS)
        uint32_t frameTime = SDL_GetTicks() - m_currentFrameTime;
        if (frameTime < 16) {
            SDL_Delay(16 - frameTime);
        }
    }
#else
    Logger::Warning("No windowing system available - exiting");
#endif
    
    Logger::Info("Main loop ended");
}

void Application::Shutdown() {
    if (!m_isInitialized) {
        return;
    }

    Logger::Info("Shutting down application");
    
    m_isRunning = false;
    
    if (m_renderer) {
        m_renderer->Shutdown();
        m_renderer.reset();
    }
    
    if (m_window) {
        m_window->Destroy();
        m_window.reset();
    }
    
#ifdef HAS_SDL2
    SDL_Quit();
    Logger::Info("SDL2 shutdown complete");
#endif
    
    m_isInitialized = false;
    Logger::Info("Application shutdown complete");
}

void Application::HandleEvents() {
#ifdef HAS_SDL2
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                m_isRunning = false;
                break;
                
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    int width = event.window.data1;
                    int height = event.window.data2;
                    m_window->HandleResize(width, height);
                    m_renderer->SetViewport(0, 0, width, height);
                }
                break;
                
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    m_isRunning = false;
                }
                break;
        }
    }
#endif
}

void Application::Update(float /*deltaTime*/) {
    // Future: Update game logic here
}

void Application::Render() {
    m_renderer->BeginFrame();
    m_renderer->Clear(0.1f, 0.1f, 0.15f, 1.0f);
    
    // Future: Render game objects here
    
    m_renderer->EndFrame();
    m_window->SwapBuffers();
}