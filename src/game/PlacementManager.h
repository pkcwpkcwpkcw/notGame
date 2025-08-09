#pragma once
#include "../core/Types.h"
#include "../core/Vec2.h"
#include <vector>

class Circuit;
class GridMap;
class Grid;
class CellWireManager;

class PlacementManager {
public:
    enum class PlacementMode {
        None,
        PlacingGate,
        PlacingWire
    };
    
private:
    PlacementMode currentMode{PlacementMode::None};
    GateType selectedGateType{GateType::NOT};
    Vec2i previewPosition{0, 0};
    bool isPreviewValid{false};
    bool continuousPlacement{false};
    
    Circuit* circuit{nullptr};
    GridMap* gridMap{nullptr};
    Grid* grid{nullptr};
    CellWireManager* cellWireManager{nullptr};
    
    std::vector<Vec2i> recentPlacements;
    
public:
    PlacementManager() = default;
    ~PlacementManager() = default;
    
    void initialize(Circuit* circ, GridMap* gMap, Grid* g, CellWireManager* cwm = nullptr) noexcept {
        circuit = circ;
        gridMap = gMap;
        grid = g;
        cellWireManager = cwm;
    }
    
    void enterPlacementMode(GateType type) noexcept;
    void exitPlacementMode() noexcept;
    [[nodiscard]] bool isInPlacementMode() const noexcept { 
        return currentMode != PlacementMode::None; 
    }
    
    void updatePreview(Vec2i gridPos) noexcept;
    [[nodiscard]] bool validatePosition(Vec2i gridPos) const noexcept;
    
    [[nodiscard]] Result<GateId> placeGate(Vec2i gridPos) noexcept;
    void cancelPlacement() noexcept;
    
    void setContinuousPlacement(bool continuous) noexcept { 
        continuousPlacement = continuous; 
    }
    [[nodiscard]] bool isContinuousPlacement() const noexcept { 
        return continuousPlacement; 
    }
    
    [[nodiscard]] PlacementMode getCurrentMode() const noexcept { return currentMode; }
    [[nodiscard]] GateType getSelectedGateType() const noexcept { return selectedGateType; }
    [[nodiscard]] Vec2i getPreviewPosition() const noexcept { return previewPosition; }
    [[nodiscard]] bool isPreviewPositionValid() const noexcept { return isPreviewValid; }
    
    void onMouseMove(Vec2 worldPos) noexcept;
    void onMouseClick(MouseButton btn, Vec2 worldPos) noexcept;
    void onKeyPress(Key key) noexcept;
    void onKeyRelease(Key key) noexcept;
    
private:
    [[nodiscard]] bool canPlaceAt(Vec2i pos) const noexcept;
    [[nodiscard]] bool hasWireConflict(Vec2i pos) const noexcept;
};