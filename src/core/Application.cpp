#include "Application.h"
#include "EventSystem.h"
#include "Timer.h"
#include "ui/ImGuiManager.h"
#include <imgui.h>
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
    
    if (!initializeSDL()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize SDL");
        return false;
    }
    
    if (!createWindow(config)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create window");
        return false;
    }
    
    if (!createGLContext(config)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create OpenGL context");
        return false;
    }
    
    if (!initializeGLEW()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize GLEW");
        return false;
    }
    
    if (!initializeImGui()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize ImGui");
        return false;
    }
    
    m_eventSystem = std::make_unique<EventSystem>();
    m_timer = std::make_unique<Timer>(config.targetFPS);
    
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    m_currentState = AppState::MENU;
    m_running = true;
    
    SDL_Log("Application initialized successfully");
    return true;
}

bool Application::initializeSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init failed: %s", SDL_GetError());
        return false;
    }
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    
    SDL_Log("SDL initialized");
    return true;
}

bool Application::createWindow(const AppConfig& config) {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, config.glMajorVersion);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, config.glMinorVersion);
    
    Uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    if (config.fullscreen) {
        windowFlags |= SDL_WINDOW_FULLSCREEN;
    }
    
    m_window = SDL_CreateWindow(
        config.windowTitle.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        config.windowWidth,
        config.windowHeight,
        windowFlags
    );
    
    if (!m_window) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }
    
    SDL_Log("Window created: %dx%d", config.windowWidth, config.windowHeight);
    return true;
}

bool Application::createGLContext(const AppConfig& config) {
    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_GL_CreateContext failed: %s", SDL_GetError());
        return false;
    }
    
    SDL_GL_MakeCurrent(m_window, m_glContext);
    
    if (config.vsync) {
        if (SDL_GL_SetSwapInterval(1) < 0) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Failed to enable VSync: %s", SDL_GetError());
        } else {
            SDL_Log("VSync enabled");
        }
    } else {
        SDL_GL_SetSwapInterval(0);
        SDL_Log("VSync disabled");
    }
    
    SDL_Log("OpenGL context created");
    return true;
}

bool Application::initializeGLEW() {
    // GLAD 사용
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize GLAD");
        return false;
    }
    
    SDL_Log("GLAD initialized");
    SDL_Log("OpenGL Version: %s", glGetString(GL_VERSION));
    SDL_Log("GLSL Version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
    SDL_Log("Renderer: %s", glGetString(GL_RENDERER));
    
    return true;
}

bool Application::initializeImGui() {
    m_imguiManager = std::make_unique<ImGuiManager>();
    
    if (!m_imguiManager->Initialize(m_window, m_glContext)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize ImGuiManager");
        return false;
    }
    
    SDL_Log("ImGui initialized via ImGuiManager");
    return true;
}

void Application::run() {
    SDL_Log("Main loop started");
    
    while (m_running) {
        m_timer->beginFrame();
        
        handleEvents();
        update(m_timer->getDeltaTime());
        render();
        
        m_timer->endFrame();
        regulateFrameRate();
        
        m_frameCount++;
        m_fpsUpdateTimer += m_timer->getDeltaTime();
        if (m_fpsUpdateTimer >= 1.0f) {
            m_currentFPS = m_frameCount / m_fpsUpdateTimer;
            m_frameCount = 0;
            m_fpsUpdateTimer = 0.0f;
            
            std::string title = m_config.windowTitle + " - FPS: " + std::to_string(static_cast<int>(m_currentFPS));
            SDL_SetWindowTitle(m_window, title.c_str());
        }
    }
    
    SDL_Log("Main loop ended");
}

void Application::handleEvents() {
    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
        m_imguiManager->ProcessEvent(event);
        
        bool imguiCapturedMouse = m_imguiManager->WantCaptureMouse();
        bool imguiCapturedKeyboard = m_imguiManager->WantCaptureKeyboard();
        
        if (event.type == SDL_QUIT) {
            m_running = false;
            return;
        }
        
        if (event.type == SDL_WINDOWEVENT) {
            if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                int width = event.window.data1;
                int height = event.window.data2;
                glViewport(0, 0, width, height);
                SDL_Log("Window resized: %dx%d", width, height);
            }
        }
        
        if (!imguiCapturedKeyboard && event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    if (m_currentState == AppState::PLAYING) {
                        setState(AppState::PAUSED);
                    } else if (m_currentState == AppState::PAUSED) {
                        setState(AppState::PLAYING);
                    }
                    break;
                    
                case SDLK_F11:
                    {
                        Uint32 flags = SDL_GetWindowFlags(m_window);
                        if (flags & SDL_WINDOW_FULLSCREEN) {
                            SDL_SetWindowFullscreen(m_window, 0);
                        } else {
                            SDL_SetWindowFullscreen(m_window, SDL_WINDOW_FULLSCREEN);
                        }
                    }
                    break;
            }
        }
        
        if (!imguiCapturedMouse || !imguiCapturedKeyboard) {
            m_eventSystem->processEvent(event);
        }
    }
}

void Application::update(float deltaTime) {
    m_imguiManager->BeginFrame();
    
    switch (m_currentState) {
        case AppState::MENU:
            ImGui::Begin("Main Menu");
            ImGui::Text("NOT Gate Game");
            if (ImGui::Button("Play")) {
                setState(AppState::PLAYING);
            }
            if (ImGui::Button("Editor")) {
                setState(AppState::EDITOR);
            }
            if (ImGui::Button("Show Demo Window")) {
                m_imguiManager->ShowDemoWindow();
            }
            if (ImGui::Button("Quit")) {
                m_running = false;
            }
            ImGui::End();
            break;
            
        case AppState::PLAYING:
            break;
            
        case AppState::PAUSED:
            ImGui::Begin("Paused");
            if (ImGui::Button("Resume")) {
                setState(AppState::PLAYING);
            }
            if (ImGui::Button("Main Menu")) {
                setState(AppState::MENU);
            }
            ImGui::End();
            break;
            
        case AppState::EDITOR:
            break;
            
        default:
            break;
    }
    
    m_eventSystem->update();
}

void Application::render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    m_imguiManager->EndFrame();
    
    SDL_GL_SwapWindow(m_window);
}

void Application::regulateFrameRate() {
    m_timer->waitForTargetFPS();
}

void Application::setState(AppState newState) {
    m_currentState = newState;
    SDL_Log("State changed to: %d", static_cast<int>(newState));
}

void Application::shutdown() {
    if (!m_running && m_currentState == AppState::SHUTTING_DOWN) {
        return;
    }
    
    m_running = false;
    m_currentState = AppState::SHUTTING_DOWN;
    
    cleanupImGui();
    cleanupGL();
    cleanupSDL();
    
    SDL_Log("Application shutdown complete");
}

void Application::cleanupImGui() {
    if (m_imguiManager) {
        m_imguiManager->Shutdown();
        m_imguiManager.reset();
    }
}

void Application::cleanupGL() {
    if (m_glContext) {
        SDL_GL_DeleteContext(m_glContext);
        m_glContext = nullptr;
    }
}

void Application::cleanupSDL() {
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    
    SDL_Quit();
}