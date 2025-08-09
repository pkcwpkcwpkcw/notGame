#include "WirePreviewSystem.h"
#include "core/Circuit.h"
#include "core/Gate.h"
#include <algorithm>
#include <cmath>

WirePreviewSystem::WirePreviewSystem(Circuit* circuit)
    : m_circuit(circuit) {
}

void WirePreviewSystem::startPreview(Vec2 startPos, GateId sourceGate, PortIndex sourcePort) noexcept {
    m_preview.startPos = startPos;
    m_preview.endPos = startPos;
    m_preview.state = PreviewState::Active;
    m_preview.isSnapped = false;
    m_preview.opacity = 1.0f;
    
    m_sourceGate = sourceGate;
    m_sourcePort = sourcePort;
    m_targetGate = Constants::INVALID_GATE_ID;
    m_targetPort = Constants::INVALID_PORT;
    
    m_animationTime = 0.0f;
    
    calculatePath();
}

void WirePreviewSystem::updatePreview(Vec2 currentPos, GateId targetGate, PortIndex targetPort) noexcept {
    if (m_preview.state == PreviewState::Inactive) return;
    
    Vec2 snappedPos = currentPos;
    
    if (targetGate != Constants::INVALID_GATE_ID && targetPort != Constants::INVALID_PORT) {
        snappedPos = snapToPort(currentPos, targetGate, targetPort);
        m_preview.isSnapped = true;
        m_targetGate = targetGate;
        m_targetPort = targetPort;
    } else if (m_snapEnabled) {
        snappedPos = snapToGrid(currentPos);
        m_preview.isSnapped = false;
        m_targetGate = Constants::INVALID_GATE_ID;
        m_targetPort = Constants::INVALID_PORT;
    } else {
        m_preview.isSnapped = false;
        m_targetGate = Constants::INVALID_GATE_ID;
        m_targetPort = Constants::INVALID_PORT;
    }
    
    m_preview.endPos = snappedPos;
    calculatePath();
}

void WirePreviewSystem::endPreview() noexcept {
    m_preview.state = PreviewState::Inactive;
    m_preview.pathPoints.clear();
    m_sourceGate = Constants::INVALID_GATE_ID;
    m_sourcePort = Constants::INVALID_PORT;
    m_targetGate = Constants::INVALID_GATE_ID;
    m_targetPort = Constants::INVALID_PORT;
}

void WirePreviewSystem::cancelPreview() noexcept {
    endPreview();
}

void WirePreviewSystem::update(float deltaTime) noexcept {
    if (m_preview.state == PreviewState::Inactive) return;
    
    if (m_animationEnabled) {
        m_animationTime += deltaTime;
        
        if (m_preview.state == PreviewState::Invalid) {
            m_preview.opacity = 0.5f + 0.3f * std::sin(m_animationTime * 5.0f);
        } else {
            m_preview.opacity = 0.8f + 0.2f * std::sin(m_animationTime * 2.0f);
        }
    }
}

void WirePreviewSystem::calculatePath() noexcept {
    m_preview.pathPoints.clear();
    
    Vec2 start = m_preview.startPos;
    Vec2 end = m_preview.endPos;
    
    float dx = end.x - start.x;
    float dy = end.y - start.y;
    
    if (std::abs(dx) < 0.01f && std::abs(dy) < 0.01f) {
        m_preview.pathPoints.push_back(start);
        m_preview.pathPoints.push_back(end);
        return;
    }
    
    m_preview.pathPoints.push_back(start);
    
    if (std::abs(dx) > std::abs(dy)) {
        float midX = start.x + dx * 0.5f;
        m_preview.pathPoints.push_back(Vec2{midX, start.y});
        m_preview.pathPoints.push_back(Vec2{midX, end.y});
    } else {
        float midY = start.y + dy * 0.5f;
        m_preview.pathPoints.push_back(Vec2{start.x, midY});
        m_preview.pathPoints.push_back(Vec2{end.x, midY});
    }
    
    m_preview.pathPoints.push_back(end);
}

Vec2 WirePreviewSystem::snapToGrid(Vec2 pos) const noexcept {
    float gridSize = Constants::GRID_CELL_SIZE;
    return Vec2{
        std::round(pos.x / gridSize) * gridSize,
        std::round(pos.y / gridSize) * gridSize
    };
}

Vec2 WirePreviewSystem::snapToPort(Vec2 pos, GateId gateId, PortIndex port) const noexcept {
    if (!m_circuit) return pos;
    
    const Gate* gate = m_circuit->getGate(gateId);
    if (!gate) return pos;
    
    Vec2 portPos;
    if (port == Constants::OUTPUT_PORT) {
        portPos = gate->getOutputPortPosition();
    } else {
        portPos = gate->getInputPortPosition(port);
    }
    
    float distance = (portPos - pos).length();
    if (distance <= m_snapDistance) {
        return portPos;
    }
    
    return pos;
}