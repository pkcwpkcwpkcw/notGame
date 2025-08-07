#include "Wire.h"
#include <algorithm>
#include <limits>

void Wire::calculatePath(const Vec2& fromPos, const Vec2& toPos) noexcept {
    pathPoints.clear();
    pathPoints.reserve(4);
    
    pathPoints.push_back(fromPos);
    
    float dx = std::abs(toPos.x - fromPos.x);
    float dy = std::abs(toPos.y - fromPos.y);
    
    if (dx < 0.01f || dy < 0.01f) {
        pathPoints.push_back(toPos);
    } else {
        Vec2 mid1(toPos.x, fromPos.y);
        pathPoints.push_back(mid1);
        pathPoints.push_back(toPos);
    }
}

bool Wire::isPointOnWire(Vec2 point, float tolerance) const noexcept {
    if (pathPoints.size() < 2) return false;
    
    for (size_t i = 0; i < pathPoints.size() - 1; ++i) {
        const Vec2& p1 = pathPoints[i];
        const Vec2& p2 = pathPoints[i + 1];
        
        Vec2 diff = p2 - p1;
        float t = std::clamp(
            (point - p1).dot(diff) / diff.lengthSquared(),
            0.0f, 1.0f
        );
        
        Vec2 closest = p1 + diff * t;
        if (closest.distance(point) <= tolerance) {
            return true;
        }
    }
    
    return false;
}

float Wire::distanceToPoint(Vec2 point) const noexcept {
    if (pathPoints.empty()) return std::numeric_limits<float>::max();
    
    float minDist = std::numeric_limits<float>::max();
    
    for (size_t i = 0; i < pathPoints.size() - 1; ++i) {
        const Vec2& p1 = pathPoints[i];
        const Vec2& p2 = pathPoints[i + 1];
        
        Vec2 diff = p2 - p1;
        float t = std::clamp(
            (point - p1).dot(diff) / diff.lengthSquared(),
            0.0f, 1.0f
        );
        
        Vec2 closest = p1 + diff * t;
        float dist = closest.distance(point);
        minDist = std::min(minDist, dist);
    }
    
    return minDist;
}