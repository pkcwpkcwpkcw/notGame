#include "SelectionManager.h"
#include "../core/Circuit.h"
#include "../core/GridMap.h"
#include "../core/Grid.h"
#include "../core/Gate.h"
#include <algorithm>
#include <SDL.h>

void SelectionManager::selectGate(GateId gateId) noexcept {
    if (gateId == Constants::INVALID_GATE_ID) {
        SDL_Log("[SelectionManager] Cannot select invalid gate");
        return;
    }
    
    SDL_Log("[SelectionManager] Selecting gate %u", gateId);
    selectedGates.insert(gateId);
    lastSelectedGate = gateId;
    updateGateSelectionState(gateId, true);
}

void SelectionManager::deselectGate(GateId gateId) noexcept {
    selectedGates.erase(gateId);
    
    if (lastSelectedGate == gateId) {
        lastSelectedGate = selectedGates.empty() ? 
            Constants::INVALID_GATE_ID : *selectedGates.begin();
    }
    
    updateGateSelectionState(gateId, false);
}

void SelectionManager::clearSelection() noexcept {
    for (GateId id : selectedGates) {
        updateGateSelectionState(id, false);
    }
    
    selectedGates.clear();
    lastSelectedGate = Constants::INVALID_GATE_ID;
}

void SelectionManager::toggleSelection(GateId gateId) noexcept {
    if (isSelected(gateId)) {
        deselectGate(gateId);
    } else {
        selectGate(gateId);
    }
}

void SelectionManager::startRangeSelection(Vec2i start) noexcept {
    selectionStart = start;
    isRangeSelecting = true;
}

void SelectionManager::updateRangeSelection(Vec2i current) noexcept {
    if (!isRangeSelecting) {
        return;
    }
    
    clearSelection();
    
    auto gatesInRect = getGatesInRect(selectionStart, current);
    for (GateId id : gatesInRect) {
        selectGate(id);
    }
}

void SelectionManager::endRangeSelection() noexcept {
    isRangeSelecting = false;
}

void SelectionManager::deleteSelected() noexcept {
    if (!circuit) {
        return;
    }
    
    std::vector<GateId> toDelete(selectedGates.begin(), selectedGates.end());
    
    for (GateId id : toDelete) {
        Gate* gate = circuit->getGate(id);
        if (gate) {
            Vec2i gridPos(static_cast<int>(gate->position.x), 
                         static_cast<int>(gate->position.y));
            
            if (gridMap) {
                gridMap->clearCell(gridPos);
            }
            
            circuit->removeGate(id);
        }
    }
    
    clearSelection();
}

void SelectionManager::moveSelected(Vec2i delta) noexcept {
    if (!circuit || !gridMap) {
        return;
    }
    
    std::vector<std::pair<GateId, Vec2i>> moves;
    
    for (GateId id : selectedGates) {
        Gate* gate = circuit->getGate(id);
        if (gate) {
            Vec2i currentPos(static_cast<int>(gate->position.x), 
                           static_cast<int>(gate->position.y));
            Vec2i newPos = currentPos + delta;
            
            if (gridMap->isInBounds(newPos) && !gridMap->isOccupied(newPos)) {
                moves.push_back({id, newPos});
            }
        }
    }
    
    for (const auto& [id, newPos] : moves) {
        Gate* gate = circuit->getGate(id);
        if (gate) {
            Vec2i oldPos(static_cast<int>(gate->position.x), 
                        static_cast<int>(gate->position.y));
            
            gridMap->clearCell(oldPos);
            gridMap->setCell(newPos, static_cast<uint32_t>(id));
            
            gate->position = Vec2(static_cast<float>(newPos.x), 
                                 static_cast<float>(newPos.y));
        }
    }
}

GateId SelectionManager::getGateAt(Vec2i gridPos) const noexcept {
    if (!gridMap) {
        SDL_Log("[SelectionManager] GridMap is null in getGateAt");
        return Constants::INVALID_GATE_ID;
    }
    
    uint32_t id = gridMap->getCell(gridPos);
    SDL_Log("[SelectionManager] GridMap cell at (%d, %d) contains ID: %u", gridPos.x, gridPos.y, id);
    return id != 0 ? static_cast<GateId>(id) : Constants::INVALID_GATE_ID;
}

void SelectionManager::onMouseClick(MouseButton btn, Vec2 worldPos, 
                                   bool ctrlHeld, bool shiftHeld) noexcept {
    if (!grid || !circuit) {
        SDL_Log("[SelectionManager] ERROR: grid or circuit is null");
        return;
    }
    
    if (btn == MouseButton::Left) {
        // worldPos is already in world/grid coordinates from Camera::ScreenToWorld
        Vec2 snapped = grid->snapToGrid(worldPos);
        Vec2i gridPos(static_cast<int>(snapped.x), static_cast<int>(snapped.y));
        
        SDL_Log("[SelectionManager] Mouse click at world pos (%.2f, %.2f), grid pos (%d, %d)", 
                worldPos.x, worldPos.y, gridPos.x, gridPos.y);
        
        GateId gateId = getGateAt(gridPos);
        SDL_Log("[SelectionManager] Gate at position: %u", gateId);
        
        if (gateId != Constants::INVALID_GATE_ID) {
            if (ctrlHeld) {
                toggleSelection(gateId);
            } else if (shiftHeld && lastSelectedGate != Constants::INVALID_GATE_ID) {
                // TODO: Implement path selection
                selectGate(gateId);
            } else {
                clearSelection();
                selectGate(gateId);
            }
        } else {
            if (!ctrlHeld && !shiftHeld) {
                SDL_Log("[SelectionManager] No gate at position, clearing selection");
                clearSelection();
            }
        }
    }
}

void SelectionManager::onKeyPress(Key key) noexcept {
    switch (key) {
        case Key::Delete:
        case Key::Backspace:
            if (hasSelection()) {
                deleteSelected();
            }
            break;
            
        case Key::A:
            // TODO: Check if Ctrl is held for Select All
            break;
            
        case Key::Escape:
            clearSelection();
            break;
            
        default:
            break;
    }
}

void SelectionManager::updateGateSelectionState(GateId gateId, bool selected) noexcept {
    if (!circuit) {
        SDL_Log("[SelectionManager] ERROR: circuit is null in updateGateSelectionState");
        return;
    }
    
    Gate* gate = circuit->getGate(gateId);
    if (gate) {
        gate->isSelected = selected;
        SDL_Log("[SelectionManager] Gate %u selection state updated to %s", 
                gateId, selected ? "SELECTED" : "DESELECTED");
    } else {
        SDL_Log("[SelectionManager] ERROR: Gate %u not found in circuit", gateId);
    }
}

std::vector<GateId> SelectionManager::getGatesInRect(Vec2i start, Vec2i end) const noexcept {
    std::vector<GateId> gatesInRect;
    
    if (!circuit) {
        return gatesInRect;
    }
    
    int minX = std::min(start.x, end.x);
    int maxX = std::max(start.x, end.x);
    int minY = std::min(start.y, end.y);
    int maxY = std::max(start.y, end.y);
    
    for (auto it = circuit->gatesBegin(); it != circuit->gatesEnd(); ++it) {
        const Gate& gate = it->second;
        int x = static_cast<int>(gate.position.x);
        int y = static_cast<int>(gate.position.y);
        
        if (x >= minX && x <= maxX && y >= minY && y <= maxY) {
            gatesInRect.push_back(gate.id);
        }
    }
    
    return gatesInRect;
}