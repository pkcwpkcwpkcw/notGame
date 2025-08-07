#include "DemoScene.h"
#include "render/RenderTypes.h"
#include <iostream>
#include <SDL.h>

DemoScene::DemoScene()
    : m_window(nullptr)
    , m_isDraggingWire(false)
    , m_dragStart(0.0f)
    , m_dragEnd(0.0f)
    , m_initialized(false) {
}

DemoScene::~DemoScene() {
    Shutdown();
}

bool DemoScene::Initialize(Window* window) {
    if (m_initialized) {
        return true;
    }
    
    m_window = window;
    
    m_circuit = std::make_unique<Circuit>();
    m_renderManager = std::make_unique<RenderManager>();
    
    if (!m_renderManager->Initialize(window)) {
        std::cerr << "Failed to initialize render manager!" << std::endl;
        return false;
    }
    
    CreateDemoCircuit();
    
    m_initialized = true;
    return true;
}

void DemoScene::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    m_renderManager.reset();
    m_circuit.reset();
    
    m_initialized = false;
}

void DemoScene::CreateDemoCircuit() {
    // 게이트 추가
    auto result1 = m_circuit->addGate(Vec2(0.0f, 0.0f));
    auto result2 = m_circuit->addGate(Vec2(5.0f, 0.0f));
    auto result3 = m_circuit->addGate(Vec2(10.0f, 0.0f));
    auto result4 = m_circuit->addGate(Vec2(0.0f, 5.0f));
    auto result5 = m_circuit->addGate(Vec2(5.0f, 5.0f));
    auto result6 = m_circuit->addGate(Vec2(10.0f, 5.0f));
    auto result7 = m_circuit->addGate(Vec2(2.5f, -5.0f));
    auto result8 = m_circuit->addGate(Vec2(7.5f, -5.0f));
    
    // 와이어 연결
    if (result1.isOk() && result2.isOk()) {
        m_circuit->connectGates(result1.value, result2.value, 0);
    }
    if (result2.isOk() && result3.isOk()) {
        m_circuit->connectGates(result2.value, result3.value, 0);
    }
    if (result4.isOk() && result5.isOk()) {
        m_circuit->connectGates(result4.value, result5.value, 1);
    }
    if (result5.isOk() && result6.isOk()) {
        m_circuit->connectGates(result5.value, result6.value, 2);
    }
    if (result1.isOk() && result7.isOk()) {
        m_circuit->connectGates(result1.value, result7.value, 0);
    }
    if (result3.isOk() && result8.isOk()) {
        m_circuit->connectGates(result3.value, result8.value, 1);
    }
    if (result7.isOk() && result2.isOk()) {
        m_circuit->connectGates(result7.value, result2.value, 1);
    }
    if (result8.isOk() && result2.isOk()) {
        m_circuit->connectGates(result8.value, result2.value, 2);
    }
    
    // 초기 신호 설정 - Gate 구조체가 다르므로 주석 처리
    // Circuit의 update 메서드가 신호를 처리함
}

void DemoScene::Update(float deltaTime) {
    if (!m_initialized || !m_circuit) {
        return;
    }
    
    // 회로 업데이트
    m_circuit->update(deltaTime);
}

void DemoScene::Render() {
    if (!m_initialized || !m_renderManager || !m_circuit) {
        return;
    }
    
    m_renderManager->BeginFrame();
    
    // RenderManager가 Circuit를 직접 렌더링
    m_renderManager->RenderCircuit(*m_circuit);
    
    if (m_isDraggingWire) {
        m_renderManager->RenderDraggingWire(m_dragStart, m_dragEnd);
    }
    
    m_renderManager->EndFrame();
}

void DemoScene::HandleInput(const SDL_Event& event) {
    if (!m_initialized || !m_renderManager) {
        return;
    }
    
    auto& camera = m_renderManager->GetCamera();
    
    switch (event.type) {
        case SDL_MOUSEWHEEL: {
            float zoomDelta = event.wheel.y * 0.1f;
            float newZoom = camera.GetZoom() * (1.0f + zoomDelta);
            newZoom = glm::clamp(newZoom, 0.1f, 10.0f);
            camera.SetZoom(newZoom);
            break;
        }
        
        case SDL_MOUSEBUTTONDOWN: {
            if (event.button.button == SDL_BUTTON_LEFT) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);
                
                glm::vec2 screenPos(static_cast<float>(mouseX), static_cast<float>(mouseY));
                glm::vec2 worldPos = camera.ScreenToWorld(screenPos);
                
                m_isDraggingWire = true;
                m_dragStart = worldPos;
                m_dragEnd = m_dragStart;
            } else if (event.button.button == SDL_BUTTON_MIDDLE) {
                SDL_SetRelativeMouseMode(SDL_TRUE);
            }
            break;
        }
        
        case SDL_MOUSEBUTTONUP: {
            if (event.button.button == SDL_BUTTON_LEFT) {
                if (m_isDraggingWire) {
                    m_isDraggingWire = false;
                    // 와이어 추가 로직은 추후 구현
                }
            } else if (event.button.button == SDL_BUTTON_MIDDLE) {
                SDL_SetRelativeMouseMode(SDL_FALSE);
            }
            break;
        }
        
        case SDL_MOUSEMOTION: {
            if (m_isDraggingWire) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);
                
                glm::vec2 screenPos(static_cast<float>(mouseX), static_cast<float>(mouseY));
                m_dragEnd = camera.ScreenToWorld(screenPos);
            } else if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
                glm::vec2 cameraDelta(
                    -event.motion.xrel * 0.01f / camera.GetZoom(),
                    event.motion.yrel * 0.01f / camera.GetZoom()
                );
                camera.SetPosition(camera.GetPosition() + cameraDelta);
            }
            break;
        }
        
        case SDL_KEYDOWN: {
            switch (event.key.keysym.sym) {
                case SDLK_r:
                    camera.SetPosition(glm::vec2(0.0f));
                    camera.SetZoom(1.0f);
                    break;
                case SDLK_g:
                    m_renderManager->SetShowGrid(!m_renderManager->GetGridRenderer().IsVisible());
                    break;
                case SDLK_SPACE:
                    CreateDemoCircuit();
                    break;
            }
            break;
        }
    }
}