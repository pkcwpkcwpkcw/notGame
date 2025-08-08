#pragma once
#include "Types.h"
#include "Vec2.h"
#include <unordered_map>
#include <array>
#include <vector>
#include <functional>

struct Vec2iHash {
    std::size_t operator()(const Vec2i& vec) const noexcept {
        return std::hash<int32_t>()(vec.x) ^ (std::hash<int32_t>()(vec.y) << 1);
    }
};

class GridMap {
private:
    static constexpr int CHUNK_SIZE = 32;
    static constexpr uint32_t INVALID_ID = 0;
    
    struct Chunk {
        std::array<uint32_t, CHUNK_SIZE * CHUNK_SIZE> cells;
        bool isDirty;
        
        Chunk() : isDirty(false) {
            cells.fill(INVALID_ID);
        }
    };
    
    std::unordered_map<Vec2i, Chunk, Vec2iHash> chunks;
    Vec2i worldMin{-1000, -1000};
    Vec2i worldMax{1000, 1000};
    
public:
    GridMap() = default;
    ~GridMap() = default;
    
    [[nodiscard]] uint32_t getCell(Vec2i pos) const noexcept;
    void setCell(Vec2i pos, uint32_t id) noexcept;
    void clearCell(Vec2i pos) noexcept;
    
    [[nodiscard]] bool isOccupied(Vec2i pos) const noexcept {
        return getCell(pos) != INVALID_ID;
    }
    
    [[nodiscard]] bool isInBounds(Vec2i pos) const noexcept {
        return pos.x >= worldMin.x && pos.x <= worldMax.x &&
               pos.y >= worldMin.y && pos.y <= worldMax.y;
    }
    
    void setBounds(Vec2i min, Vec2i max) noexcept {
        worldMin = min;
        worldMax = max;
    }
    
    void markChunkDirty(Vec2i chunkCoord) noexcept;
    [[nodiscard]] bool isChunkDirty(Vec2i chunkCoord) const noexcept;
    void clearDirtyFlag(Vec2i chunkCoord) noexcept;
    
    [[nodiscard]] std::vector<Vec2i> getDirtyChunks() const noexcept;
    
    void clear() noexcept;
    
    [[nodiscard]] Vec2i worldToChunk(Vec2i worldPos) const noexcept {
        return Vec2i{
            worldPos.x >= 0 ? worldPos.x / CHUNK_SIZE : (worldPos.x - CHUNK_SIZE + 1) / CHUNK_SIZE,
            worldPos.y >= 0 ? worldPos.y / CHUNK_SIZE : (worldPos.y - CHUNK_SIZE + 1) / CHUNK_SIZE
        };
    }
    
    [[nodiscard]] Vec2i worldToLocal(Vec2i worldPos) const noexcept {
        Vec2i local;
        local.x = ((worldPos.x % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
        local.y = ((worldPos.y % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
        return local;
    }
    
private:
    Chunk* getChunk(Vec2i chunkCoord) noexcept;
    const Chunk* getChunk(Vec2i chunkCoord) const noexcept;
    Chunk* getOrCreateChunk(Vec2i chunkCoord) noexcept;
};