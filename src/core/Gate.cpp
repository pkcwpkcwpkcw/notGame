#include "Gate.h"
#include <algorithm>

void Gate::update(float deltaTime) noexcept {
    if (!isDelayActive) return;
    
    delayTimer -= deltaTime;
    if (delayTimer <= 0.0f) {
        currentOutput = pendingOutput;
        isDelayActive = false;
        delayTimer = 0.0f;
    }
}

SignalState Gate::calculateOutput(
    const std::array<SignalState, 3>& inputs) const noexcept {
    
    for (const auto& input : inputs) {
        if (input == SignalState::HIGH) {
            return SignalState::LOW;
        }
    }
    return SignalState::HIGH;
}

Vec2 Gate::getInputPortPosition(PortIndex port) const noexcept {
    if (port < 0 || port >= Constants::MAX_INPUT_PORTS) {
        return position;
    }
    
    constexpr float PORT_SPACING = 0.3f;
    constexpr float PORT_OFFSET = 0.5f;
    
    float yOffset = (port - 1) * PORT_SPACING;
    return Vec2(position.x - PORT_OFFSET, position.y + yOffset);
}

Vec2 Gate::getOutputPortPosition() const noexcept {
    constexpr float PORT_OFFSET = 0.5f;
    return Vec2(position.x + PORT_OFFSET, position.y);
}

PortIndex Gate::getClosestInputPort(Vec2 pos) const noexcept {
    float minDist = std::numeric_limits<float>::max();
    PortIndex closestPort = Constants::INVALID_PORT;
    
    for (PortIndex i = 0; i < Constants::MAX_INPUT_PORTS; ++i) {
        Vec2 portPos = getInputPortPosition(i);
        float dist = pos.distanceSquared(portPos);
        if (dist < minDist) {
            minDist = dist;
            closestPort = i;
        }
    }
    
    return closestPort;
}

bool Gate::isPointInBounds(Vec2 point) const noexcept {
    constexpr float HALF_SIZE = 0.4f;
    return std::abs(point.x - position.x) <= HALF_SIZE &&
           std::abs(point.y - position.y) <= HALF_SIZE;
}