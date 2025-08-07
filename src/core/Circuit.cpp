#include "Circuit.h"
#include <algorithm>
#include <queue>
#include <unordered_set>

Result<GateId> Circuit::addGate(Vec2 position) noexcept {
    if (!canPlaceGate(position)) {
        return {Constants::INVALID_GATE_ID, ErrorCode::POSITION_OCCUPIED};
    }
    
    Gate gate;
    gate.id = nextGateId++;
    gate.type = GateType::NOT;
    gate.position = position;
    
    gates[gate.id] = std::move(gate);
    needsPropagation = true;
    
    return {gate.id, ErrorCode::SUCCESS};
}

ErrorCode Circuit::removeGate(GateId id) noexcept {
    auto it = gates.find(id);
    if (it == gates.end()) {
        return ErrorCode::INVALID_ID;
    }
    
    removeGateConnections(id);
    gates.erase(it);
    updateTopologicalOrder();
    
    return ErrorCode::SUCCESS;
}

Gate* Circuit::getGate(GateId id) noexcept {
    auto it = gates.find(id);
    return it != gates.end() ? &it->second : nullptr;
}

const Gate* Circuit::getGate(GateId id) const noexcept {
    auto it = gates.find(id);
    return it != gates.end() ? &it->second : nullptr;
}

GateId Circuit::getGateAt(Vec2 position, float tolerance) const noexcept {
    for (const auto& [id, gate] : gates) {
        if (gate.position.distance(position) <= tolerance) {
            return id;
        }
    }
    return Constants::INVALID_GATE_ID;
}

Result<WireId> Circuit::connectGates(
    GateId fromId, GateId toId, PortIndex toPort) noexcept {
    
    if (!canConnect(fromId, toId, toPort)) {
        return {Constants::INVALID_WIRE_ID, ErrorCode::PORT_ALREADY_CONNECTED};
    }
    
    if (hasCircularDependency(fromId, toId)) {
        return {Constants::INVALID_WIRE_ID, ErrorCode::CIRCULAR_DEPENDENCY};
    }
    
    Wire wire;
    wire.id = nextWireId++;
    wire.fromGateId = fromId;
    wire.toGateId = toId;
    wire.fromPort = Constants::OUTPUT_PORT;
    wire.toPort = toPort;
    
    gates[fromId].connectOutput(wire.id);
    gates[toId].connectInput(toPort, wire.id);
    
    Vec2 fromPos = gates[fromId].getOutputPortPosition();
    Vec2 toPos = gates[toId].getInputPortPosition(toPort);
    wire.calculatePath(fromPos, toPos);
    
    wires[wire.id] = std::move(wire);
    
    markGateDirty(toId);
    updateTopologicalOrder();
    
    return {wire.id, ErrorCode::SUCCESS};
}

ErrorCode Circuit::removeWire(WireId id) noexcept {
    auto it = wires.find(id);
    if (it == wires.end()) {
        return ErrorCode::INVALID_ID;
    }
    
    Wire& wire = it->second;
    
    if (auto* fromGate = getGate(wire.fromGateId)) {
        fromGate->disconnectOutput();
    }
    if (auto* toGate = getGate(wire.toGateId)) {
        toGate->disconnectInput(wire.toPort);
        markGateDirty(wire.toGateId);
    }
    
    wires.erase(it);
    updateTopologicalOrder();
    
    return ErrorCode::SUCCESS;
}

Wire* Circuit::getWire(WireId id) noexcept {
    auto it = wires.find(id);
    return it != wires.end() ? &it->second : nullptr;
}

const Wire* Circuit::getWire(WireId id) const noexcept {
    auto it = wires.find(id);
    return it != wires.end() ? &it->second : nullptr;
}

WireId Circuit::getWireAt(Vec2 position, float tolerance) const noexcept {
    for (const auto& [id, wire] : wires) {
        if (wire.isPointOnWire(position, tolerance)) {
            return id;
        }
    }
    return Constants::INVALID_WIRE_ID;
}

void Circuit::update(float deltaTime) noexcept {
    if (isPaused) return;
    
    for (auto& [id, gate] : gates) {
        gate.update(deltaTime);
    }
    
    if (needsPropagation || !dirtyGates.empty()) {
        propagateSignals();
    }
    
    simulationTime += deltaTime;
}

void Circuit::reset() noexcept {
    for (auto& [id, gate] : gates) {
        gate.currentOutput = SignalState::LOW;
        gate.pendingOutput = SignalState::LOW;
        gate.delayTimer = 0.0f;
        gate.isDelayActive = false;
        gate.isDirty = true;
    }
    
    for (auto& [id, wire] : wires) {
        wire.signalState = SignalState::LOW;
    }
    
    simulationTime = 0.0f;
    needsPropagation = true;
}

bool Circuit::canPlaceGate(Vec2 position) const noexcept {
    constexpr float MIN_DISTANCE = 1.0f;
    
    for (const auto& [id, gate] : gates) {
        if (gate.position.distance(position) < MIN_DISTANCE) {
            return false;
        }
    }
    return true;
}

bool Circuit::canConnect(
    GateId fromId, GateId toId, PortIndex toPort) const noexcept {
    
    if (fromId == toId) return false;
    
    auto fromIt = gates.find(fromId);
    auto toIt = gates.find(toId);
    
    if (fromIt == gates.end() || toIt == gates.end()) {
        return false;
    }
    
    const Gate& fromGate = fromIt->second;
    const Gate& toGate = toIt->second;
    
    if (!fromGate.canConnectOutput()) {
        return false;
    }
    
    if (!toGate.canConnectInput(toPort)) {
        return false;
    }
    
    return true;
}

bool Circuit::hasCircularDependency(
    GateId fromId, GateId toId) const noexcept {
    
    std::queue<GateId> toCheck;
    std::unordered_set<GateId> visited;
    
    toCheck.push(toId);
    visited.insert(toId);
    
    while (!toCheck.empty()) {
        GateId current = toCheck.front();
        toCheck.pop();
        
        if (current == fromId) {
            return true;
        }
        
        const Gate* gate = getGate(current);
        if (!gate) continue;
        
        if (gate->outputWire != Constants::INVALID_WIRE_ID) {
            const Wire* wire = getWire(gate->outputWire);
            if (wire && visited.find(wire->toGateId) == visited.end()) {
                toCheck.push(wire->toGateId);
                visited.insert(wire->toGateId);
            }
        }
    }
    
    return false;
}

void Circuit::propagateSignals() noexcept {
    updateGateInputs();
    
    for (GateId gateId : updateOrder) {
        Gate& gate = gates[gateId];
        
        if (!gate.isDirty) continue;
        
        std::array<SignalState, 3> inputs{
            SignalState::FLOATING,
            SignalState::FLOATING,
            SignalState::FLOATING
        };
        
        for (int i = 0; i < Constants::MAX_INPUT_PORTS; ++i) {
            WireId wireId = gate.inputWires[i];
            if (wireId != Constants::INVALID_WIRE_ID) {
                inputs[i] = wires[wireId].signalState;
            }
        }
        
        SignalState newOutput = gate.calculateOutput(inputs);
        
        if (newOutput != gate.currentOutput && !gate.isDelayActive) {
            gate.pendingOutput = newOutput;
            gate.delayTimer = Constants::GATE_DELAY;
            gate.isDelayActive = true;
        }
        
        if (gate.outputWire != Constants::INVALID_WIRE_ID) {
            Wire& outWire = wires[gate.outputWire];
            if (outWire.signalState != gate.currentOutput) {
                outWire.signalState = gate.currentOutput;
                markGateDirty(outWire.toGateId);
            }
        }
        
        gate.isDirty = false;
    }
    
    dirtyGates.clear();
    needsPropagation = false;
}

void Circuit::updateGateInputs() noexcept {
    for (auto& [id, gate] : gates) {
        gate.isDirty = true;
    }
}

void Circuit::updateTopologicalOrder() noexcept {
    updateOrder.clear();
    updateOrder.reserve(gates.size());
    
    for (const auto& [id, gate] : gates) {
        updateOrder.push_back(id);
    }
}

void Circuit::markGateDirty(GateId id) noexcept {
    if (gates.find(id) != gates.end()) {
        dirtyGates.push_back(id);
        needsPropagation = true;
    }
}

void Circuit::removeGateConnections(GateId id) noexcept {
    Gate* gate = getGate(id);
    if (!gate) return;
    
    for (int i = 0; i < Constants::MAX_INPUT_PORTS; ++i) {
        if (gate->inputWires[i] != Constants::INVALID_WIRE_ID) {
            removeWire(gate->inputWires[i]);
        }
    }
    
    if (gate->outputWire != Constants::INVALID_WIRE_ID) {
        removeWire(gate->outputWire);
    }
}