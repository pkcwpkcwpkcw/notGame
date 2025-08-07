#pragma once
#include "Types.h"
#include "Vec2.h"
#include <vector>

struct Wire {
    WireId id{Constants::INVALID_WIRE_ID};
    
    GateId fromGateId{Constants::INVALID_GATE_ID};
    GateId toGateId{Constants::INVALID_GATE_ID};
    PortIndex fromPort{Constants::OUTPUT_PORT};
    PortIndex toPort{Constants::INVALID_PORT};
    
    SignalState signalState{SignalState::LOW};
    
    std::vector<Vec2> pathPoints;
    
    void calculatePath(const Vec2& fromPos, const Vec2& toPos) noexcept;
    [[nodiscard]] bool isPointOnWire(Vec2 point, float tolerance = 0.1f) const noexcept;
    [[nodiscard]] float distanceToPoint(Vec2 point) const noexcept;
    
    [[nodiscard]] bool isValid() const noexcept {
        return id != Constants::INVALID_WIRE_ID &&
               fromGateId != Constants::INVALID_GATE_ID &&
               toGateId != Constants::INVALID_GATE_ID &&
               toPort >= 0 && toPort < Constants::MAX_INPUT_PORTS;
    }
};