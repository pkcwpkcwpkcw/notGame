#pragma once

#include <glm/glm.hpp>
#include <cstdint>
#include <functional>

namespace Input {

enum class ClickTarget {
    None,
    Gate,
    Wire,
    Port,
    Empty
};

enum class DragPhase {
    None,
    Start,
    Move,
    End,
    Cancel
};

enum class MouseButton {
    Left = 0,
    Middle = 1,
    Right = 2
};

struct HitResult {
    ClickTarget type = ClickTarget::None;
    uint32_t objectId = 0;
    float distance = FLT_MAX;
    glm::vec2 hitPoint{0, 0};
    int portIndex = -1;  
    bool isInput = false; 
};

struct MouseEvent {
    enum Type { Move, Down, Up, Wheel } type;
    glm::vec2 screenPos{0, 0};
    glm::vec2 worldPos{0, 0};
    glm::ivec2 gridPos{0, 0};
    int button = -1;
    float wheelDelta = 0;
    uint32_t timestamp = 0;
};

struct ClickEvent {
    glm::vec2 worldPos{0, 0};
    glm::ivec2 gridPos{0, 0};
    HitResult hit;
    int button = 0;
    bool doubleClick = false;
    uint32_t timestamp = 0;
};

struct DragEvent {
    DragPhase phase = DragPhase::None;
    glm::vec2 startWorld{0, 0};
    glm::vec2 currentWorld{0, 0};
    glm::vec2 deltaWorld{0, 0};
    glm::ivec2 startGrid{0, 0};
    glm::ivec2 currentGrid{0, 0};
    HitResult dragTarget;
    float distance = 0;
    float duration = 0;
    int button = 0;
};

struct HoverEvent {
    glm::vec2 worldPos{0, 0};
    glm::ivec2 gridPos{0, 0};
    HitResult hit;
    HitResult previousHit;
};

template<typename EventType>
using EventCallback = std::function<void(const EventType&)>;

} // namespace Input