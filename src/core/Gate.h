#pragma once
#include "Types.h"
#include "Vec2.h"
#include <array>
#include <limits>

struct alignas(64) Gate {
    GateId id{Constants::INVALID_GATE_ID};
    GateType type{GateType::NOT};
    uint16_t _padding1{0};
    
    Vec2 position{0, 0};
    
    std::array<WireId, 3> inputWires{
        Constants::INVALID_WIRE_ID,
        Constants::INVALID_WIRE_ID,
        Constants::INVALID_WIRE_ID
    };
    WireId outputWire{Constants::INVALID_WIRE_ID};
    
    SignalState currentOutput{SignalState::LOW};
    SignalState pendingOutput{SignalState::LOW};
    float delayTimer{0.0f};
    uint16_t _padding2{0};
    
    bool isDirty{false};
    bool isDelayActive{false};
    bool isSelected{false};
    bool isHovered{false};
    uint8_t _padding3[4]{0};
    
    void update(float deltaTime) noexcept;
    [[nodiscard]] SignalState calculateOutput(
        const std::array<SignalState, 3>& inputs) const noexcept;
    
    [[nodiscard]] Vec2 getInputPortPosition(PortIndex port) const noexcept;
    [[nodiscard]] Vec2 getOutputPortPosition() const noexcept;
    [[nodiscard]] PortIndex getClosestInputPort(Vec2 pos) const noexcept;
    [[nodiscard]] bool isPointInBounds(Vec2 point) const noexcept;
    
    [[nodiscard]] bool canConnectInput(PortIndex port) const noexcept {
        return port >= 0 && port < Constants::MAX_INPUT_PORTS &&
               inputWires[port] == Constants::INVALID_WIRE_ID;
    }
    
    [[nodiscard]] bool canConnectOutput() const noexcept {
        return outputWire == Constants::INVALID_WIRE_ID;
    }
    
    void connectInput(PortIndex port, WireId wire) noexcept {
        if (port >= 0 && port < Constants::MAX_INPUT_PORTS) {
            inputWires[port] = wire;
            isDirty = true;
        }
    }
    
    void connectOutput(WireId wire) noexcept {
        outputWire = wire;
    }
    
    void disconnectInput(PortIndex port) noexcept {
        if (port >= 0 && port < Constants::MAX_INPUT_PORTS) {
            inputWires[port] = Constants::INVALID_WIRE_ID;
            isDirty = true;
        }
    }
    
    void disconnectOutput() noexcept {
        outputWire = Constants::INVALID_WIRE_ID;
    }
};

static_assert(sizeof(Gate) == 64, "Gate should be exactly one cache line");