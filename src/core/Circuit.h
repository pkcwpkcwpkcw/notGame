#pragma once
#include "Types.h"
#include "Gate.h"
#include "Wire.h"
#include "GatePool.h"
#include "GridMap.h"
#include <unordered_map>
#include <vector>
#include <memory>

class Circuit {
private:
    std::unordered_map<GateId, Gate> gates;
    std::unordered_map<WireId, Wire> wires;
    
    GateId nextGateId{1};
    WireId nextWireId{1};
    
    float simulationTime{0.0f};
    bool isPaused{false};
    bool needsPropagation{false};
    
    std::vector<GateId> dirtyGates;
    std::vector<GateId> updateOrder;
    
public:
    Circuit() = default;
    ~Circuit() = default;
    
    Circuit(const Circuit&) = delete;
    Circuit& operator=(const Circuit&) = delete;
    Circuit(Circuit&&) = default;
    Circuit& operator=(Circuit&&) = default;
    
    [[nodiscard]] Result<GateId> addGate(Vec2 position) noexcept;
    ErrorCode removeGate(GateId id) noexcept;
    [[nodiscard]] Gate* getGate(GateId id) noexcept;
    [[nodiscard]] const Gate* getGate(GateId id) const noexcept;
    [[nodiscard]] GateId getGateAt(Vec2 position, float tolerance = 0.5f) const noexcept;
    
    [[nodiscard]] Result<WireId> connectGates(
        GateId fromId, GateId toId, PortIndex toPort) noexcept;
    ErrorCode removeWire(WireId id) noexcept;
    [[nodiscard]] Wire* getWire(WireId id) noexcept;
    [[nodiscard]] const Wire* getWire(WireId id) const noexcept;
    [[nodiscard]] WireId getWireAt(Vec2 position, float tolerance = 0.1f) const noexcept;
    
    void update(float deltaTime) noexcept;
    void pause() noexcept { isPaused = true; }
    void resume() noexcept { isPaused = false; }
    void reset() noexcept;
    
    [[nodiscard]] bool canPlaceGate(Vec2 position) const noexcept;
    [[nodiscard]] bool canConnect(
        GateId fromId, GateId toId, PortIndex toPort) const noexcept;
    [[nodiscard]] bool hasCircularDependency(
        GateId fromId, GateId toId) const noexcept;
    
    [[nodiscard]] size_t getGateCount() const noexcept { return gates.size(); }
    [[nodiscard]] size_t getWireCount() const noexcept { return wires.size(); }
    [[nodiscard]] float getSimulationTime() const noexcept { return simulationTime; }
    [[nodiscard]] bool isRunning() const noexcept { return !isPaused; }
    
    auto gatesBegin() noexcept { return gates.begin(); }
    auto gatesEnd() noexcept { return gates.end(); }
    auto wiresBegin() noexcept { return wires.begin(); }
    auto wiresEnd() noexcept { return wires.end(); }
    
    auto gatesBegin() const noexcept { return gates.begin(); }
    auto gatesEnd() const noexcept { return gates.end(); }
    auto wiresBegin() const noexcept { return wires.begin(); }
    auto wiresEnd() const noexcept { return wires.end(); }
    
private:
    void propagateSignals() noexcept;
    void updateGateInputs() noexcept;
    void updateTopologicalOrder() noexcept;
    void markGateDirty(GateId id) noexcept;
    void removeGateConnections(GateId id) noexcept;
};