#include "Application.h"
#include "EventSystem.h"
#include "Timer.h"
#include "Circuit.h"
#include "Grid.h"
#include "GridMap.h"
#include "WireManager.h"
#include "CellWireManager.h"
#include "ui/ImGuiManager.h"
#include "../ui/GatePaletteUI.h"
#include "../game/PlacementManager.h"
#include "../game/SelectionManager.h"
#include "../render/GridRenderer.h"
#include "../render/Camera.h"
#include "../render/InputHandler.h"
#include "../render/RenderManager.h"
#include "../render/Window.h"
#include "../render/RenderTypes.h"
#include "../render/Renderer.h"
#include "../input/InputManager.h"
#include "../input/WireInputHandler.h"
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
    
    // Circuit 초기화
    m_circuit = std::make_unique<Circuit>();
    
    // Gate Placement System 초기화
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Initializing Gate Placement System...");
    m_gridMap = std::make_unique<GridMap>();
    m_placementManager = std::make_unique<PlacementManager>();
    m_selectionManager = std::make_unique<SelectionManager>();
    m_gatePaletteUI = std::make_unique<GatePaletteUI>();
    m_wireManager = std::make_unique<WireManager>(m_circuit.get());
    m_cellWireManager = std::make_unique<CellWireManager>(m_circuit.get());
    
    // Grid 시스템 생성 및 연결
    Grid* gridSystem = new Grid();
    m_placementManager->initialize(m_circuit.get(), m_gridMap.get(), gridSystem, m_cellWireManager.get());
    m_selectionManager->initialize(m_circuit.get(), m_gridMap.get(), gridSystem);
    m_gatePaletteUI->initialize(m_placementManager.get(), m_selectionManager.get());
    
    // WireManager 초기화
    m_wireManager->initialize();
    
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Gate Placement System initialized successfully");
    
    // InputManager 초기화
    m_inputManager = std::make_unique<Input::InputManager>();
    m_inputManager->initialize(m_camera.get(), m_circuit.get());
    m_inputManager->setViewport(config.windowWidth, config.windowHeight);
    
    // CellWireManager를 InputManager 이벤트에 연결
    m_inputManager->subscribe<Input::DragEvent>([this](const Input::DragEvent& e) {
        if (m_cellWireManager) {
            Vec2 worldPos{e.currentWorld.x, e.currentWorld.y};
            glm::vec2 glmWorldPos(worldPos.x, worldPos.y);
            
            switch (e.phase) {
                case Input::DragPhase::Start:
                    m_cellWireManager->onDragStart(glm::vec2(e.startWorld.x, e.startWorld.y));
                    break;
                case Input::DragPhase::Move:
                    m_cellWireManager->onDragMove(glmWorldPos);
                    break;
                case Input::DragPhase::End:
                    m_cellWireManager->onDragEnd(glmWorldPos);
                    break;
                default:
                    break;
            }
        }
    });
    
    m_inputManager->subscribe<Input::ClickEvent>([this](const Input::ClickEvent& e) {
        if (m_wireManager) {
            m_wireManager->onClick(e);
        }
    });
    
    m_inputManager->subscribe<Input::HoverEvent>([this](const Input::HoverEvent& e) {
        if (m_wireManager) {
            Vec2 worldPos{e.worldPos.x, e.worldPos.y};
            m_wireManager->onMouseMove(worldPos);
        }
    });
    
    // InputManager 이벤트 콜백 설정
    m_inputManager->setOnClick([this](const Input::ClickEvent& e) {
        if (m_currentState == AppState::PLAYING) {
            if (e.hit.type == Input::ClickTarget::Gate) {
                SDL_Log("Gate clicked: %u", e.hit.objectId);
                m_inputManager->selectGate(e.hit.objectId);
            } else if (e.hit.type == Input::ClickTarget::Wire) {
                SDL_Log("Wire clicked: %u", e.hit.objectId);
                m_inputManager->selectWire(e.hit.objectId);
            } else if (e.hit.type == Input::ClickTarget::Empty) {
                SDL_Log("Empty space clicked at grid: %d, %d", e.gridPos.x, e.gridPos.y);
            }
        }
    });
    
    m_inputManager->setOnDragEnd([this](const Input::DragEvent& e) {
        if (m_currentState == AppState::PLAYING) {
            SDL_Log("Drag ended: from (%.2f, %.2f) to (%.2f, %.2f)", 
                e.startWorld.x, e.startWorld.y, e.currentWorld.x, e.currentWorld.y);
        }
    });
    
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
        
        // Debug: Log ImGui capture state when in placement mode
        static bool lastPlacementMode = false;
        bool currentPlacementMode = m_placementManager && m_placementManager->isInPlacementMode();
        if (currentPlacementMode && !lastPlacementMode) {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Entered placement mode - ImGui keyboard capture: %s", 
                       imguiCapturedKeyboard ? "YES" : "NO");
        }
        lastPlacementMode = currentPlacementMode;
        
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
        
        // Debug: Log all keyboard events in placement mode
        if (event.type == SDL_KEYDOWN && m_placementManager && m_placementManager->isInPlacementMode()) {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Key down in placement mode - key: %s, ImGui captured: %s", 
                       SDL_GetKeyName(event.key.keysym.sym), imguiCapturedKeyboard ? "YES" : "NO");
        }
        
        // Handle keyboard events - in placement mode, bypass ImGui capture for certain keys
        bool isInPlacementMode = m_placementManager && m_placementManager->isInPlacementMode();
        bool shouldHandleKey = !imguiCapturedKeyboard || 
                               (isInPlacementMode && (event.key.keysym.sym == SDLK_ESCAPE || 
                                                      event.key.keysym.sym == SDLK_LSHIFT || 
                                                      event.key.keysym.sym == SDLK_RSHIFT));
        
        if (shouldHandleKey && event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "ESC key pressed");
                    if (m_placementManager) {
                        bool inPlacementMode = m_placementManager->isInPlacementMode();
                        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "PlacementManager mode check: %s", inPlacementMode ? "IN PLACEMENT MODE" : "NOT IN PLACEMENT MODE");
                        if (inPlacementMode) {
                            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Calling exitPlacementMode()");
                            m_placementManager->exitPlacementMode();
                        } else if (m_selectionManager && m_selectionManager->hasSelection()) {
                            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Clearing selection");
                            m_selectionManager->clearSelection();
                        } else if (m_currentState == AppState::PLAYING) {
                            setState(AppState::PAUSED);
                        } else if (m_currentState == AppState::PAUSED) {
                            setState(AppState::PLAYING);
                        }
                    }
                    break;
                    
                case SDLK_n:
                    if (m_placementManager && m_currentState == AppState::PLAYING) {
                        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "N key pressed - entering placement mode");
                        m_placementManager->enterPlacementMode(GateType::NOT);
                    }
                    break;
                    
                case SDLK_LSHIFT:
                case SDLK_RSHIFT:
                    if (m_placementManager && m_currentState == AppState::PLAYING && !event.key.repeat) {
                        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Shift key pressed - enabling continuous placement");
                        m_placementManager->onKeyPress(Key::LeftShift);
                    }
                    break;
                    
                case SDLK_DELETE:
                case SDLK_BACKSPACE:
                    if (m_selectionManager && m_currentState == AppState::PLAYING) {
                        if (m_selectionManager->hasSelection()) {
                            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Deleting %zu selected gates", 
                                       m_selectionManager->getSelectionCount());
                            m_selectionManager->deleteSelected();
                        }
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
        
        // Keyboard events - only process if ImGui doesn't want keyboard OR if in placement mode for specific keys
        bool isInPlacementModeKeyRelease = m_placementManager && m_placementManager->isInPlacementMode() && 
                                          event.type == SDL_KEYUP && (event.key.keysym.sym == SDLK_LSHIFT || 
                                                                      event.key.keysym.sym == SDLK_RSHIFT);
        
        if ((event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) && 
            (!imguiCapturedKeyboard || isInPlacementModeKeyRelease)) {
            shouldProcessEvent = true;
            
            // Handle key release events for placement manager
            if (event.type == SDL_KEYUP && m_placementManager && m_currentState == AppState::PLAYING) {
                switch (event.key.keysym.sym) {
                    case SDLK_LSHIFT:
                    case SDLK_RSHIFT:
                        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Shift key released - disabling continuous placement");
                        m_placementManager->onKeyRelease(Key::LeftShift);
                        break;
                }
            }
        }
        
        // Window events always process
        if (event.type == SDL_WINDOWEVENT) {
            shouldProcessEvent = true;
            
            // Update InputManager viewport on resize
            if (event.window.event == SDL_WINDOWEVENT_RESIZED && m_inputManager) {
                m_inputManager->setViewport(event.window.data1, event.window.data2);
            }
        }
        
        if (shouldProcessEvent) {
            // Handle placement and selection mouse events
            if (m_currentState == AppState::PLAYING && !imguiCapturedMouse) {
                if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                    if (m_placementManager && m_placementManager->isInPlacementMode()) {
                        // Placement mode takes priority
                        if (m_camera) {
                            glm::vec2 screenPos(static_cast<float>(event.button.x), static_cast<float>(event.button.y));
                            glm::vec2 worldPos = m_camera->ScreenToWorld(screenPos);
                            Vec2 pos(worldPos.x, worldPos.y);
                            m_placementManager->onMouseClick(MouseButton::Left, pos);
                        }
                    } else if (m_selectionManager && m_camera) {
                        // Selection mode when not placing
                        glm::vec2 screenPos(static_cast<float>(event.button.x), static_cast<float>(event.button.y));
                        glm::vec2 worldPos = m_camera->ScreenToWorld(screenPos);
                        Vec2 pos(worldPos.x, worldPos.y);
                        
                        // Check for modifier keys
                        const Uint8* keystate = SDL_GetKeyboardState(NULL);
                        bool ctrlHeld = keystate[SDL_SCANCODE_LCTRL] || keystate[SDL_SCANCODE_RCTRL];
                        bool shiftHeld = keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT];
                        
                        m_selectionManager->onMouseClick(MouseButton::Left, pos, ctrlHeld, shiftHeld);
                    }
                } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT) {
                    if (m_placementManager && m_placementManager->isInPlacementMode()) {
                        // Right click cancels placement mode
                        m_placementManager->exitPlacementMode();
                    } else if (m_selectionManager && m_camera) {
                        // Right click for context menu
                        glm::vec2 screenPos(static_cast<float>(event.button.x), static_cast<float>(event.button.y));
                        glm::vec2 worldPos = m_camera->ScreenToWorld(screenPos);
                        Grid* gridSystem = new Grid();
                        Vec2 snapped = gridSystem->snapToGrid(Vec2(worldPos.x, worldPos.y));
                        Vec2i gridPos(static_cast<int>(snapped.x), static_cast<int>(snapped.y));
                        
                        // Store context menu position for rendering
                        m_contextMenuPos = glm::vec2(event.button.x, event.button.y);
                        m_contextMenuGridPos = gridPos;
                        m_showContextMenu = true;
                        
                        // Select gate if clicking on one
                        GateId gateId = m_selectionManager->getGateAt(gridPos);
                        if (gateId != Constants::INVALID_GATE_ID) {
                            if (!m_selectionManager->isSelected(gateId)) {
                                m_selectionManager->clearSelection();
                                m_selectionManager->selectGate(gateId);
                            }
                        }
                    }
                } else if (event.type == SDL_MOUSEMOTION) {
                    if (m_placementManager && m_camera) {
                        glm::vec2 screenPos(static_cast<float>(event.motion.x), static_cast<float>(event.motion.y));
                        glm::vec2 worldPos = m_camera->ScreenToWorld(screenPos);
                        Vec2 pos(worldPos.x, worldPos.y);
                        m_placementManager->onMouseMove(pos);
                    }
                }
            }
            
            // New InputManager handles events
            if (m_inputManager && !imguiCapturedMouse) {
                m_inputManager->handleEvent(event);
            }
            
            // Legacy input handler (will be phased out)
            if (m_inputHandler) {
                m_inputHandler->HandleEvent(event);
            }
            m_eventSystem->processEvent(event);
        }
    }
}

void Application::update(float deltaTime) {
    // Update new InputManager
    if (m_inputManager) {
        m_inputManager->update(deltaTime);
    }
    
    // Update legacy input handler
    if (m_inputHandler) {
        m_inputHandler->Update(deltaTime);
    }
    
    m_imguiManager->BeginFrame();
    
    static bool editorInitialized = false;
    
    // Debug: Always show state
    {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        ImGui::Begin("Debug State", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
        const char* stateStr = "Unknown";
        switch (m_currentState) {
            case AppState::MENU: stateStr = "MENU"; break;
            case AppState::PLAYING: stateStr = "PLAYING"; break;
            case AppState::PAUSED: stateStr = "PAUSED"; break;
            case AppState::EDITOR: stateStr = "EDITOR"; break;
        }
        ImGui::Text("Current State: %s", stateStr);
        ImGui::Text("Frame: %u", m_frameCount);
        ImGui::End();
    }
    
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
            {
                // Always show a simple test window first
                ImGui::Begin("TEST WINDOW - PLAYING MODE", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
                ImGui::Text("This is AppState::PLAYING");
                ImGui::Text("Frame: %d", m_frameCount);
                ImGui::End();
                
                // Set limited grid for stage mode (example: 20x20 grid)
                if (m_camera && !m_camera->IsGridUnlimited()) {
                    // Grid is already limited
                } else if (m_camera) {
                    m_camera->SetGridBounds(glm::ivec2(-10, -10), glm::ivec2(9, 9));
                }
                
                // Create test gates for input testing (once)
                static bool testGatesCreated = false;
                if (!testGatesCreated && m_circuit) {
                    SDL_Log("Creating test circuit...");
                    
                    // Add a few test gates
                    auto r1 = m_circuit->addGate(Vec2(0, 0));
                    auto r2 = m_circuit->addGate(Vec2(3, 0));
                    auto r3 = m_circuit->addGate(Vec2(6, 0));
                    
                    if (r1.isOk()) {
                        SDL_Log("Gate 1 created with ID: %u", r1.value);
                    } else {
                        SDL_Log("Failed to create Gate 1, error: %d", (int)r1.error);
                    }
                    
                    if (r2.isOk()) {
                        SDL_Log("Gate 2 created with ID: %u", r2.value);
                    } else {
                        SDL_Log("Failed to create Gate 2, error: %d", (int)r2.error);
                    }
                    
                    if (r3.isOk()) {
                        SDL_Log("Gate 3 created with ID: %u", r3.value);
                    } else {
                        SDL_Log("Failed to create Gate 3, error: %d", (int)r3.error);
                    }
                    
                    // Add test wires connecting gates if gates were created successfully
                    if (r1.isOk() && r2.isOk() && r3.isOk()) {
                        auto w1 = m_circuit->connectGates(r1.value, r2.value, 0);
                        auto w2 = m_circuit->connectGates(r2.value, r3.value, 0);
                        
                        if (w1.isOk()) {
                            SDL_Log("Wire 1 created with ID: %u", w1.value);
                        } else {
                            SDL_Log("Failed to create Wire 1, error: %d", (int)w1.error);
                        }
                        
                        if (w2.isOk()) {
                            SDL_Log("Wire 2 created with ID: %u", w2.value);
                        } else {
                            SDL_Log("Failed to create Wire 2, error: %d", (int)w2.error);
                        }
                    }
                    
                    testGatesCreated = true;
                    SDL_Log("Test circuit creation completed. Total gates: %zu, Total wires: %zu", 
                            m_circuit->getGateCount(), m_circuit->getWireCount());
                    
                    // Reset camera to see the gates
                    if (m_camera) {
                        m_camera->Reset();
                        SDL_Log("Camera reset to position (%.2f, %.2f), zoom: %.2f", 
                                m_camera->GetPosition().x, m_camera->GetPosition().y, m_camera->GetZoom());
                    }
                }
                
                // Render Gate Palette UI
                if (m_gatePaletteUI) {
                    m_gatePaletteUI->render();
                }
                
                // Game controls UI
                ImGui::Begin("Game Controls", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
                
                // Display circuit status
                ImGui::Text("Circuit Status:");
                ImGui::Text("  Gates: %zu", m_circuit ? m_circuit->getGateCount() : 0);
                ImGui::Text("  Wires: %zu", m_circuit ? m_circuit->getWireCount() : 0);
                ImGui::Text("  Camera: (%.1f, %.1f) Zoom: %.2f", 
                    m_camera ? m_camera->GetPosition().x : 0.0f,
                    m_camera ? m_camera->GetPosition().y : 0.0f,
                    m_camera ? m_camera->GetZoom() : 1.0f);
                ImGui::Separator();
                
                if (ImGui::Button("Pause (ESC)")) {
                    setState(AppState::PAUSED);
                }
                if (ImGui::Button("Back to Menu")) {
                    setState(AppState::MENU);
                }
                
                // Manual gate creation button for testing
                if (ImGui::Button("Add Test Gate")) {
                    if (m_circuit) {
                        static float xPos = -3.0f;
                        auto result = m_circuit->addGate(Vec2(xPos, 2.0f));
                        if (result.isOk()) {
                            SDL_Log("Manual gate added at (%.1f, 2.0) with ID: %u", xPos, result.value);
                            xPos += 2.0f;
                        }
                    }
                }
                
                if (ImGui::Button("Reset Camera")) {
                    if (m_camera) {
                        m_camera->Reset();
                    }
                }
                
                // Toggle debug overlay
                static bool showInputDebug = false;
                if (ImGui::Checkbox("Show Input Debug", &showInputDebug)) {
                    if (m_inputManager) {
                        m_inputManager->setDebugOverlay(showInputDebug);
                    }
                }
                ImGui::End();
                
                // Render InputManager debug overlay
                if (m_inputManager) {
                    m_inputManager->renderDebugOverlay();
                }
                
                // Render context menu
                if (m_showContextMenu) {
                    ImGui::SetNextWindowPos(ImVec2(m_contextMenuPos.x, m_contextMenuPos.y));
                    if (ImGui::BeginPopupContextItem("##ContextMenu", ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_MouseButtonRight)) {
                        bool hasGate = false;
                        if (m_selectionManager) {
                            GateId gateId = m_selectionManager->getGateAt(m_contextMenuGridPos);
                            hasGate = (gateId != Constants::INVALID_GATE_ID);
                        }
                        
                        if (hasGate) {
                            if (ImGui::MenuItem("Delete Gate", "Delete")) {
                                if (m_selectionManager && m_selectionManager->hasSelection()) {
                                    m_selectionManager->deleteSelected();
                                }
                            }
                            ImGui::Separator();
                        }
                        
                        if (ImGui::MenuItem("Place NOT Gate", "N")) {
                            if (m_placementManager) {
                                m_placementManager->enterPlacementMode(GateType::NOT);
                            }
                        }
                        
                        if (hasGate) {
                            ImGui::Separator();
                            if (ImGui::MenuItem("Select All", "Ctrl+A")) {
                                // TODO: Implement select all
                            }
                            if (ImGui::MenuItem("Clear Selection", "Esc")) {
                                if (m_selectionManager) {
                                    m_selectionManager->clearSelection();
                                }
                            }
                        }
                        
                        ImGui::EndPopup();
                    } else {
                        m_showContextMenu = false;
                    }
                    
                    // Open popup on first frame
                    if (m_showContextMenu) {
                        ImGui::OpenPopup("##ContextMenu");
                    }
                }
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
    // Clear only color buffer, not depth (ImGui doesn't use depth buffer)
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Render the grid and circuit first
    if (m_renderManager && m_circuit && (m_currentState == AppState::PLAYING || m_currentState == AppState::EDITOR)) {
        m_renderManager->BeginFrame();
        m_renderManager->RenderCircuit(*m_circuit);
        
        // CellWire 렌더링
        if (m_cellWireManager) {
            m_renderManager->RenderCellWires(m_cellWireManager->getAllWires());
        }
        
        m_renderManager->EndFrame();
        
        // Render gate preview AFTER everything else (on top)
        if (m_placementManager && m_placementManager->isInPlacementMode()) {
            // Disable depth testing to ensure preview is always on top
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
            
            Vec2i previewPos = m_placementManager->getPreviewPosition();
            bool isValid = m_placementManager->isPreviewPositionValid();
            glm::vec2 worldPos(static_cast<float>(previewPos.x), static_cast<float>(previewPos.y));
            m_renderManager->RenderGatePreview(worldPos, GateType::NOT, isValid);
            
            // Re-enable depth testing
            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
        }
        
        // Render wire preview when connecting
        if (m_wireManager && m_wireManager->isConnecting()) {
            const auto& previewPath = m_wireManager->getPreviewPath();
            if (previewPath.size() >= 2) {
                glm::vec2 start(previewPath[0].x, previewPath[0].y);
                glm::vec2 end(previewPath.back().x, previewPath.back().y);
                m_renderManager->RenderDraggingWire(start, end);
            }
        }
    } else if (m_gridRenderer && m_camera && (m_currentState == AppState::PLAYING || m_currentState == AppState::EDITOR)) {
        m_gridRenderer->Render(*m_camera);
    }
    
    // Removed repetitive rendering logs
    
    // Render ImGui on top of everything else
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
    
    // Note: Circuit is already initialized in initialize()
    
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