#include "GridMap.h"

uint32_t GridMap::getCell(Vec2i pos) const noexcept {
    if (!isInBounds(pos)) {
        return INVALID_ID;
    }
    
    Vec2i chunkCoord = worldToChunk(pos);
    const Chunk* chunk = getChunk(chunkCoord);
    
    if (!chunk) {
        return INVALID_ID;
    }
    
    Vec2i localPos = worldToLocal(pos);
    size_t index = localPos.y * CHUNK_SIZE + localPos.x;
    
    return chunk->cells[index];
}

void GridMap::setCell(Vec2i pos, uint32_t id) noexcept {
    if (!isInBounds(pos)) {
        return;
    }
    
    Vec2i chunkCoord = worldToChunk(pos);
    Chunk* chunk = getOrCreateChunk(chunkCoord);
    
    if (!chunk) {
        return;
    }
    
    Vec2i localPos = worldToLocal(pos);
    size_t index = localPos.y * CHUNK_SIZE + localPos.x;
    
    chunk->cells[index] = id;
    chunk->isDirty = true;
}

void GridMap::clearCell(Vec2i pos) noexcept {
    setCell(pos, INVALID_ID);
}

void GridMap::markChunkDirty(Vec2i chunkCoord) noexcept {
    Chunk* chunk = getChunk(chunkCoord);
    if (chunk) {
        chunk->isDirty = true;
    }
}

bool GridMap::isChunkDirty(Vec2i chunkCoord) const noexcept {
    const Chunk* chunk = getChunk(chunkCoord);
    return chunk ? chunk->isDirty : false;
}

void GridMap::clearDirtyFlag(Vec2i chunkCoord) noexcept {
    Chunk* chunk = getChunk(chunkCoord);
    if (chunk) {
        chunk->isDirty = false;
    }
}

std::vector<Vec2i> GridMap::getDirtyChunks() const noexcept {
    std::vector<Vec2i> dirtyChunks;
    
    for (const auto& [coord, chunk] : chunks) {
        if (chunk.isDirty) {
            dirtyChunks.push_back(coord);
        }
    }
    
    return dirtyChunks;
}

void GridMap::clear() noexcept {
    chunks.clear();
}

GridMap::Chunk* GridMap::getChunk(Vec2i chunkCoord) noexcept {
    auto it = chunks.find(chunkCoord);
    return it != chunks.end() ? &it->second : nullptr;
}

const GridMap::Chunk* GridMap::getChunk(Vec2i chunkCoord) const noexcept {
    auto it = chunks.find(chunkCoord);
    return it != chunks.end() ? &it->second : nullptr;
}

GridMap::Chunk* GridMap::getOrCreateChunk(Vec2i chunkCoord) noexcept {
    auto [it, inserted] = chunks.try_emplace(chunkCoord);
    return &it->second;
}