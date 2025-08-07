#include "Application.h"
#include "EventSystem.h"
#include "Timer.h"
#include "Circuit.h"
#include "ui/ImGuiManager.h"
#include "../render/GridRenderer.h"
#include "../render/Camera.h"
#include "../render/InputHandler.h"
#include "../render/RenderManager.h"
#include "../render/Window.h"
#include "../render/RenderTypes.h"
#include "../render/Renderer.h"
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
    
    m_camera = std::make_unique<Camera>(config.windowWidth, config.windowHeight);
    m_gridRenderer = std::make_unique<GridRenderer>();
    
    if (!m_gridRenderer->Initialize(config.windowWidth, config.windowHeight)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize grid renderer");
        return false;
    }
    
    m_inputHandler = std::make_unique<InputHandler>(*m_camera, *m_gridRenderer);
    
    // 새로운 렌더링 시스템 초기화
    if (!initializeRenderers()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize render system");
        return false;
    }
    
    // 테스트용 회로 생성 (비활성화)
    // createDemoCircuit();
    
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    m_currentState = AppState::MENU;  // 메뉴 화면으로 시작
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
                if (m_camera) {
                    m_camera->SetScreenSize(width, height);
                }
                if (m_gridRenderer) {
                    m_gridRenderer->OnResize(width, height);
                }
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
        
        // Handle input based on event type
        bool shouldProcessEvent = false;
        
        // Mouse events - only process if ImGui doesn't want mouse
        if ((event.type == SDL_MOUSEMOTION || 
             event.type == SDL_MOUSEBUTTONDOWN || 
             event.type == SDL_MOUSEBUTTONUP || 
             event.type == SDL_MOUSEWHEEL) && !imguiCapturedMouse) {
            shouldProcessEvent = true;
        }
        
        // Keyboard events - only process if ImGui doesn't want keyboard
        if ((event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) && !imguiCapturedKeyboard) {
            shouldProcessEvent = true;
        }
        
        // Window events always process
        if (event.type == SDL_WINDOWEVENT) {
            shouldProcessEvent = true;
        }
        
        if (shouldProcessEvent) {
            if (m_inputHandler) {
                m_inputHandler->HandleEvent(event);
            }
            m_eventSystem->processEvent(event);
        }
    }
}

void Application::update(float deltaTime) {
    if (m_inputHandler) {
        m_inputHandler->Update(deltaTime);
    }
    
    m_imguiManager->BeginFrame();
    
    static bool editorInitialized = false;
    
    switch (m_currentState) {
        case AppState::MENU:
            editorInitialized = false;  // Reset when returning to menu
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
            // Set limited grid for stage mode (example: 20x20 grid)
            if (m_camera && !m_camera->IsGridUnlimited()) {
                // Grid is already limited
            } else if (m_camera) {
                m_camera->SetGridBounds(glm::ivec2(-10, -10), glm::ivec2(9, 9));
            }
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
            // Editor mode starts with unlimited grid but can be changed by user
            if (!editorInitialized && m_camera) {
                m_camera->SetUnlimitedGrid(true);
                editorInitialized = true;
            }
            
            // Show editor info
            ImGui::Begin("Editor Mode");
            ImGui::Text("Sandbox Mode - Unlimited Grid");
            ImGui::Text("Camera Position: (%.2f, %.2f)", 
                m_camera ? m_camera->GetPosition().x : 0.0f,
                m_camera ? m_camera->GetPosition().y : 0.0f);
            ImGui::Text("Zoom: %.2fx", m_camera ? m_camera->GetZoom() : 1.0f);
            ImGui::Separator();
            
            if (ImGui::Button("Test Limited Grid (10x10)")) {
                if (m_camera) {
                    m_camera->SetGridBounds(glm::ivec2(-5, -5), glm::ivec2(4, 4));
                    m_camera->Reset();
                    SDL_Log("Set 10x10 grid bounds: min(-5,-5) max(4,4), unlimited=%d", 
                            m_camera->IsGridUnlimited() ? 1 : 0);
                }
            }
            if (ImGui::Button("Test Limited Grid (50x50)")) {
                if (m_camera) {
                    m_camera->SetGridBounds(glm::ivec2(-25, -25), glm::ivec2(24, 24));
                    m_camera->Reset();
                }
            }
            if (ImGui::Button("Unlimited Grid")) {
                if (m_camera) {
                    m_camera->SetUnlimitedGrid(true);
                }
            }
            
            if (ImGui::Button("Back to Menu")) {
                setState(AppState::MENU);
            }
            ImGui::End();
            break;
            
        default:
            break;
    }
    
    m_eventSystem->update();
}

void Application::render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 새로운 렌더링 시스템 사용
    if (m_renderManager && m_circuit && (m_currentState == AppState::PLAYING || m_currentState == AppState::EDITOR)) {
        m_renderManager->BeginFrame();
        m_renderManager->RenderCircuit(*m_circuit);
        m_renderManager->EndFrame();
    } else if (m_gridRenderer && m_camera && (m_currentState == AppState::PLAYING || m_currentState == AppState::EDITOR)) {
        m_gridRenderer->Render(*m_camera);
    }
    
    m_imguiManager->EndFrame();
    
    SDL_GL_SwapWindow(m_window);
}

void Application::regulateFrameRate() {
    m_timer->waitForTargetFPS();
}

void Application::setState(AppState newState) {
    // Reset editor initialization flag when leaving editor mode
    if (m_currentState == AppState::EDITOR && newState != AppState::EDITOR) {
        // This will be reset in the EDITOR case statement
    }
    
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
    if (m_renderManager) {
        m_renderManager->Shutdown();
        m_renderManager.reset();
    }
    
    if (m_gridRenderer) {
        m_gridRenderer->Shutdown();
        m_gridRenderer.reset();
    }
    
    if (m_glContext) {
        SDL_GL_DeleteContext(m_glContext);
        m_glContext = nullptr;
    }
}

bool Application::initializeRenderers() {
    // Window 래퍼 생성
    m_renderWindow = std::make_unique<Window>();
    m_renderWindow->SetSDLWindow(m_window);
    
    // RenderManager 초기화
    m_renderManager = std::make_unique<RenderManager>();
    if (!m_renderManager->Initialize(m_renderWindow.get())) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize RenderManager");
        return false;
    }
    
    // RenderManager가 Application의 카메라를 사용하도록 설정
    if (m_camera) {
        m_renderManager->SetCamera(m_camera.get());
    }
    
    // Circuit 초기화
    m_circuit = std::make_unique<Circuit>();
    
    SDL_Log("Render system initialized successfully");
    return true;
}

void Application::createDemoCircuit() {
    if (!m_circuit) {
        return;
    }
    
    // 간단한 NOT 게이트 체인 생성
    // 5개의 NOT 게이트를 일렬로 연결
    std::vector<GateId> gateIds;
    
    for (int i = 0; i < 5; i++) {
        Vec2 position(i * 3.0f, 0.0f);  // 3칸 간격으로 배치
        auto result = m_circuit->addGate(position);
        if (result.isOk()) {
            gateIds.push_back(result.value);
        }
    }
    
    // 게이트들을 와이어로 연결
    for (size_t i = 0; i < gateIds.size() - 1; i++) {
        m_circuit->connectGates(gateIds[i], gateIds[i + 1], 1);  // 가운데 입력 포트로 연결
    }
    
    // 추가 테스트 회로: 피드백 루프
    Vec2 pos1(-3.0f, -3.0f);
    Vec2 pos2(0.0f, -3.0f);
    Vec2 pos3(3.0f, -3.0f);
    
    auto g1 = m_circuit->addGate(pos1);
    auto g2 = m_circuit->addGate(pos2);
    auto g3 = m_circuit->addGate(pos3);
    
    if (g1.isOk() && g2.isOk() && g3.isOk()) {
        // 피드백 루프 와이어
        m_circuit->connectGates(g1.value, g2.value, 0);  // 첫 번째 입력 포트
        m_circuit->connectGates(g2.value, g3.value, 1);  // 가운데 입력 포트  
        m_circuit->connectGates(g3.value, g1.value, 2);  // 세 번째 입력 포트
    }
    
    SDL_Log("Demo circuit created with %zu gates and %zu wires", 
            m_circuit->getGateCount(), m_circuit->getWireCount());
}

void Application::cleanupSDL() {
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    
    SDL_Quit();
}