#pragma once
#include "../core/Types.h"
#include "../core/Vec2.h"
#include <unordered_set>
#include <vector>

class Circuit;
class GridMap;
class Grid;

class SelectionManager {
private:
    std::unordered_set<GateId> selectedGates;
    GateId lastSelectedGate{Constants::INVALID_GATE_ID};
    Vec2i selectionStart{0, 0};
    bool isRangeSelecting{false};
    
    Circuit* circuit{nullptr};
    GridMap* gridMap{nullptr};
    Grid* grid{nullptr};
    
public:
    SelectionManager() = default;
    ~SelectionManager() = default;
    
    void initialize(Circuit* circ, GridMap* gMap, Grid* g) noexcept {
        circuit = circ;
        gridMap = gMap;
        grid = g;
    }
    
    void selectGate(GateId gateId) noexcept;
    void deselectGate(GateId gateId) noexcept;
    void clearSelection() noexcept;
    void toggleSelection(GateId gateId) noexcept;
    
    void startRangeSelection(Vec2i start) noexcept;
    void updateRangeSelection(Vec2i current) noexcept;
    void endRangeSelection() noexcept;
    
    [[nodiscard]] bool isSelected(GateId gateId) const noexcept {
        return selectedGates.find(gateId) != selectedGates.end();
    }
    
    [[nodiscard]] const std::unordered_set<GateId>& getSelection() const noexcept {
        return selectedGates;
    }
    
    [[nodiscard]] size_t getSelectionCount() const noexcept {
        return selectedGates.size();
    }
    
    [[nodiscard]] GateId getLastSelected() const noexcept {
        return lastSelectedGate;
    }
    
    [[nodiscard]] bool hasSelection() const noexcept {
        return !selectedGates.empty();
    }
    
    void deleteSelected() noexcept;
    void moveSelected(Vec2i delta) noexcept;
    
    [[nodiscard]] GateId getGateAt(Vec2i gridPos) const noexcept;
    
    void onMouseClick(MouseButton btn, Vec2 screenPos, bool ctrlHeld, bool shiftHeld) noexcept;
    void onKeyPress(Key key) noexcept;
    
private:
    void updateGateSelectionState(GateId gateId, bool selected) noexcept;
    std::vector<GateId> getGatesInRect(Vec2i start, Vec2i end) const noexcept;
};