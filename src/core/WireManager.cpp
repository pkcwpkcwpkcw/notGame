#include "WireManager.h"
#include "WirePreviewSystem.h"
#include "PortHighlightSystem.h"
#include "WirePathCalculator.h"
#include "ConnectionValidator.h"
#include "core/Gate.h"
#include <algorithm>
#include <SDL.h>
#include <glm/glm.hpp>
#include <cmath>

WireManager::WireManager(Circuit* circuit)
    : m_circuit(circuit) {
    m_previewSystem = std::make_unique<WirePreviewSystem>(circuit);
    m_highlightSystem = std::make_unique<PortHighlightSystem>(circuit);
    m_pathCalculator = std::make_unique<WirePathCalculator>(circuit);
    m_validator = std::make_unique<ConnectionValidator>(circuit);
}

WireManager::~WireManager() = default;

void WireManager::initialize() noexcept {
    m_previewSystem->setSnapEnabled(true);
    m_previewSystem->setSnapDistance(m_snapDistance);
    m_previewSystem->setAnimationEnabled(true);
    
    m_highlightSystem->setHighlightRadius(m_highlightRadius);
    m_highlightSystem->setPulseEnabled(true);
    m_highlightSystem->setPortHoverCallback(
        [this](GateId gateId, PortIndex port) {
            if (isConnecting()) {
                bool isValid = canConnect(
                    m_context.sourceGateId, m_context.sourcePort,
                    gateId, port
                );
                m_previewSystem->setValidationResult(isValid);
            }
        }
    );
    
    m_pathCalculator->setGridSnapping(true);
    m_pathCalculator->setOptimizePath(true);
    
    m_validator->setAllowMultipleOutputs(true);
    m_validator->setAllowSelfConnection(false);
}

void WireManager::shutdown() noexcept {
    cancelWireConnection();
    m_highlightedPorts.clear();
}

void WireManager::onDragStart(const Input::DragEvent& event) noexcept {
    if (!m_circuit) return;
    
    // 와이어는 아무 곳에서나 시작할 수 있음
    Vec2 startPos{event.startWorld.x, event.startWorld.y};
    
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[WireManager] Wire drag started at (%.2f, %.2f)", 
                startPos.x, startPos.y);
    
    // 드래그 시작 위치 저장
    m_context.state = WireConnectionState::Connecting;
    m_context.sourcePos = startPos;
    m_context.currentMousePos = startPos;
    m_context.sourceGateId = Constants::INVALID_GATE_ID;
    m_context.sourcePort = Constants::INVALID_PORT;
    m_context.targetGateId = Constants::INVALID_GATE_ID;
    m_context.targetPort = Constants::INVALID_PORT;
    
    // Clear and start tracking path
    m_context.previewPath.clear();
    m_context.previewPath.push_back(startPos);
    m_dragPath.clear();
    m_dragPath.push_back(startPos);
    m_lastGridPos = glm::ivec2(std::floor(startPos.x), std::floor(startPos.y));
    
    m_previewSystem->startPreview(startPos, Constants::INVALID_GATE_ID, Constants::INVALID_PORT);
    updateState(WireConnectionState::Connecting);
}

void WireManager::onDragMove(const Input::DragEvent& event) noexcept {
    if (!isConnecting()) return;
    
    Vec2 currentPos{event.currentWorld.x, event.currentWorld.y};
    glm::ivec2 currentGridPos(std::floor(currentPos.x), std::floor(currentPos.y));
    
    // Check if we moved to a new grid cell
    if (currentGridPos != m_lastGridPos) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[WireManager] Moved to new cell: (%d, %d)", 
                    currentGridPos.x, currentGridPos.y);
        
        // Add intermediate cells if we skipped any
        std::vector<glm::ivec2> cellPath = bresenhamLine(m_lastGridPos, currentGridPos);
        
        for (const auto& gridPos : cellPath) {
            if (gridPos != m_lastGridPos) {  // Skip the starting cell as it's already in the path
                Vec2 cellCenter{gridPos.x + 0.5f, gridPos.y + 0.5f};
                m_dragPath.push_back(cellCenter);
            }
        }
        
        m_lastGridPos = currentGridPos;
    }
    
    updateWirePreview(currentPos);
}

void WireManager::onDragEnd(const Input::DragEvent& event) noexcept {
    if (!isConnecting()) return;
    if (!m_circuit) return;
    
    Vec2 endPos{event.currentWorld.x, event.currentWorld.y};
    
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[WireManager] Wire drag ended at (%.2f, %.2f)", 
                endPos.x, endPos.y);
    
    // Make sure the end position is included
    glm::ivec2 endGridPos(std::floor(endPos.x), std::floor(endPos.y));
    if (endGridPos != m_lastGridPos) {
        Vec2 cellCenter{endGridPos.x + 0.5f, endGridPos.y + 0.5f};
        m_dragPath.push_back(cellCenter);
    }
    
    // Create wire using the complete drag path
    if (m_dragPath.size() >= 2) {
        createPathWire(m_dragPath);
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[WireManager] Created wire with %zu points", 
                    m_dragPath.size());
    }
    
    cancelWireConnection();
}

void WireManager::onDragCancel(const Input::DragEvent& event) noexcept {
    cancelWireConnection();
}

void WireManager::onMouseMove(const Vec2& worldPos) noexcept {
    if (isConnecting()) {
        updateWirePreview(worldPos);
        m_highlightSystem->updateHighlights(worldPos);
    }
}

void WireManager::onClick(const Input::ClickEvent& event) noexcept {
    if (!m_circuit) return;
    
    if (event.button == static_cast<int>(Input::MouseButton::Right)) {
        if (event.hit.type == Input::ClickTarget::Wire) {
            WireId wireId = static_cast<WireId>(event.hit.objectId);
            deleteWire(wireId);
        }
    }
}

Result<WireId> WireManager::createWire(
    GateId fromGate, PortIndex fromPort,
    GateId toGate, PortIndex toPort) noexcept {
    
    if (!m_circuit) {
        return Result<WireId>{Constants::INVALID_WIRE_ID, ErrorCode::NOT_INITIALIZED};
    }
    
    auto validation = m_validator->validateConnection(fromGate, fromPort, toGate, toPort);
    if (!validation.isValid) {
        return Result<WireId>{Constants::INVALID_WIRE_ID, validation.errorCode};
    }
    
    auto result = m_circuit->connectGates(fromGate, toGate, toPort);
    
    if (result.success() && m_onWireCreated) {
        m_onWireCreated(result.value);
    }
    
    return result;
}

ErrorCode WireManager::deleteWire(WireId wireId) noexcept {
    if (!m_circuit) return ErrorCode::NOT_INITIALIZED;
    
    ErrorCode result = m_circuit->removeWire(wireId);
    
    if (result == ErrorCode::SUCCESS && m_onWireDeleted) {
        m_onWireDeleted(wireId);
    }
    
    return result;
}

ErrorCode WireManager::deleteWiresAt(Vec2 position, float tolerance) noexcept {
    if (!m_circuit) return ErrorCode::NOT_INITIALIZED;
    
    WireId wireId = m_circuit->getWireAt(position, tolerance);
    if (wireId != Constants::INVALID_WIRE_ID) {
        return deleteWire(wireId);
    }
    
    return ErrorCode::INVALID_ID;
}

ErrorCode WireManager::deleteWiresForGate(GateId gateId) noexcept {
    if (!m_circuit) return ErrorCode::NOT_INITIALIZED;
    
    std::vector<WireId> wiresToDelete;
    
    for (auto it = m_circuit->wiresBegin(); it != m_circuit->wiresEnd(); ++it) {
        const Wire& wire = it->second;
        if (wire.fromGateId == gateId || wire.toGateId == gateId) {
            wiresToDelete.push_back(wire.id);
        }
    }
    
    for (WireId wireId : wiresToDelete) {
        deleteWire(wireId);
    }
    
    return ErrorCode::SUCCESS;
}

void WireManager::startWireConnection(GateId gateId, PortIndex port, Vec2 startPos) noexcept {
    // This function is now only used for gate-to-gate connections
    // For cell-to-cell connections, see onDragStart
    if (!m_circuit) return;
    
    if (gateId != Constants::INVALID_GATE_ID) {
        const Gate* gate = m_circuit->getGate(gateId);
        if (!gate) return;
        
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[WireManager] Starting wire connection from gate %d port %d", 
                    gateId, port);
        
        m_context.state = WireConnectionState::Connecting;
        m_context.sourceGateId = gateId;
        m_context.sourcePort = port;
        if (port == Constants::OUTPUT_PORT) {
            m_context.sourcePos = gate->getOutputPortPosition();
        } else {
            m_context.sourcePos = gate->getInputPortPosition(port);
        }
        m_context.targetGateId = Constants::INVALID_GATE_ID;
        m_context.targetPort = Constants::INVALID_PORT;
        m_context.currentMousePos = startPos;
        m_context.isValid = false;
        m_context.validationError = ErrorCode::SUCCESS;
        
        m_previewSystem->startPreview(m_context.sourcePos, gateId, port);
        m_highlightSystem->startHighlighting(gateId, port);
    } else {
        // Cell-to-cell connection
        m_context.state = WireConnectionState::Connecting;
        m_context.sourceGateId = Constants::INVALID_GATE_ID;
        m_context.sourcePort = Constants::INVALID_PORT;
        m_context.sourcePos = startPos;
        m_context.targetGateId = Constants::INVALID_GATE_ID;
        m_context.targetPort = Constants::INVALID_PORT;
        m_context.currentMousePos = startPos;
        m_context.isValid = true;
        m_context.validationError = ErrorCode::SUCCESS;
        
        m_previewSystem->startPreview(startPos, Constants::INVALID_GATE_ID, Constants::INVALID_PORT);
    }
    
    updateState(WireConnectionState::Connecting);
    calculatePreviewPath();
}

void WireManager::updateWirePreview(Vec2 currentPos) noexcept {
    if (!isConnecting()) return;
    
    m_context.currentMousePos = currentPos;
    
    auto [nearestGate, nearestPort] = findNearestPort(currentPos, 
        m_context.sourcePort == Constants::OUTPUT_PORT);
    
    if (nearestGate != Constants::INVALID_GATE_ID && 
        nearestPort != Constants::INVALID_PORT) {
        m_context.targetGateId = nearestGate;
        m_context.targetPort = nearestPort;
        
        const Gate* gate = m_circuit->getGate(nearestGate);
        if (gate) {
            if (nearestPort == Constants::OUTPUT_PORT) {
                m_context.targetPos = gate->getOutputPortPosition();
            } else {
                m_context.targetPos = gate->getInputPortPosition(nearestPort);
            }
            currentPos = m_context.targetPos;
        }
        
        m_context.isValid = canConnect(
            m_context.sourceGateId, m_context.sourcePort,
            nearestGate, nearestPort
        );
    } else {
        m_context.targetGateId = Constants::INVALID_GATE_ID;
        m_context.targetPort = Constants::INVALID_PORT;
        m_context.targetPos = currentPos;
        m_context.isValid = false;
    }
    
    m_previewSystem->updatePreview(currentPos, m_context.targetGateId, m_context.targetPort);
    m_previewSystem->setValidationResult(m_context.isValid);
    
    updateState(WireConnectionState::Previewing);
    calculatePreviewPath();
}

void WireManager::completeWireConnection(GateId targetGate, PortIndex targetPort) noexcept {
    if (!isConnecting()) return;
    
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[WireManager] Completing wire connection to gate %d port %d", 
                targetGate, targetPort);
    
    auto result = createWire(
        m_context.sourceGateId, m_context.sourcePort,
        targetGate, targetPort
    );
    
    if (result.success()) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[WireManager] Wire created successfully with ID: %d", result.value);
        Wire* wire = m_circuit->getWire(result.value);
        if (wire) {
            wire->pathPoints = m_context.previewPath;
        }
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[WireManager] Failed to create wire, error code: %d", 
                     static_cast<int>(result.error));
    }
    
    cancelWireConnection();
}

void WireManager::cancelWireConnection() noexcept {
    m_context.state = WireConnectionState::Idle;
    m_context.sourceGateId = Constants::INVALID_GATE_ID;
    m_context.sourcePort = Constants::INVALID_PORT;
    m_context.targetGateId = Constants::INVALID_GATE_ID;
    m_context.targetPort = Constants::INVALID_PORT;
    m_context.previewPath.clear();
    
    m_previewSystem->cancelPreview();
    m_highlightSystem->stopHighlighting();
    m_highlightedPorts.clear();
    
    updateState(WireConnectionState::Idle);
}

std::vector<std::pair<GateId, PortIndex>> WireManager::getHighlightedPorts() const noexcept {
    std::vector<std::pair<GateId, PortIndex>> ports;
    
    for (const auto& highlight : m_highlightSystem->getHighlights()) {
        ports.push_back({highlight.gateId, highlight.portIndex});
    }
    
    return ports;
}

bool WireManager::canConnect(
    GateId fromGate, PortIndex fromPort,
    GateId toGate, PortIndex toPort) const noexcept {
    
    return m_validator->canConnect(fromGate, fromPort, toGate, toPort);
}

void WireManager::updateState(WireConnectionState newState) noexcept {
    if (m_context.state != newState) {
        m_context.state = newState;
        if (m_onStateChanged) {
            m_onStateChanged(newState);
        }
    }
}

void WireManager::updateHighlightedPorts() noexcept {
    if (!isConnecting()) {
        m_highlightedPorts.clear();
        return;
    }
    
    m_highlightedPorts.clear();
    for (const auto& highlight : m_highlightSystem->getHighlights()) {
        m_highlightedPorts.insert({highlight.gateId, highlight.portIndex});
    }
}

void WireManager::calculatePreviewPath() noexcept {
    if (!isConnecting()) return;
    
    Vec2 start = m_context.sourcePos;
    Vec2 end = (m_context.targetGateId != Constants::INVALID_GATE_ID) 
        ? m_context.targetPos 
        : m_context.currentMousePos;
    
    PathConstraints constraints;
    constraints.avoidGates = true;
    constraints.cornerRadius = m_enablePathSmoothing ? 5.0f : 0.0f;
    
    m_context.previewPath = m_pathCalculator->calculatePath(
        start, end, 
        PathStyle::Manhattan, 
        constraints
    );
}

Vec2 WireManager::getPortPosition(GateId gateId, PortIndex port) const noexcept {
    if (!m_circuit) return Vec2{0, 0};
    
    const Gate* gate = m_circuit->getGate(gateId);
    if (!gate) return Vec2{0, 0};
    
    if (port == Constants::OUTPUT_PORT) {
        return gate->getOutputPortPosition();
    } else {
        return gate->getInputPortPosition(port);
    }
}

bool WireManager::isPortAvailable(GateId gateId, PortIndex port) const noexcept {
    return m_validator->isPortAvailable(gateId, port);
}

void WireManager::createCellToCellWire(Vec2 startPos, Vec2 endPos) noexcept {
    if (!m_circuit) return;
    
    // Create a new wire that connects two cells
    WireId wireId = m_circuit->getNextWireId();
    
    Wire wire;
    wire.id = wireId;
    wire.fromGateId = Constants::INVALID_GATE_ID;  // Not connected to a gate
    wire.toGateId = Constants::INVALID_GATE_ID;     // Not connected to a gate  
    wire.fromPort = Constants::INVALID_PORT;
    wire.toPort = Constants::INVALID_PORT;
    
    // Calculate wire path (straight line or Manhattan path)
    wire.pathPoints.push_back(startPos);
    
    // Use Manhattan path for better visual
    if (std::abs(endPos.x - startPos.x) > 0.01f || std::abs(endPos.y - startPos.y) > 0.01f) {
        // Horizontal first, then vertical
        wire.pathPoints.push_back(Vec2{endPos.x, startPos.y});
    }
    
    wire.pathPoints.push_back(endPos);
    
    // Add wire to circuit
    ErrorCode result = m_circuit->addWire(wire);
    
    if (result == ErrorCode::SUCCESS) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[WireManager] Cell-to-cell wire created with ID: %d from (%.2f,%.2f) to (%.2f,%.2f)", 
                    wireId, startPos.x, startPos.y, endPos.x, endPos.y);
        
        if (m_onWireCreated) {
            m_onWireCreated(wireId);
        }
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[WireManager] Failed to create cell-to-cell wire");
    }
}

void WireManager::createPathWire(const std::vector<Vec2>& path) noexcept {
    if (!m_circuit || path.size() < 2) return;
    
    // Create a wire that follows the complete drag path
    WireId wireId = m_circuit->getNextWireId();
    
    Wire wire;
    wire.id = wireId;
    wire.fromGateId = Constants::INVALID_GATE_ID;
    wire.toGateId = Constants::INVALID_GATE_ID;
    wire.fromPort = Constants::INVALID_PORT;
    wire.toPort = Constants::INVALID_PORT;
    
    // Use the complete path from dragging
    wire.pathPoints = path;
    
    // Add wire to circuit
    ErrorCode result = m_circuit->addWire(wire);
    
    if (result == ErrorCode::SUCCESS) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[WireManager] Path wire created with ID: %d, %zu points", 
                    wireId, path.size());
        
        if (m_onWireCreated) {
            m_onWireCreated(wireId);
        }
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[WireManager] Failed to create path wire");
    }
}

std::vector<glm::ivec2> WireManager::bresenhamLine(glm::ivec2 start, glm::ivec2 end) const noexcept {
    std::vector<glm::ivec2> cells;
    
    int x0 = start.x;
    int y0 = start.y;
    int x1 = end.x;
    int y1 = end.y;
    
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    
    while (true) {
        cells.push_back(glm::ivec2(x0, y0));
        
        if (x0 == x1 && y0 == y1) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
    
    return cells;
}

std::pair<GateId, PortIndex> WireManager::findNearestPort(
    Vec2 position, bool inputPorts) const noexcept {
    
    if (!m_circuit) return {Constants::INVALID_GATE_ID, Constants::INVALID_PORT};
    
    GateId nearestGate = Constants::INVALID_GATE_ID;
    PortIndex nearestPort = Constants::INVALID_PORT;
    float minDistance = m_snapDistance;
    
    for (auto it = m_circuit->gatesBegin(); it != m_circuit->gatesEnd(); ++it) {
        GateId gateId = it->first;
        const Gate& gate = it->second;
        
        if (gateId == m_context.sourceGateId) continue;
        
        if (inputPorts) {
            for (PortIndex i = 0; i < Constants::MAX_INPUT_PORTS; ++i) {
                Vec2 portPos = gate.getInputPortPosition(i);
                float dist = (portPos - position).length();
                
                if (dist < minDistance && isPortAvailable(gateId, i)) {
                    minDistance = dist;
                    nearestGate = gateId;
                    nearestPort = i;
                }
            }
        } else {
            Vec2 portPos = gate.getOutputPortPosition();
            float dist = (portPos - position).length();
            
            if (dist < minDistance && isPortAvailable(gateId, Constants::OUTPUT_PORT)) {
                minDistance = dist;
                nearestGate = gateId;
                nearestPort = Constants::OUTPUT_PORT;
            }
        }
    }
    
    return {nearestGate, nearestPort};
}