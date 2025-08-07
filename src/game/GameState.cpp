#include "game/GameState.h"
#include "core/Circuit.h"
#include "ui/UIManager.h"
#include <iostream>

namespace notgame {
namespace game {

GameState::GameState()
    : currentMode_(GameMode::MENU)
    , paused_(false)
    , simulationSpeed_(1.0f)
    , modeData_(nullptr) {
    // TODO: Initialize circuit when Circuit class is complete
    // circuit_ = std::make_shared<core::Circuit>();
    uiManager_ = std::make_shared<ui::UIManager>();
}

GameState::~GameState() {
    shutdown();
}

bool GameState::initialize() {
    if (!uiManager_->initialize()) {
        std::cerr << "Failed to initialize UI Manager" << std::endl;
        return false;
    }
    
    return true;
}

void GameState::shutdown() {
    if (uiManager_) {
        uiManager_->shutdown();
    }
    
    if (circuit_) {
        circuit_.reset();
    }
    
    modeData_ = nullptr;
}

void GameState::update(float deltaTime) {
    if (paused_) {
        return;
    }
    
    // Update based on current mode
    switch (currentMode_) {
        case GameMode::MENU:
            updateMenu(deltaTime);
            break;
        case GameMode::PUZZLE:
            updatePuzzle(deltaTime);
            break;
        case GameMode::SANDBOX:
            updateSandbox(deltaTime);
            break;
        case GameMode::PAUSE:
            // Paused, no update
            break;
    }
    
    // Update circuit simulation
    if (circuit_ && currentMode_ != GameMode::MENU) {
        // TODO: Update circuit when Circuit class is complete
        // circuit_->update(deltaTime * simulationSpeed_);
    }
}

void GameState::render(render::Renderer& renderer) {
    // Render will be implemented when renderer is ready
}

void GameState::handleInput() {
    // Input handling will be implemented with SDL2
}

void GameState::setMode(GameMode mode) {
    currentMode_ = mode;
    
    // Mode-specific initialization
    switch (mode) {
        case GameMode::MENU:
            uiManager_->showMainMenu(true);
            break;
        case GameMode::PUZZLE:
            uiManager_->showMainMenu(false);
            uiManager_->showToolPalette(true);
            break;
        case GameMode::SANDBOX:
            uiManager_->showMainMenu(false);
            uiManager_->showToolPalette(true);
            uiManager_->showSimulationControls(true);
            break;
        case GameMode::PAUSE:
            break;
    }
}

void GameState::setPaused(bool paused) {
    paused_ = paused;
}

void GameState::setSimulationSpeed(float speed) {
    simulationSpeed_ = speed;
}

bool GameState::loadLevel(const std::string& levelPath) {
    // Level loading will be implemented later
    return false;
}

bool GameState::saveLevel(const std::string& levelPath) {
    // Level saving will be implemented later
    return false;
}

void GameState::updateMenu(float deltaTime) {
    // Menu update logic
}

void GameState::updatePuzzle(float deltaTime) {
    // Puzzle mode update logic
}

void GameState::updateSandbox(float deltaTime) {
    // Sandbox mode update logic
}

} // namespace game
} // namespace notgame