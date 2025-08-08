#pragma once
#include "../core/Types.h"
#include <functional>
#include <string>

class PlacementManager;
class SelectionManager;

class GatePaletteUI {
public:
    using PlacementCallback = std::function<void(GateType)>;
    using DeleteCallback = std::function<void()>;
    
private:
    PlacementManager* placementManager{nullptr};
    SelectionManager* selectionManager{nullptr};
    
    bool isVisible{true};
    bool isDocked{false};
    float paletteWidth{200.0f};
    
    PlacementCallback onGateSelected;
    DeleteCallback onDeleteSelected;
    
    // UI State
    GateType hoveredGateType{GateType::NOT};
    bool showTooltips{true};
    
public:
    GatePaletteUI() = default;
    ~GatePaletteUI() = default;
    
    void initialize(PlacementManager* pm, SelectionManager* sm) noexcept {
        placementManager = pm;
        selectionManager = sm;
    }
    
    void render() noexcept;
    void renderGatePalette() noexcept;
    void renderSelectionInfo() noexcept;
    void renderPlacementMode() noexcept;
    
    void setVisible(bool visible) noexcept { isVisible = visible; }
    [[nodiscard]] bool getVisible() const noexcept { return isVisible; }
    
    void setPlacementCallback(PlacementCallback cb) noexcept { onGateSelected = cb; }
    void setDeleteCallback(DeleteCallback cb) noexcept { onDeleteSelected = cb; }
    
    void toggleDocking() noexcept { isDocked = !isDocked; }
    
private:
    void renderGateButton(GateType type, const char* label, const char* tooltip) noexcept;
    void renderShortcutHints() noexcept;
    [[nodiscard]] std::string getGateTypeString(GateType type) const noexcept;
};