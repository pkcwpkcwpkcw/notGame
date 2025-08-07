#pragma once

#include "InputTypes.h"
#include "../core/Circuit.h"
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace Input {

class HitDetector {
private:
    Circuit* m_circuit = nullptr;
    float m_wireHitThreshold = 0.1f;
    float m_portHitRadius = 0.2f;
    
    struct SpatialGrid {
        static constexpr int CELL_SIZE = 10;
        std::unordered_map<uint64_t, std::vector<uint32_t>> gateIndex;
        std::unordered_map<uint64_t, std::vector<uint32_t>> wireIndex;
        
        uint64_t hashPosition(const glm::ivec2& pos) const {
            int32_t cellX = pos.x / CELL_SIZE;
            int32_t cellY = pos.y / CELL_SIZE;
            uint32_t ux = static_cast<uint32_t>(cellX + 0x7FFFFFFF);
            uint32_t uy = static_cast<uint32_t>(cellY + 0x7FFFFFFF);
            return (static_cast<uint64_t>(ux) << 32) | static_cast<uint64_t>(uy);
        }
        
        void clear() {
            gateIndex.clear();
            wireIndex.clear();
        }
        
        void insertGate(uint32_t id, const glm::ivec2& pos) {
            uint64_t hash = hashPosition(pos);
            gateIndex[hash].push_back(id);
        }
        
        void insertWire(uint32_t id, const std::vector<Vec2>& pathPoints) {
            if (pathPoints.size() < 2) return;
            
            glm::vec2 start(pathPoints.front().x, pathPoints.front().y);
            glm::vec2 end(pathPoints.back().x, pathPoints.back().y);
            glm::ivec2 minGrid(
                static_cast<int>(std::floor(std::min(start.x, end.x))),
                static_cast<int>(std::floor(std::min(start.y, end.y)))
            );
            glm::ivec2 maxGrid(
                static_cast<int>(std::ceil(std::max(start.x, end.x))),
                static_cast<int>(std::ceil(std::max(start.y, end.y)))
            );
            
            for (int y = minGrid.y; y <= maxGrid.y; y++) {
                for (int x = minGrid.x; x <= maxGrid.x; x++) {
                    uint64_t hash = hashPosition(glm::ivec2(x, y));
                    wireIndex[hash].push_back(id);
                }
            }
        }
    };
    
    mutable SpatialGrid m_spatialGrid;
    mutable bool m_gridDirty = true;
    
public:
    HitDetector() = default;
    
    void setCircuit(Circuit* circuit) {
        m_circuit = circuit;
        m_gridDirty = true;
    }
    
    void setWireHitThreshold(float threshold) {
        m_wireHitThreshold = threshold;
    }
    
    void setPortHitRadius(float radius) {
        m_portHitRadius = radius;
    }
    
    void invalidateGrid() {
        m_gridDirty = true;
    }
    
    HitResult detectHit(const glm::vec2& worldPos) const {
        if (!m_circuit) {
            return HitResult{ClickTarget::Empty, 0, 0, worldPos};
        }
        
        rebuildGridIfNeeded();
        
        if (auto gateHit = checkGateHit(worldPos); gateHit.type != ClickTarget::None) {
            return gateHit;
        }
        
        if (auto wireHit = checkWireHit(worldPos); wireHit.type != ClickTarget::None) {
            return wireHit;
        }
        
        return HitResult{ClickTarget::Empty, 0, 0, worldPos};
    }
    
    HitResult checkPortHit(const glm::vec2& worldPos, uint32_t gateId) const {
        if (!m_circuit) return HitResult{};
        
        Gate* gate = m_circuit->getGate(gateId);
        if (!gate) return HitResult{};
        HitResult result{ClickTarget::None, gateId, FLT_MAX, worldPos};
        
        for (int i = 0; i < 3; i++) {
            glm::vec2 inputPos = glm::vec2(gate->position.x, gate->position.y) + glm::vec2(-0.4f, -0.3f + i * 0.3f);
            float dist = glm::length(worldPos - inputPos);
            if (dist < m_portHitRadius && dist < result.distance) {
                result.type = ClickTarget::Port;
                result.distance = dist;
                result.portIndex = i;
                result.isInput = true;
                result.hitPoint = inputPos;
            }
        }
        
        glm::vec2 outputPos = glm::vec2(gate->position.x, gate->position.y) + glm::vec2(0.4f, 0.0f);
        float dist = glm::length(worldPos - outputPos);
        if (dist < m_portHitRadius && dist < result.distance) {
            result.type = ClickTarget::Port;
            result.distance = dist;
            result.portIndex = 0;
            result.isInput = false;
            result.hitPoint = outputPos;
        }
        
        return result;
    }
    
private:
    void rebuildGridIfNeeded() const {
        if (!m_gridDirty || !m_circuit) return;
        
        m_spatialGrid.clear();
        
        for (auto it = m_circuit->gatesBegin(); it != m_circuit->gatesEnd(); ++it) {
            const Gate& gate = it->second;
            m_spatialGrid.insertGate(gate.id, glm::ivec2(gate.position.x, gate.position.y));
        }
        
        for (auto it = m_circuit->wiresBegin(); it != m_circuit->wiresEnd(); ++it) {
            const Wire& wire = it->second;
            m_spatialGrid.insertWire(wire.id, wire.pathPoints);
        }
        
        m_gridDirty = false;
    }
    
    HitResult checkGateHit(const glm::vec2& worldPos) const {
        glm::ivec2 gridPos(std::floor(worldPos.x), std::floor(worldPos.y));
        uint64_t cellHash = m_spatialGrid.hashPosition(gridPos);
        
        auto it = m_spatialGrid.gateIndex.find(cellHash);
        if (it == m_spatialGrid.gateIndex.end()) {
            return HitResult{};
        }
        
        for (uint32_t gateId : it->second) {
            const Gate* gate = m_circuit->getGate(gateId);
            
            if (gate) {
                glm::vec2 gatePos(gate->position.x, gate->position.y);
                glm::vec2 min = gatePos - glm::vec2(0.5f);
                glm::vec2 max = gatePos + glm::vec2(0.5f);
                
                if (worldPos.x >= min.x && worldPos.x <= max.x &&
                    worldPos.y >= min.y && worldPos.y <= max.y) {
                    return HitResult{ClickTarget::Gate, gateId, 0, gatePos};
                }
            }
        }
        
        return HitResult{};
    }
    
    HitResult checkWireHit(const glm::vec2& worldPos) const {
        HitResult closest{ClickTarget::None, 0, FLT_MAX, worldPos};
        
        glm::ivec2 gridPos(std::floor(worldPos.x), std::floor(worldPos.y));
        
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                glm::ivec2 checkPos = gridPos + glm::ivec2(dx, dy);
                uint64_t cellHash = m_spatialGrid.hashPosition(checkPos);
                
                auto it = m_spatialGrid.wireIndex.find(cellHash);
                if (it == m_spatialGrid.wireIndex.end()) continue;
                
                for (uint32_t wireId : it->second) {
                    const Wire* wire = m_circuit->getWire(wireId);
                    
                    if (wire) {
                        if (wire->pathPoints.size() >= 2) {
                            // Check distance to each segment of the wire path
                            for (size_t i = 0; i < wire->pathPoints.size() - 1; i++) {
                                glm::vec2 segStart(wire->pathPoints[i].x, wire->pathPoints[i].y);
                                glm::vec2 segEnd(wire->pathPoints[i + 1].x, wire->pathPoints[i + 1].y);
                                float dist = distanceToLineSegment(worldPos, segStart, segEnd);
                                
                                if (dist < m_wireHitThreshold && dist < closest.distance) {
                                    closest.type = ClickTarget::Wire;
                                    closest.objectId = wireId;
                                    closest.distance = dist;
                                    
                                    glm::vec2 ab = segEnd - segStart;
                                    glm::vec2 ap = worldPos - segStart;
                                    float t = glm::clamp(glm::dot(ap, ab) / glm::dot(ab, ab), 0.0f, 1.0f);
                                    closest.hitPoint = segStart + t * ab;
                                }
                            }
                        }
                    }
                }
            }
        }
        
        return closest;
    }
    
    float distanceToLineSegment(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b) const {
        glm::vec2 ab = b - a;
        float lengthSq = glm::dot(ab, ab);
        
        if (lengthSq < 0.0001f) {
            return glm::length(p - a);
        }
        
        glm::vec2 ap = p - a;
        float t = glm::clamp(glm::dot(ap, ab) / lengthSq, 0.0f, 1.0f);
        glm::vec2 closest = a + t * ab;
        return glm::length(p - closest);
    }
};

} // namespace Input