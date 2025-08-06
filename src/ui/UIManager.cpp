#include "ui/UIManager.h"
#include <iostream>

namespace notgame {
namespace ui {

UIManager::UIManager()
    : currentTool_(Tool::SELECT)
    , showMainMenu_(true)
    , showToolPalette_(false)
    , showSimulationControls_(false)
    , showDebugInfo_(false)
    , mouseX_(0)
    , mouseY_(0) {
    for (auto& button : mouseButtons_) {
        button = false;
    }
}

UIManager::~UIManager() {
}

bool UIManager::initialize() {
    // ImGui initialization will be done when SDL2 is available
    return true;
}

void UIManager::shutdown() {
    // Cleanup will be done when ImGui is integrated
}

void UIManager::beginFrame() {
    // ImGui frame start
}

void UIManager::endFrame() {
    // ImGui frame end
}

void UIManager::render() {
    if (showMainMenu_) {
        renderMainMenu();
    }
    
    if (showToolPalette_) {
        renderToolPalette();
    }
    
    if (showSimulationControls_) {
        renderSimulationControls();
    }
    
    if (showDebugInfo_) {
        renderDebugInfo();
    }
}

void UIManager::setCurrentTool(Tool tool) {
    currentTool_ = tool;
}

void UIManager::handleMouseMove(int x, int y) {
    mouseX_ = x;
    mouseY_ = y;
}

void UIManager::handleMouseButton(int button, bool pressed) {
    if (button >= 0 && button < 3) {
        mouseButtons_[button] = pressed;
    }
}

void UIManager::handleKey(int key, bool pressed) {
    // Keyboard handling
}

void UIManager::renderMainMenu() {
    // Main menu rendering (ImGui implementation pending)
}

void UIManager::renderToolPalette() {
    // Tool palette rendering (ImGui implementation pending)
}

void UIManager::renderSimulationControls() {
    // Simulation controls rendering (ImGui implementation pending)
}

void UIManager::renderDebugInfo() {
    // Debug info rendering (ImGui implementation pending)
}

} // namespace ui
} // namespace notgame