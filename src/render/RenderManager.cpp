#include "RenderManager.h"
#include "render/Window.h"
#include "render/RenderTypes.h"
#include <iostream>
#include <SDL.h>

RenderManager::RenderManager()
    : m_showGrid(true)
    , m_initialized(false)
    , m_externalCamera(nullptr) {
}

RenderManager::~RenderManager() {
    Shutdown();
}

bool RenderManager::Initialize(Window* window) {
    if (m_initialized) {
        return true;
    }
    
    m_renderer = std::make_unique<Renderer>();
    if (!m_renderer->Initialize(window)) {
        std::cerr << "Failed to initialize renderer!" << std::endl;
        return false;
    }
    
    int width, height;
    window->GetSize(width, height);
    
    m_gridRenderer = std::make_unique<GridRenderer>();
    if (!m_gridRenderer->Initialize(width, height)) {
        std::cerr << "Failed to initialize grid renderer!" << std::endl;
        return false;
    }
    
    m_gateRenderer = std::make_unique<GateRenderer>();
    if (!m_gateRenderer->Initialize()) {
        std::cerr << "Failed to initialize gate renderer!" << std::endl;
        return false;
    }
    
    m_wireRenderer = std::make_unique<WireRenderer>();
    if (!m_wireRenderer->Initialize()) {
        std::cerr << "Failed to initialize wire renderer!" << std::endl;
        return false;
    }
    
    window->GetSize(width, height);
    m_camera = std::make_unique<Camera>(width, height);
    m_camera->SetPosition(glm::vec2(0.0f, 0.0f));
    m_camera->SetZoom(1.0f);
    
    m_gridRenderer->SetGridSize(1.0f);
    m_gridRenderer->SetGridColor(glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
    
    m_gateRenderer->SetGateSize(1.0f);
    m_gateRenderer->EnableInstancing(true);
    
    m_wireRenderer->SetLineWidth(2.0f);
    m_wireRenderer->SetAntialiasing(true);
    
    m_initialized = true;
    return true;
}

void RenderManager::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    m_wireRenderer.reset();
    m_gateRenderer.reset();
    m_gridRenderer.reset();
    m_renderer.reset();
    
    m_initialized = false;
}

void RenderManager::BeginFrame() {
    if (!m_initialized) {
        return;
    }
    
    m_renderer->BeginFrame();
    m_renderer->Clear(0.1f, 0.1f, 0.15f, 1.0f);
    
    m_gateRenderer->BeginFrame();
}

void RenderManager::EndFrame() {
    if (!m_initialized) {
        return;
    }
    
    m_gateRenderer->EndFrame();
    m_renderer->EndFrame();
}

void RenderManager::RenderCircuit(const Circuit& circuit) {
    if (!m_initialized) {
        return;
    }
    
    Camera& camera = m_externalCamera ? *m_externalCamera : *m_camera;
    
    if (m_showGrid) {
        m_gridRenderer->Render(camera);
    }
    
    // Circuit의 게이트와 와이어를 수집
    std::vector<Gate> gates;
    std::vector<RenderWire> renderWires;
    
    // 게이트 수집 - Copy the gates to preserve selection state
    for (auto it = circuit.gatesBegin(); it != circuit.gatesEnd(); ++it) {
        gates.push_back(it->second);  // This copies the gate including isSelected state
    }
    
    // 게이트 수집 완료
    
    // 와이어를 RenderWire로 변환
    for (auto it = circuit.wiresBegin(); it != circuit.wiresEnd(); ++it) {
        const auto& wire = it->second;
        
        RenderWire rw;
        
        // Check if this is a gate-to-gate wire or a cell-to-cell wire
        if (wire.fromGateId != Constants::INVALID_GATE_ID && 
            wire.toGateId != Constants::INVALID_GATE_ID) {
            // Gate-to-gate wire
            const Gate* fromGate = circuit.getGate(wire.fromGateId);
            const Gate* toGate = circuit.getGate(wire.toGateId);
            
            if (fromGate && toGate) {
                // 와이어는 게이트의 출력(오른쪽)에서 시작하여 다음 게이트의 입력(왼쪽)으로 연결
                // 출력 게이트의 오른쪽 셀에서 시작
                rw.start = glm::vec2(fromGate->position.x + 1.0f, fromGate->position.y);
                
                // 입력 포트에 따른 y 오프셋 계산
                float yOffset = 0.0f;
                if (wire.toPort == 0) {
                    yOffset = -0.3f;  // 상단 입력
                } else if (wire.toPort == 2) {
                    yOffset = 0.3f;   // 하단 입력
                }
                // toPort == 1일 때는 yOffset = 0 (중간 입력)
                
                // 입력 게이트의 왼쪽 셀로 연결 (포트에 따라 y 조정)
                rw.end = glm::vec2(toGate->position.x - 1.0f, toGate->position.y + yOffset);
            } else {
                continue; // Skip if gates don't exist
            }
        } else if (!wire.pathPoints.empty()) {
            // Cell-to-cell wire - render each segment
            if (wire.pathPoints.size() >= 2) {
                // Create a RenderWire for each segment of the path
                for (size_t i = 0; i < wire.pathPoints.size() - 1; ++i) {
                    RenderWire segment;
                    segment.start = glm::vec2(wire.pathPoints[i].x, wire.pathPoints[i].y);
                    segment.end = glm::vec2(wire.pathPoints[i + 1].x, wire.pathPoints[i + 1].y);
                    segment.hasSignal = (wire.signalState == SignalState::HIGH);
                    segment.fromGate = Constants::INVALID_GATE_ID;
                    segment.toGate = Constants::INVALID_GATE_ID;
                    segment.fromPort = Constants::INVALID_PORT;
                    segment.toPort = Constants::INVALID_PORT;
                    renderWires.push_back(segment);
                }
                continue; // Skip the main wire addition since we added segments
            } else {
                continue; // Skip if not enough path points
            }
        } else {
            continue; // Skip invalid wires
        }
        
        rw.hasSignal = (wire.signalState == SignalState::HIGH);
        rw.fromGate = wire.fromGateId;
        rw.toGate = wire.toGateId;
        rw.fromPort = wire.fromPort;
        rw.toPort = wire.toPort;
        renderWires.push_back(rw);
    }
    
    m_wireRenderer->RenderWires(renderWires, camera);
    m_gateRenderer->RenderGates(gates, camera);
}

void RenderManager::RenderCellWires(const std::unordered_map<uint64_t, CellWire>& cellWires) {
    if (!m_initialized) {
        return;
    }
    
    Camera& camera = m_externalCamera ? *m_externalCamera : *m_camera;
    std::vector<RenderWire> renderWires;
    
    // CellWire를 RenderWire로 변환
    for (const auto& [key, cellWire] : cellWires) {
        if (!cellWire.exists) continue;
        
        Vec2 center = cellWire.getCenterPos();
        glm::vec2 centerPos(center.x, center.y);
        
        // 각 연결 방향에 대해 선분 생성
        if (cellWire.hasConnection(WireDirection::Up)) {
            RenderWire segment;
            segment.start = centerPos;
            segment.end = glm::vec2(centerPos.x, centerPos.y - 0.5f);
            segment.hasSignal = cellWire.hasSignal;
            segment.fromGate = Constants::INVALID_GATE_ID;
            segment.toGate = Constants::INVALID_GATE_ID;
            renderWires.push_back(segment);
        }
        if (cellWire.hasConnection(WireDirection::Down)) {
            RenderWire segment;
            segment.start = centerPos;
            segment.end = glm::vec2(centerPos.x, centerPos.y + 0.5f);
            segment.hasSignal = cellWire.hasSignal;
            segment.fromGate = Constants::INVALID_GATE_ID;
            segment.toGate = Constants::INVALID_GATE_ID;
            renderWires.push_back(segment);
        }
        if (cellWire.hasConnection(WireDirection::Left)) {
            RenderWire segment;
            segment.start = centerPos;
            segment.end = glm::vec2(centerPos.x - 0.5f, centerPos.y);
            segment.hasSignal = cellWire.hasSignal;
            segment.fromGate = Constants::INVALID_GATE_ID;
            segment.toGate = Constants::INVALID_GATE_ID;
            renderWires.push_back(segment);
        }
        if (cellWire.hasConnection(WireDirection::Right)) {
            RenderWire segment;
            segment.start = centerPos;
            segment.end = glm::vec2(centerPos.x + 0.5f, centerPos.y);
            segment.hasSignal = cellWire.hasSignal;
            segment.fromGate = Constants::INVALID_GATE_ID;
            segment.toGate = Constants::INVALID_GATE_ID;
            renderWires.push_back(segment);
        }
        
        // 중앙 점도 작은 선분으로 표시 (점처럼 보이게)
        if (cellWire.connections == WireDirection::None || true) {  // 항상 중앙 점 표시
            RenderWire dot;
            dot.start = centerPos;
            dot.end = centerPos + glm::vec2(0.01f, 0.01f);  // 아주 작은 선분
            dot.hasSignal = cellWire.hasSignal;
            dot.fromGate = Constants::INVALID_GATE_ID;
            dot.toGate = Constants::INVALID_GATE_ID;
            renderWires.push_back(dot);
        }
    }
    
    // WireRenderer를 사용해 렌더링
    m_wireRenderer->RenderWires(renderWires, camera);
}

void RenderManager::RenderDraggingWire(const glm::vec2& start, const glm::vec2& end) {
    if (!m_initialized) {
        return;
    }
    
    Camera& camera = m_externalCamera ? *m_externalCamera : *m_camera;
    m_wireRenderer->RenderDraggingWire(start, end, camera);
}

void RenderManager::RenderGatePreview(const glm::vec2& position, GateType type, bool isValid) {
    if (!m_initialized || !m_gateRenderer) {
        return;
    }
    
    Camera& camera = m_externalCamera ? *m_externalCamera : *m_camera;
    m_gateRenderer->RenderGatePreview(position, type, isValid, camera);
}

void RenderManager::SetGridSize(float size) {
    if (m_gridRenderer) {
        m_gridRenderer->SetGridSize(size);
    }
    if (m_gateRenderer) {
        m_gateRenderer->SetGateSize(size);
    }
}

void RenderManager::SetGridColor(const glm::vec4& color) {
    if (m_gridRenderer) {
        m_gridRenderer->SetGridColor(color);
    }
}