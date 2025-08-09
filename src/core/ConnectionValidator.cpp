#include "ConnectionValidator.h"
#include "core/Circuit.h"
#include "core/Gate.h"
#include "core/Wire.h"
#include <algorithm>
#include <cmath>

ConnectionValidator::ConnectionValidator(Circuit* circuit)
    : m_circuit(circuit) {
}

ValidationResult ConnectionValidator::validateConnection(
    GateId fromGate, PortIndex fromPort,
    GateId toGate, PortIndex toPort) const noexcept {
    
    if (!m_circuit) {
        return ValidationResult::Invalid(ErrorCode::NOT_INITIALIZED, 
            "Circuit not initialized");
    }
    
    if (fromGate == Constants::INVALID_GATE_ID || toGate == Constants::INVALID_GATE_ID) {
        return ValidationResult::Invalid(ErrorCode::INVALID_ID, 
            "Invalid gate ID");
    }
    
    if (fromGate == toGate && !m_allowSelfConnection) {
        return ValidationResult::Invalid(ErrorCode::INVALID_ID, 
            "Self-connection not allowed");
    }
    
    const Gate* sourceGate = m_circuit->getGate(fromGate);
    const Gate* targetGate = m_circuit->getGate(toGate);
    
    if (!sourceGate || !targetGate) {
        return ValidationResult::Invalid(ErrorCode::INVALID_ID, 
            "Gate not found");
    }
    
    if (!isValidPort(fromGate, fromPort) || !isValidPort(toGate, toPort)) {
        return ValidationResult::Invalid(ErrorCode::INVALID_ID, 
            "Invalid port index");
    }
    
    bool isFromOutput = (fromPort == Constants::OUTPUT_PORT);
    bool isToInput = (toPort >= 0 && toPort < Constants::MAX_INPUT_PORTS);
    
    if (!isFromOutput || !isToInput) {
        return ValidationResult::Invalid(ErrorCode::INVALID_ID, 
            "Can only connect output to input");
    }
    
    if (!isPortAvailable(toGate, toPort)) {
        return ValidationResult::Invalid(ErrorCode::PORT_ALREADY_CONNECTED, 
            "Port already connected");
    }
    
    if (wouldCreateCycle(fromGate, toGate)) {
        return ValidationResult::Invalid(ErrorCode::CIRCULAR_DEPENDENCY, 
            "Connection would create a cycle");
    }
    
    float distance = calculateDistance(fromGate, toGate);
    if (distance > m_maxWireLength) {
        return ValidationResult::Invalid(ErrorCode::OUT_OF_BOUNDS, 
            "Wire too long");
    }
    
    return ValidationResult::Valid();
}

bool ConnectionValidator::canConnect(
    GateId fromGate, PortIndex fromPort,
    GateId toGate, PortIndex toPort) const noexcept {
    
    return validateConnection(fromGate, fromPort, toGate, toPort).isValid;
}

bool ConnectionValidator::isPortAvailable(GateId gateId, PortIndex port) const noexcept {
    if (!m_circuit) return false;
    
    const Gate* gate = m_circuit->getGate(gateId);
    if (!gate) return false;
    
    if (port == Constants::OUTPUT_PORT) {
        if (!m_allowMultipleOutputs) {
            return gate->canConnectOutput();
        }
        return true;
    }
    
    if (port >= 0 && port < Constants::MAX_INPUT_PORTS) {
        return gate->canConnectInput(port);
    }
    
    return false;
}

bool ConnectionValidator::wouldCreateCycle(GateId fromGate, GateId toGate) const noexcept {
    if (!m_circuit) return false;
    
    std::unordered_set<GateId> visited;
    std::unordered_set<GateId> recursionStack;
    
    return detectCycleDFS(toGate, fromGate, visited, recursionStack);
}

bool ConnectionValidator::isValidPort(GateId gateId, PortIndex port) const noexcept {
    if (!m_circuit) return false;
    
    const Gate* gate = m_circuit->getGate(gateId);
    if (!gate) return false;
    
    if (port == Constants::OUTPUT_PORT) {
        return true;
    }
    
    if (port >= 0 && port < Constants::MAX_INPUT_PORTS) {
        return true;
    }
    
    return false;
}

bool ConnectionValidator::areGatesAdjacent(GateId gate1, GateId gate2) const noexcept {
    if (!m_circuit) return false;
    
    const Gate* g1 = m_circuit->getGate(gate1);
    const Gate* g2 = m_circuit->getGate(gate2);
    
    if (!g1 || !g2) return false;
    
    Vec2 pos1 = g1->position;
    Vec2 pos2 = g2->position;
    
    float dx = std::abs(pos1.x - pos2.x);
    float dy = std::abs(pos1.y - pos2.y);
    
    return (dx <= Constants::GRID_CELL_SIZE * 1.5f && dy < 0.1f) ||
           (dy <= Constants::GRID_CELL_SIZE * 1.5f && dx < 0.1f);
}

size_t ConnectionValidator::getConnectionCount(GateId gateId) const noexcept {
    if (!m_circuit) return 0;
    
    size_t count = 0;
    
    for (auto it = m_circuit->wiresBegin(); it != m_circuit->wiresEnd(); ++it) {
        const Wire& wire = it->second;
        if (wire.fromGateId == gateId || wire.toGateId == gateId) {
            count++;
        }
    }
    
    return count;
}

std::vector<WireId> ConnectionValidator::getIncomingWires(GateId gateId) const noexcept {
    std::vector<WireId> incoming;
    
    if (!m_circuit) return incoming;
    
    for (auto it = m_circuit->wiresBegin(); it != m_circuit->wiresEnd(); ++it) {
        const Wire& wire = it->second;
        if (wire.toGateId == gateId) {
            incoming.push_back(wire.id);
        }
    }
    
    return incoming;
}

std::vector<WireId> ConnectionValidator::getOutgoingWires(GateId gateId) const noexcept {
    std::vector<WireId> outgoing;
    
    if (!m_circuit) return outgoing;
    
    for (auto it = m_circuit->wiresBegin(); it != m_circuit->wiresEnd(); ++it) {
        const Wire& wire = it->second;
        if (wire.fromGateId == gateId) {
            outgoing.push_back(wire.id);
        }
    }
    
    return outgoing;
}

bool ConnectionValidator::detectCycleDFS(
    GateId current, GateId target,
    std::unordered_set<GateId>& visited,
    std::unordered_set<GateId>& recursionStack) const noexcept {
    
    if (!m_circuit) return false;
    
    if (current == target) {
        return true;
    }
    
    if (visited.find(current) != visited.end()) {
        return false;
    }
    
    visited.insert(current);
    recursionStack.insert(current);
    
    auto outgoing = getOutgoingWires(current);
    for (WireId wireId : outgoing) {
        const Wire* wire = m_circuit->getWire(wireId);
        if (wire) {
            if (recursionStack.find(wire->toGateId) != recursionStack.end()) {
                return true;
            }
            if (detectCycleDFS(wire->toGateId, target, visited, recursionStack)) {
                return true;
            }
        }
    }
    
    recursionStack.erase(current);
    return false;
}

float ConnectionValidator::calculateDistance(GateId gate1, GateId gate2) const noexcept {
    if (!m_circuit) return FLT_MAX;
    
    const Gate* g1 = m_circuit->getGate(gate1);
    const Gate* g2 = m_circuit->getGate(gate2);
    
    if (!g1 || !g2) return FLT_MAX;
    
    Vec2 pos1 = g1->position;
    Vec2 pos2 = g2->position;
    
    return (pos2 - pos1).length();
}