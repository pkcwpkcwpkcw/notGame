#pragma once

#include <memory>
#include <vector>
#include "render/Renderer.h"
#include "render/GridRenderer.h"
#include "render/GateRenderer.h"
#include "render/WireRenderer.h"
#include "render/Camera.h"
#include "core/Circuit.h"

class Window;

class RenderManager {
public:
    RenderManager();
    ~RenderManager();
    
    bool Initialize(Window* window);
    void Shutdown();
    
    void BeginFrame();
    void EndFrame();
    
    void RenderCircuit(const Circuit& circuit);
    void RenderDraggingWire(const glm::vec2& start, const glm::vec2& end);
    
    void SetGridSize(float size);
    void SetGridColor(const glm::vec4& color);
    void SetShowGrid(bool show) { m_showGrid = show; }
    
    Camera& GetCamera() { return m_externalCamera ? *m_externalCamera : *m_camera; }
    const Camera& GetCamera() const { return m_externalCamera ? *m_externalCamera : *m_camera; }
    
    void SetCamera(Camera* camera) { m_externalCamera = camera; }
    
    GridRenderer& GetGridRenderer() { return *m_gridRenderer; }
    GateRenderer& GetGateRenderer() { return *m_gateRenderer; }
    WireRenderer& GetWireRenderer() { return *m_wireRenderer; }
    
private:
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<GridRenderer> m_gridRenderer;
    std::unique_ptr<GateRenderer> m_gateRenderer;
    std::unique_ptr<WireRenderer> m_wireRenderer;
    
    std::unique_ptr<Camera> m_camera;
    Camera* m_externalCamera;
    
    bool m_showGrid;
    bool m_initialized;
};