#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <SDL.h>
#include <glm/glm.hpp>
#include <vector>

class Camera;
class GridRenderer;

class InputHandler {
public:
    InputHandler(Camera& camera, GridRenderer& gridRenderer);
    
    void HandleEvent(const SDL_Event& event);
    
    void OnMouseMove(int x, int y);
    void OnMouseDown(int button, int x, int y);
    void OnMouseUp(int button, int x, int y);
    void OnMouseWheel(float delta);
    
    void OnKeyDown(SDL_Keycode key);
    void OnKeyUp(SDL_Keycode key);
    
    void Update(float deltaTime);
    
private:
    Camera& m_camera;
    GridRenderer& m_gridRenderer;
    
    bool m_isPanning;
    glm::vec2 m_lastMousePos;
    glm::vec2 m_panStartPos;
    
    bool m_isSelecting;
    glm::ivec2 m_selectionStart;
    std::vector<glm::ivec2> m_selectedCells;
    
    bool m_ctrlPressed;
    bool m_shiftPressed;
    
    bool m_keyUp;
    bool m_keyDown;
    bool m_keyLeft;
    bool m_keyRight;
    
    void UpdateSelection(const glm::ivec2& currentCell);
};

#endif // INPUT_HANDLER_H