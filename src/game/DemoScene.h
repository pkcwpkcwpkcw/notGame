#pragma once

#include <memory>
#include <SDL_events.h>
#include "core/Circuit.h"
#include "render/RenderManager.h"
#include "render/Window.h"

class DemoScene {
public:
    DemoScene();
    ~DemoScene();
    
    bool Initialize(Window* window);
    void Shutdown();
    
    void Update(float deltaTime);
    void Render();
    void HandleInput(const SDL_Event& event);
    
    void CreateDemoCircuit();
    
private:
    std::unique_ptr<Circuit> m_circuit;
    std::unique_ptr<RenderManager> m_renderManager;
    
    Window* m_window;
    
    bool m_isDraggingWire;
    glm::vec2 m_dragStart;
    glm::vec2 m_dragEnd;
    
    bool m_initialized;
};