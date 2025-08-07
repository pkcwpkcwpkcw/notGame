#pragma once
#include "Vec2.h"
#include "Types.h"
#include <algorithm>

class Grid {
private:
    float cellSize;
    Vec2 offset;
    float zoom;
    
    bool hasLimits;
    Vec2 minBounds;
    Vec2 maxBounds;
    
public:
    explicit Grid(float cellSize_ = Constants::GRID_CELL_SIZE) noexcept
        : cellSize(cellSize_)
        , offset(0, 0)
        , zoom(1.0f)
        , hasLimits(false)
        , minBounds(-1000, -1000)
        , maxBounds(1000, 1000) {}
    
    [[nodiscard]] Vec2 screenToGrid(Vec2 screenPos) const noexcept {
        Vec2 worldPos = (screenPos - offset) / zoom;
        return worldPos / cellSize;
    }
    
    [[nodiscard]] Vec2 gridToScreen(Vec2 gridPos) const noexcept {
        Vec2 worldPos = gridPos * cellSize;
        return worldPos * zoom + offset;
    }
    
    [[nodiscard]] Vec2 snapToGrid(Vec2 pos) const noexcept {
        return Vec2(
            std::round(pos.x),
            std::round(pos.y)
        );
    }
    
    void pan(Vec2 delta) noexcept {
        offset += delta;
        if (hasLimits) {
            clampOffset();
        }
    }
    
    void zoomAt(Vec2 screenPos, float factor) noexcept {
        factor = std::clamp(factor, 0.1f, 10.0f);
        
        Vec2 gridPos = screenToGrid(screenPos);
        zoom *= factor;
        zoom = std::clamp(zoom, 0.25f, 4.0f);
        
        Vec2 newScreenPos = gridToScreen(gridPos);
        offset += screenPos - newScreenPos;
    }
    
    [[nodiscard]] bool isValidPosition(Vec2 gridPos) const noexcept {
        if (!hasLimits) return true;
        
        return gridPos.x >= minBounds.x && gridPos.x <= maxBounds.x &&
               gridPos.y >= minBounds.y && gridPos.y <= maxBounds.y;
    }
    
    void setLimits(Vec2 min, Vec2 max) noexcept {
        hasLimits = true;
        minBounds = min;
        maxBounds = max;
    }
    
    void removeLimits() noexcept {
        hasLimits = false;
    }
    
    [[nodiscard]] float getCellSize() const noexcept { return cellSize; }
    [[nodiscard]] float getZoom() const noexcept { return zoom; }
    [[nodiscard]] Vec2 getOffset() const noexcept { return offset; }
    
private:
    void clampOffset() noexcept {
        Vec2 minScreen = gridToScreen(minBounds);
        Vec2 maxScreen = gridToScreen(maxBounds);
        
        offset.x = std::clamp(offset.x, minScreen.x, maxScreen.x);
        offset.y = std::clamp(offset.y, minScreen.y, maxScreen.y);
    }
};