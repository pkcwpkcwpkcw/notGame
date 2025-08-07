#pragma once

#include <glm/glm.hpp>

// 렌더링을 위한 임시 Wire 구조체
struct RenderWire {
    glm::vec2 start;
    glm::vec2 end;
    bool hasSignal;
    int fromGate;
    int fromPort;
    int toGate;
    int toPort;
    
    RenderWire() : start(0.0f), end(0.0f), hasSignal(false), 
                   fromGate(-1), fromPort(-1), toGate(-1), toPort(-1) {}
};