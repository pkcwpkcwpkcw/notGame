#pragma once

#include "core/Types.h"
#include "core/Vec2.h"
#include <vector>
#include <memory>

class Circuit;

enum class PathStyle {
    Direct,
    Manhattan,
    Smart,
    Curved
};

struct PathConstraints {
    bool avoidGates{true};
    bool avoidWires{false};
    float minSegmentLength{5.0f};
    float cornerRadius{0.0f};
    int maxSegments{10};
};

class WirePathCalculator {
public:
    explicit WirePathCalculator(Circuit* circuit);
    ~WirePathCalculator() = default;
    
    [[nodiscard]] std::vector<Vec2> calculatePath(
        Vec2 start, Vec2 end,
        PathStyle style = PathStyle::Manhattan,
        const PathConstraints& constraints = PathConstraints{}) noexcept;
    
    [[nodiscard]] std::vector<Vec2> calculateDirectPath(
        Vec2 start, Vec2 end) const noexcept;
    
    [[nodiscard]] std::vector<Vec2> calculateManhattanPath(
        Vec2 start, Vec2 end) const noexcept;
    
    [[nodiscard]] std::vector<Vec2> calculateSmartPath(
        Vec2 start, Vec2 end,
        const PathConstraints& constraints) noexcept;
    
    [[nodiscard]] std::vector<Vec2> smoothPath(
        const std::vector<Vec2>& path,
        float cornerRadius) const noexcept;
    
    [[nodiscard]] float calculatePathLength(
        const std::vector<Vec2>& path) const noexcept;
    
    [[nodiscard]] bool isPathClear(
        const std::vector<Vec2>& path,
        float clearance = 1.0f) const noexcept;
    
    void setGridSnapping(bool enable) noexcept { m_gridSnapping = enable; }
    void setGridSize(float size) noexcept { m_gridSize = size; }
    void setOptimizePath(bool enable) noexcept { m_optimizePath = enable; }
    
private:
    struct PathNode {
        Vec2 position;
        float gCost;
        float hCost;
        float fCost() const { return gCost + hCost; }
        PathNode* parent;
    };
    
    [[nodiscard]] std::vector<Vec2> reconstructPath(PathNode* endNode) const noexcept;
    [[nodiscard]] std::vector<PathNode*> getNeighbors(
        PathNode* node, Vec2 target) noexcept;
    [[nodiscard]] float heuristic(Vec2 a, Vec2 b) const noexcept;
    [[nodiscard]] bool isPositionBlocked(Vec2 pos) const noexcept;
    
    Vec2 snapToGrid(Vec2 pos) const noexcept;
    std::vector<Vec2> optimizePath(const std::vector<Vec2>& path) const noexcept;
    std::vector<Vec2> insertCorners(Vec2 start, Vec2 end) const noexcept;
    
    Circuit* m_circuit;
    bool m_gridSnapping{true};
    float m_gridSize{Constants::GRID_CELL_SIZE};
    bool m_optimizePath{true};
    
    std::vector<std::unique_ptr<PathNode>> m_nodePool;
    size_t m_nextNodeIndex{0};
    
    PathNode* getNode() noexcept;
    void resetNodePool() noexcept;
};