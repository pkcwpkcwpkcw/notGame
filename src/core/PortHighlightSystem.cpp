#include "PortHighlightSystem.h"
#include "core/Circuit.h"
#include "core/Gate.h"
#include <algorithm>
#define _USE_MATH_DEFINES
#include <cmath>

PortHighlightSystem::PortHighlightSystem(Circuit* circuit)
    : m_circuit(circuit) {
}

void PortHighlightSystem::startHighlighting(GateId sourceGate, PortIndex sourcePort) noexcept {
    m_sourceGate = sourceGate;
    m_sourcePort = sourcePort;
    m_isHighlighting = true;
    
    clearAllHighlights();
    updateCompatibility();
}

void PortHighlightSystem::updateHighlights(Vec2 mousePos) noexcept {
    if (!m_isHighlighting || !m_circuit) return;
    
    updateProximity(mousePos);
}

void PortHighlightSystem::stopHighlighting() noexcept {
    m_isHighlighting = false;
    m_sourceGate = Constants::INVALID_GATE_ID;
    m_sourcePort = Constants::INVALID_PORT;
    clearAllHighlights();
}

void PortHighlightSystem::highlightPort(GateId gateId, PortIndex port, HighlightType type) noexcept {
    if (!m_circuit) return;
    
    const Gate* gate = m_circuit->getGate(gateId);
    if (!gate) return;
    
    auto key = std::make_pair(gateId, port);
    auto it = m_highlightMap.find(key);
    
    if (it != m_highlightMap.end()) {
        m_activeHighlights[it->second].type = type;
        m_activeHighlights[it->second].intensity = 1.0f;
    } else {
        PortHighlight highlight;
        highlight.gateId = gateId;
        highlight.portIndex = port;
        if (port == Constants::OUTPUT_PORT) {
            highlight.position = gate->getOutputPortPosition();
        } else {
            highlight.position = gate->getInputPortPosition(port);
        }
        highlight.type = type;
        highlight.intensity = 1.0f;
        highlight.pulsePhase = 0.0f;
        
        m_highlightMap[key] = m_activeHighlights.size();
        m_activeHighlights.push_back(highlight);
    }
}

void PortHighlightSystem::clearPortHighlight(GateId gateId, PortIndex port) noexcept {
    auto key = std::make_pair(gateId, port);
    auto it = m_highlightMap.find(key);
    
    if (it != m_highlightMap.end()) {
        size_t index = it->second;
        
        if (index < m_activeHighlights.size() - 1) {
            m_activeHighlights[index] = m_activeHighlights.back();
            auto movedKey = std::make_pair(
                m_activeHighlights[index].gateId,
                m_activeHighlights[index].portIndex
            );
            m_highlightMap[movedKey] = index;
        }
        
        m_activeHighlights.pop_back();
        m_highlightMap.erase(it);
    }
}

void PortHighlightSystem::clearAllHighlights() noexcept {
    m_activeHighlights.clear();
    m_highlightMap.clear();
}

bool PortHighlightSystem::isHighlighted(GateId gateId, PortIndex port) const noexcept {
    auto key = std::make_pair(gateId, port);
    return m_highlightMap.find(key) != m_highlightMap.end();
}

HighlightType PortHighlightSystem::getHighlightType(GateId gateId, PortIndex port) const noexcept {
    auto key = std::make_pair(gateId, port);
    auto it = m_highlightMap.find(key);
    
    if (it != m_highlightMap.end() && it->second < m_activeHighlights.size()) {
        return m_activeHighlights[it->second].type;
    }
    
    return HighlightType::None;
}

void PortHighlightSystem::update(float deltaTime) noexcept {
    if (!m_isHighlighting) return;
    
    updateAnimations(deltaTime);
}

void PortHighlightSystem::updateCompatibility() noexcept {
    if (!m_circuit) return;
    
    bool isSourceOutput = (m_sourcePort == Constants::OUTPUT_PORT);
    
    for (auto it = m_circuit->gatesBegin(); it != m_circuit->gatesEnd(); ++it) {
        GateId gateId = it->first;
        if (gateId == m_sourceGate) continue;
        
        if (isSourceOutput) {
            for (PortIndex i = 0; i < Constants::MAX_INPUT_PORTS; ++i) {
                if (isPortCompatible(gateId, i)) {
                    highlightPort(gateId, i, HighlightType::Compatible);
                }
            }
        } else {
            if (isPortCompatible(gateId, Constants::OUTPUT_PORT)) {
                highlightPort(gateId, Constants::OUTPUT_PORT, HighlightType::Compatible);
            }
        }
    }
}

void PortHighlightSystem::updateProximity(Vec2 mousePos) noexcept {
    if (!m_circuit) return;
    
    GateId closestGate = Constants::INVALID_GATE_ID;
    PortIndex closestPort = Constants::INVALID_PORT;
    float closestDistance = m_highlightRadius;
    
    for (auto& highlight : m_activeHighlights) {
        float distance = calculateDistance(mousePos, highlight.gateId, highlight.portIndex);
        
        if (distance < closestDistance) {
            closestDistance = distance;
            closestGate = highlight.gateId;
            closestPort = highlight.portIndex;
        }
        
        if (distance < m_highlightRadius * 0.5f) {
            highlight.type = HighlightType::Hover;
            highlight.intensity = 1.0f;
        } else if (highlight.type == HighlightType::Hover) {
            highlight.type = HighlightType::Compatible;
            highlight.intensity = 0.7f;
        }
    }
    
    if (closestGate != Constants::INVALID_GATE_ID && m_onPortHover) {
        m_onPortHover(closestGate, closestPort);
    }
}

void PortHighlightSystem::updateAnimations(float deltaTime) noexcept {
    for (auto& highlight : m_activeHighlights) {
        if (m_pulseEnabled) {
            highlight.pulsePhase += deltaTime * m_pulseSpeed;
            if (highlight.pulsePhase > 2 * 3.14159265359) {
                highlight.pulsePhase -= 2 * 3.14159265359;
            }
            
            if (highlight.type == HighlightType::Hover) {
                highlight.intensity = 0.7f + 0.3f * std::sin(highlight.pulsePhase);
            } else if (highlight.type == HighlightType::Compatible) {
                highlight.intensity = 0.5f + 0.2f * std::sin(highlight.pulsePhase);
            }
        }
        
        if (highlight.type == HighlightType::None) {
            highlight.intensity = std::max(0.0f, highlight.intensity - deltaTime * m_fadeSpeed);
        }
    }
    
    m_activeHighlights.erase(
        std::remove_if(m_activeHighlights.begin(), m_activeHighlights.end(),
            [](const PortHighlight& h) { return h.intensity <= 0.0f; }),
        m_activeHighlights.end()
    );
}

bool PortHighlightSystem::isPortCompatible(GateId gateId, PortIndex port) const noexcept {
    if (!m_circuit) return false;
    
    const Gate* gate = m_circuit->getGate(gateId);
    if (!gate) return false;
    
    bool isSourceOutput = (m_sourcePort == Constants::OUTPUT_PORT);
    bool isTargetInput = (port >= 0 && port < Constants::MAX_INPUT_PORTS);
    
    if (isSourceOutput && !isTargetInput) return false;
    if (!isSourceOutput && isTargetInput) return false;
    
    if (isTargetInput && !gate->canConnectInput(port)) {
        return false;
    }
    
    if (m_circuit->hasCircularDependency(m_sourceGate, gateId)) {
        return false;
    }
    
    return true;
}

float PortHighlightSystem::calculateDistance(Vec2 mousePos, GateId gateId, PortIndex port) const noexcept {
    if (!m_circuit) return FLT_MAX;
    
    const Gate* gate = m_circuit->getGate(gateId);
    if (!gate) return FLT_MAX;
    
    Vec2 portPos;
    if (port == Constants::OUTPUT_PORT) {
        portPos = gate->getOutputPortPosition();
    } else {
        portPos = gate->getInputPortPosition(port);
    }
    return (portPos - mousePos).length();
}