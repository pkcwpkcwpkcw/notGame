#include "PlacementManager.h"
#include "../core/Circuit.h"
#include "../core/GridMap.h"
#include "../core/Grid.h"
#include <SDL.h>

void PlacementManager::enterPlacementMode(GateType type) noexcept {
    currentMode = PlacementMode::PlacingGate;
    selectedGateType = type;
    isPreviewValid = false;
    recentPlacements.clear();
    SDL_Log("[PlacementManager] Entered placement mode for gate type: %d", (int)type);
}

void PlacementManager::exitPlacementMode() noexcept {
    SDL_Log("[PlacementManager] Exiting placement mode");
    currentMode = PlacementMode::None;
    isPreviewValid = false;
    continuousPlacement = false;
    SDL_Log("[PlacementManager] Mode changed to: None");
}

void PlacementManager::updatePreview(Vec2i gridPos) noexcept {
    previewPosition = gridPos;
    isPreviewValid = validatePosition(gridPos);
}

bool PlacementManager::validatePosition(Vec2i gridPos) const noexcept {
    if (!gridMap || !circuit) {
        return false;
    }
    
    if (!gridMap->isInBounds(gridPos)) {
        return false;
    }
    
    if (gridMap->isOccupied(gridPos)) {
        return false;
    }
    
    if (hasWireConflict(gridPos)) {
        return false;
    }
    
    return true;
}

Result<GateId> PlacementManager::placeGate(Vec2i gridPos) noexcept {
    if (!validatePosition(gridPos)) {
        return Result<GateId>{Constants::INVALID_GATE_ID, ErrorCode::INVALID_POSITION};
    }
    
    if (!circuit || !gridMap) {
        return Result<GateId>{Constants::INVALID_GATE_ID, ErrorCode::NOT_INITIALIZED};
    }
    
    Vec2 worldPos = Vec2(static_cast<float>(gridPos.x), static_cast<float>(gridPos.y));
    
    auto result = circuit->addGate(worldPos);
    
    if (result.success()) {
        GateId gateId = result.value;
        SDL_Log("[PlacementManager] Storing gate %u at grid position (%d, %d) in GridMap", gateId, gridPos.x, gridPos.y);
        gridMap->setCell(gridPos, static_cast<uint32_t>(gateId));
        
        // Verify it was stored
        uint32_t storedId = gridMap->getCell(gridPos);
        SDL_Log("[PlacementManager] Verification: GridMap cell at (%d, %d) now contains ID: %u", gridPos.x, gridPos.y, storedId);
        
        recentPlacements.push_back(gridPos);
        
        SDL_Log("[PlacementManager] Gate placed. Continuous placement: %s", continuousPlacement ? "YES" : "NO");
        if (!continuousPlacement) {
            SDL_Log("[PlacementManager] Single placement mode - exiting placement mode");
            exitPlacementMode();
        } else {
            SDL_Log("[PlacementManager] Continuous placement mode - staying in placement mode");
        }
        
        return result;
    }
    
    return result;
}

void PlacementManager::cancelPlacement() noexcept {
    exitPlacementMode();
}

void PlacementManager::onMouseMove(Vec2 worldPos) noexcept {
    if (!grid || currentMode != PlacementMode::PlacingGate) {
        return;
    }
    
    // worldPos is already in grid coordinates from Camera::ScreenToWorld
    Vec2 snapped = grid->snapToGrid(worldPos);
    Vec2i gridPos(static_cast<int>(snapped.x), static_cast<int>(snapped.y));
    
    updatePreview(gridPos);
}

void PlacementManager::onMouseClick(MouseButton btn, Vec2 worldPos) noexcept {
    SDL_Log("[PlacementManager] Mouse click - button: %d, world pos: (%.2f, %.2f), mode: %d", 
            (int)btn, worldPos.x, worldPos.y, (int)currentMode);
    
    if (!grid) {
        SDL_Log("[PlacementManager] ERROR: Grid is null!");
        return;
    }
    
    if (btn == MouseButton::Left && currentMode == PlacementMode::PlacingGate) {
        // worldPos is already in grid coordinates from Camera::ScreenToWorld
        Vec2 snapped = grid->snapToGrid(worldPos);
        Vec2i gridPos(static_cast<int>(snapped.x), static_cast<int>(snapped.y));
        
        SDL_Log("[PlacementManager] Attempting to place gate at grid pos: (%d, %d)", gridPos.x, gridPos.y);
        auto result = placeGate(gridPos);
        if (result.success()) {
            SDL_Log("[PlacementManager] Gate placed successfully with ID: %u", result.value);
        } else {
            SDL_Log("[PlacementManager] Gate placement failed with error: %d", (int)result.error);
        }
    } else if (btn == MouseButton::Right) {
        if (currentMode != PlacementMode::None) {
            cancelPlacement();
        }
    }
}

void PlacementManager::onKeyPress(Key key) noexcept {
    SDL_Log("[PlacementManager] onKeyPress called with key: %d, current mode: %d", (int)key, (int)currentMode);
    
    switch (key) {
        case Key::N:
            if (currentMode == PlacementMode::None) {
                enterPlacementMode(GateType::NOT);
            }
            break;
            
        case Key::Escape:
            SDL_Log("[PlacementManager] ESC key in onKeyPress, mode: %d", (int)currentMode);
            if (currentMode != PlacementMode::None) {
                SDL_Log("[PlacementManager] Calling cancelPlacement()");
                cancelPlacement();
            }
            break;
            
        case Key::LeftShift:
        case Key::RightShift:
            if (!continuousPlacement) {  // Only set if not already enabled
                SDL_Log("[PlacementManager] Shift key pressed - setting continuous placement to TRUE");
                setContinuousPlacement(true);
                SDL_Log("[PlacementManager] Continuous placement mode enabled (flag = %s)", continuousPlacement ? "true" : "false");
            }
            break;
            
        default:
            break;
    }
}

void PlacementManager::onKeyRelease(Key key) noexcept {
    SDL_Log("[PlacementManager] onKeyRelease called with key: %d", (int)key);
    
    switch (key) {
        case Key::LeftShift:
        case Key::RightShift:
            SDL_Log("[PlacementManager] Shift key released - setting continuous placement to FALSE");
            setContinuousPlacement(false);
            SDL_Log("[PlacementManager] Continuous placement mode disabled (flag = %s)", continuousPlacement ? "true" : "false");
            break;
            
        default:
            break;
    }
}

bool PlacementManager::canPlaceAt(Vec2i pos) const noexcept {
    return validatePosition(pos);
}

bool PlacementManager::hasWireConflict(Vec2i pos) const noexcept {
    return false;
}