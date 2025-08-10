#pragma once

#include <SDL.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include "../core/Vec2.h"

class EventSystem;
class Timer;
class ImGuiManager;
class GridRenderer;
class Camera;
class InputHandler;
class Circuit;
class RenderManager;
class Window;
class PlacementManager;
class SelectionManager;
class GridMap;
class GatePaletteUI;
class WireManager;
class CellWireManager;

namespace simulation {
    class CircuitSimulator;
}

namespace Input {
    class InputManager;
}

enum class AppState {
    INITIALIZING,
    MENU,
    PLAYING,
    PAUSED,
    EDITOR,
    SHUTTING_DOWN
};

struct AppConfig {
    int windowWidth = 1280;
    int windowHeight = 720;
    std::string windowTitle = "NOT Gate Game";
    bool fullscreen = false;
    bool vsync = true;
    int targetFPS = 60;
    int glMajorVersion = 3;
    int glMinorVersion = 3;
};

class Application {
public:
    Application();
    ~Application();
    
    bool initialize(const AppConfig& config);
    void run();
    void shutdown();
    
    void setState(AppState newState);
    AppState getState() const { return m_currentState; }
    
    bool isRunning() const { return m_running; }
    void quit() { m_running = false; }
    
    SDL_Window* getWindow() const { return m_window; }
    SDL_GLContext getGLContext() const { return m_glContext; }
    
private:
    bool initializeSDL();
    bool createWindow(const AppConfig& config);
    bool createGLContext(const AppConfig& config);
    bool initializeGLEW();
    bool initializeImGui();
    bool initializeRenderers();
    void createDemoCircuit();
    
    void handleEvents();
    void update(float deltaTime);
    void render();
    void regulateFrameRate();
    
    void cleanupImGui();
    void cleanupGL();
    void cleanupSDL();
    
private:
    SDL_Window* m_window;
    SDL_GLContext m_glContext;
    
    AppConfig m_config;
    
    bool m_running;
    AppState m_currentState;
    
    std::unique_ptr<EventSystem> m_eventSystem;
    std::unique_ptr<Timer> m_timer;
    std::unique_ptr<ImGuiManager> m_imguiManager;
    
    std::unique_ptr<GridRenderer> m_gridRenderer;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<InputHandler> m_inputHandler;
    std::unique_ptr<Input::InputManager> m_inputManager;
    
    std::unique_ptr<Circuit> m_circuit;
    std::unique_ptr<RenderManager> m_renderManager;
    std::unique_ptr<Window> m_renderWindow;
    
    std::unique_ptr<PlacementManager> m_placementManager;
    std::unique_ptr<SelectionManager> m_selectionManager;
    std::unique_ptr<GridMap> m_gridMap;
    std::unique_ptr<GatePaletteUI> m_gatePaletteUI;
    std::unique_ptr<WireManager> m_wireManager;
    std::unique_ptr<CellWireManager> m_cellWireManager;
    std::unique_ptr<simulation::CircuitSimulator> m_circuitSimulator;
    
    uint32_t m_frameCount;
    float m_fpsUpdateTimer;
    float m_currentFPS;
    
    // Context menu state
    bool m_showContextMenu{false};
    glm::vec2 m_contextMenuPos{0, 0};
    Vec2i m_contextMenuGridPos{0, 0};
};