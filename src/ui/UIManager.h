#pragma once

#include <functional>
#include <string>

namespace notgame {
namespace ui {

enum class Tool {
    SELECT,
    PLACE_GATE,
    DRAW_WIRE,
    ERASE,
    PAN
};

class UIManager {
public:
    UIManager();
    ~UIManager();
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // Frame updates
    void beginFrame();
    void endFrame();
    
    // UI rendering
    void render();
    
    // Tool management
    void setCurrentTool(Tool tool);
    Tool getCurrentTool() const { return currentTool_; }
    
    // Window management
    void showMainMenu(bool show) { showMainMenu_ = show; }
    void showToolPalette(bool show) { showToolPalette_ = show; }
    void showSimulationControls(bool show) { showSimulationControls_ = show; }
    
    // Callbacks
    using MenuCallback = std::function<void(const std::string&)>;
    void setMenuCallback(MenuCallback callback) { menuCallback_ = callback; }
    
    // Input handling
    void handleMouseMove(int x, int y);
    void handleMouseButton(int button, bool pressed);
    void handleKey(int key, bool pressed);
    
private:
    Tool currentTool_;
    
    // Window states
    bool showMainMenu_;
    bool showToolPalette_;
    bool showSimulationControls_;
    bool showDebugInfo_;
    
    // Callbacks
    MenuCallback menuCallback_;
    
    // UI state
    int mouseX_, mouseY_;
    bool mouseButtons_[3];
    
    void renderMainMenu();
    void renderToolPalette();
    void renderSimulationControls();
    void renderDebugInfo();
};

} // namespace ui
} // namespace notgame