#include "WirePathCalculator.h"
#include "core/Circuit.h"
#include "core/Gate.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_map>

WirePathCalculator::WirePathCalculator(Circuit* circuit)
    : m_circuit(circuit) {
    m_nodePool.reserve(1000);
    for (size_t i = 0; i < 1000; ++i) {
        m_nodePool.push_back(std::make_unique<PathNode>());
    }
}

std::vector<Vec2> WirePathCalculator::calculatePath(
    Vec2 start, Vec2 end,
    PathStyle style,
    const PathConstraints& constraints) noexcept {
    
    switch (style) {
        case PathStyle::Direct:
            return calculateDirectPath(start, end);
        case PathStyle::Manhattan:
            return calculateManhattanPath(start, end);
        case PathStyle::Smart:
            return calculateSmartPath(start, end, constraints);
        case PathStyle::Curved:
            {
                auto path = calculateManhattanPath(start, end);
                return smoothPath(path, constraints.cornerRadius);
            }
        default:
            return calculateManhattanPath(start, end);
    }
}

std::vector<Vec2> WirePathCalculator::calculateDirectPath(Vec2 start, Vec2 end) const noexcept {
    std::vector<Vec2> path;
    path.push_back(start);
    path.push_back(end);
    return path;
}

std::vector<Vec2> WirePathCalculator::calculateManhattanPath(Vec2 start, Vec2 end) const noexcept {
    std::vector<Vec2> path;
    
    if (m_gridSnapping) {
        start = snapToGrid(start);
        end = snapToGrid(end);
    }
    
    path.push_back(start);
    
    float dx = end.x - start.x;
    float dy = end.y - start.y;
    
    if (std::abs(dx) < 0.01f && std::abs(dy) < 0.01f) {
        path.push_back(end);
        return path;
    }
    
    if (std::abs(dx) > std::abs(dy)) {
        float midX = start.x + dx * 0.6f;
        path.push_back(Vec2{midX, start.y});
        path.push_back(Vec2{midX, end.y});
    } else {
        float midY = start.y + dy * 0.6f;
        path.push_back(Vec2{start.x, midY});
        path.push_back(Vec2{end.x, midY});
    }
    
    path.push_back(end);
    
    if (m_optimizePath) {
        return optimizePath(path);
    }
    
    return path;
}

std::vector<Vec2> WirePathCalculator::calculateSmartPath(
    Vec2 start, Vec2 end,
    const PathConstraints& constraints) noexcept {
    
    resetNodePool();
    
    if (m_gridSnapping) {
        start = snapToGrid(start);
        end = snapToGrid(end);
    }
    
    auto cmp = [](const PathNode* a, const PathNode* b) {
        return a->fCost() > b->fCost();
    };
    std::priority_queue<PathNode*, std::vector<PathNode*>, decltype(cmp)> openSet(cmp);
    std::unordered_map<uint64_t, PathNode*> closedSet;
    
    auto posToKey = [this](Vec2 pos) -> uint64_t {
        int gridX = static_cast<int>(pos.x / m_gridSize);
        int gridY = static_cast<int>(pos.y / m_gridSize);
        return (static_cast<uint64_t>(gridX) << 32) | static_cast<uint64_t>(gridY);
    };
    
    PathNode* startNode = getNode();
    startNode->position = start;
    startNode->gCost = 0;
    startNode->hCost = heuristic(start, end);
    startNode->parent = nullptr;
    
    openSet.push(startNode);
    
    while (!openSet.empty()) {
        PathNode* current = openSet.top();
        openSet.pop();
        
        if ((current->position - end).length() < m_gridSize * 0.5f) {
            return reconstructPath(current);
        }
        
        uint64_t currentKey = posToKey(current->position);
        if (closedSet.find(currentKey) != closedSet.end()) {
            continue;
        }
        closedSet[currentKey] = current;
        
        auto neighbors = getNeighbors(current, end);
        for (PathNode* neighbor : neighbors) {
            uint64_t neighborKey = posToKey(neighbor->position);
            if (closedSet.find(neighborKey) != closedSet.end()) {
                continue;
            }
            
            if (constraints.avoidGates && isPositionBlocked(neighbor->position)) {
                continue;
            }
            
            float newGCost = current->gCost + (neighbor->position - current->position).length();
            
            if (newGCost < neighbor->gCost) {
                neighbor->parent = current;
                neighbor->gCost = newGCost;
                neighbor->hCost = heuristic(neighbor->position, end);
                openSet.push(neighbor);
            }
        }
    }
    
    return calculateManhattanPath(start, end);
}

std::vector<Vec2> WirePathCalculator::smoothPath(
    const std::vector<Vec2>& path,
    float cornerRadius) const noexcept {
    
    if (path.size() < 3 || cornerRadius < 0.01f) {
        return path;
    }
    
    std::vector<Vec2> smoothed;
    smoothed.push_back(path[0]);
    
    for (size_t i = 1; i < path.size() - 1; ++i) {
        Vec2 prev = path[i - 1];
        Vec2 curr = path[i];
        Vec2 next = path[i + 1];
        
        Vec2 dir1 = (curr - prev).normalized();
        Vec2 dir2 = (next - curr).normalized();
        
        float dot = dir1.dot(dir2);
        if (std::abs(dot) > 0.99f) {
            smoothed.push_back(curr);
            continue;
        }
        
        float dist1 = (curr - prev).length();
        float dist2 = (next - curr).length();
        float radius = std::min({cornerRadius, dist1 * 0.4f, dist2 * 0.4f});
        
        Vec2 corner1 = curr - dir1 * radius;
        Vec2 corner2 = curr + dir2 * radius;
        
        smoothed.push_back(corner1);
        
        for (int j = 1; j < 5; ++j) {
            float t = j / 5.0f;
            Vec2 bezier = corner1 * (1 - t) * (1 - t) +
                         curr * 2 * t * (1 - t) +
                         corner2 * t * t;
            smoothed.push_back(bezier);
        }
        
        smoothed.push_back(corner2);
    }
    
    smoothed.push_back(path.back());
    return smoothed;
}

float WirePathCalculator::calculatePathLength(const std::vector<Vec2>& path) const noexcept {
    if (path.size() < 2) return 0.0f;
    
    float length = 0.0f;
    for (size_t i = 1; i < path.size(); ++i) {
        length += (path[i] - path[i - 1]).length();
    }
    return length;
}

bool WirePathCalculator::isPathClear(
    const std::vector<Vec2>& path,
    float clearance) const noexcept {
    
    if (!m_circuit) return true;
    
    for (size_t i = 1; i < path.size(); ++i) {
        Vec2 start = path[i - 1];
        Vec2 end = path[i];
        Vec2 dir = (end - start).normalized();
        float dist = (end - start).length();
        
        for (float t = 0; t < dist; t += clearance * 0.5f) {
            Vec2 point = start + dir * t;
            if (isPositionBlocked(point)) {
                return false;
            }
        }
    }
    
    return true;
}

std::vector<Vec2> WirePathCalculator::reconstructPath(PathNode* endNode) const noexcept {
    std::vector<Vec2> path;
    PathNode* current = endNode;
    
    while (current != nullptr) {
        path.push_back(current->position);
        current = current->parent;
    }
    
    std::reverse(path.begin(), path.end());
    return path;
}

std::vector<WirePathCalculator::PathNode*> WirePathCalculator::getNeighbors(
    PathNode* node, Vec2 target) noexcept {
    
    std::vector<PathNode*> neighbors;
    
    const float step = m_gridSize;
    const Vec2 offsets[] = {
        {step, 0}, {-step, 0}, {0, step}, {0, -step}
    };
    
    for (const auto& offset : offsets) {
        Vec2 newPos = node->position + offset;
        
        PathNode* neighbor = getNode();
        neighbor->position = newPos;
        neighbor->gCost = FLT_MAX;
        neighbor->hCost = 0;
        neighbor->parent = nullptr;
        
        neighbors.push_back(neighbor);
    }
    
    return neighbors;
}

float WirePathCalculator::heuristic(Vec2 a, Vec2 b) const noexcept {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

bool WirePathCalculator::isPositionBlocked(Vec2 pos) const noexcept {
    if (!m_circuit) return false;
    
    GateId gateId = m_circuit->getGateAt(pos, m_gridSize * 0.4f);
    return gateId != Constants::INVALID_GATE_ID;
}

Vec2 WirePathCalculator::snapToGrid(Vec2 pos) const noexcept {
    return Vec2{
        std::round(pos.x / m_gridSize) * m_gridSize,
        std::round(pos.y / m_gridSize) * m_gridSize
    };
}

std::vector<Vec2> WirePathCalculator::optimizePath(const std::vector<Vec2>& path) const noexcept {
    if (path.size() <= 2) return path;
    
    std::vector<Vec2> optimized;
    optimized.push_back(path[0]);
    
    for (size_t i = 1; i < path.size() - 1; ++i) {
        Vec2 prev = path[i - 1];
        Vec2 curr = path[i];
        Vec2 next = path[i + 1];
        
        Vec2 dir1 = (curr - prev).normalized();
        Vec2 dir2 = (next - curr).normalized();
        
        float dot = dir1.dot(dir2);
        if (std::abs(dot - 1.0f) > 0.01f) {
            optimized.push_back(curr);
        }
    }
    
    optimized.push_back(path.back());
    return optimized;
}

WirePathCalculator::PathNode* WirePathCalculator::getNode() noexcept {
    if (m_nextNodeIndex >= m_nodePool.size()) {
        m_nodePool.push_back(std::make_unique<PathNode>());
    }
    return m_nodePool[m_nextNodeIndex++].get();
}

void WirePathCalculator::resetNodePool() noexcept {
    m_nextNodeIndex = 0;
}