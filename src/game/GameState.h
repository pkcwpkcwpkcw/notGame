#pragma once

#include <memory>
#include <string>

namespace notgame {

namespace core {
    class Circuit;
}

namespace render {
    class Renderer;
}

namespace ui {
    class UIManager;
}

namespace game {

enum class GameMode {
    MENU,
    PUZZLE,
    SANDBOX,
    PAUSE
};

class GameState {
public:
    GameState();
    ~GameState();
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // Game loop
    void update(float deltaTime);
    void render(render::Renderer& renderer);
    void handleInput();
    
    // Mode management
    void setMode(GameMode mode);
    GameMode getMode() const { return currentMode_; }
    
    // Circuit access
    std::shared_ptr<core::Circuit> getCircuit() const { return circuit_; }
    
    // Level management
    bool loadLevel(const std::string& levelPath);
    bool saveLevel(const std::string& levelPath);
    
    // Game state
    bool isPaused() const { return paused_; }
    void setPaused(bool paused);
    
    float getSimulationSpeed() const { return simulationSpeed_; }
    void setSimulationSpeed(float speed);
    
private:
    GameMode currentMode_;
    bool paused_;
    float simulationSpeed_;
    
    std::shared_ptr<core::Circuit> circuit_;
    std::shared_ptr<ui::UIManager> uiManager_;
    
    // Mode-specific data
    void* modeData_;  // Will be PuzzleMode* or SandboxMode*
    
    void updateMenu(float deltaTime);
    void updatePuzzle(float deltaTime);
    void updateSandbox(float deltaTime);
};

} // namespace game
} // namespace notgame