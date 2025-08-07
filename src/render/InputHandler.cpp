#include "InputHandler.h"
#include "Camera.h"
#include "GridRenderer.h"
#include <algorithm>
#include <climits>

InputHandler::InputHandler(Camera& camera, GridRenderer& gridRenderer)
    : m_camera(camera)
    , m_gridRenderer(gridRenderer)
    , m_isPanning(false)
    , m_lastMousePos(0, 0)
    , m_panStartPos(0, 0)
    , m_isSelecting(false)
    , m_selectionStart(-1, -1)
    , m_ctrlPressed(false)
    , m_shiftPressed(false)
    , m_keyUp(false)
    , m_keyDown(false)
    , m_keyLeft(false)
    , m_keyRight(false) {
}

void InputHandler::HandleEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_MOUSEMOTION:
            OnMouseMove(event.motion.x, event.motion.y);
            break;
            
        case SDL_MOUSEBUTTONDOWN:
            OnMouseDown(event.button.button, event.button.x, event.button.y);
            break;
            
        case SDL_MOUSEBUTTONUP:
            OnMouseUp(event.button.button, event.button.x, event.button.y);
            break;
            
        case SDL_MOUSEWHEEL:
            OnMouseWheel(event.wheel.y);
            break;
            
        case SDL_KEYDOWN:
            if (event.key.repeat == 0) {
                OnKeyDown(event.key.keysym.sym);
            }
            break;
            
        case SDL_KEYUP:
            OnKeyUp(event.key.keysym.sym);
            break;
            
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                m_camera.SetScreenSize(event.window.data1, event.window.data2);
                m_gridRenderer.OnResize(event.window.data1, event.window.data2);
            }
            break;
    }
}

void InputHandler::OnMouseMove(int x, int y) {
    glm::vec2 mousePos(x, y);
    
    glm::ivec2 gridPos = m_camera.ScreenToGrid(mousePos);
    
    // 그리드 경계 체크
    if (!m_camera.IsGridUnlimited()) {
        glm::ivec2 minBounds = m_camera.GetMinGridBounds();
        glm::ivec2 maxBounds = m_camera.GetMaxGridBounds();
        
        // Debug log
        static int logCounter = 0;
        if (logCounter++ % 30 == 0) {  // Log every 30th frame to avoid spam
            SDL_Log("Grid pos: (%d, %d), Bounds: min(%d, %d), max(%d, %d)", 
                    gridPos.x, gridPos.y, 
                    minBounds.x, minBounds.y, 
                    maxBounds.x, maxBounds.y);
        }
        
        // 경계 밖이면 호버 셀 없음으로 설정
        if (gridPos.x < minBounds.x || gridPos.x > maxBounds.x ||
            gridPos.y < minBounds.y || gridPos.y > maxBounds.y) {
            m_gridRenderer.SetHoveredCell(glm::ivec2(INT_MIN, INT_MIN));
        } else {
            m_gridRenderer.SetHoveredCell(gridPos);
        }
    } else {
        m_gridRenderer.SetHoveredCell(gridPos);
    }
    
    if (m_isPanning) {
        glm::vec2 delta = mousePos - m_lastMousePos;
        m_camera.Pan(delta);
        // Debug log
        SDL_Log("Panning: delta(%.2f, %.2f), camera pos(%.2f, %.2f)", 
                delta.x, delta.y, m_camera.GetPosition().x, m_camera.GetPosition().y);
    }
    
    if (m_isSelecting) {
        UpdateSelection(gridPos);
    }
    
    m_lastMousePos = mousePos;
}

void InputHandler::OnMouseDown(int button, int x, int y) {
    glm::vec2 mousePos(x, y);
    glm::ivec2 gridPos = m_camera.ScreenToGrid(mousePos);
    
    SDL_Log("Mouse down: button=%d, pos(%d, %d)", button, x, y);
    
    if (button == SDL_BUTTON_MIDDLE || (button == SDL_BUTTON_RIGHT && !m_isSelecting)) {
        m_isPanning = true;
        m_panStartPos = mousePos;
        SDL_Log("Panning started");
    } else if (button == SDL_BUTTON_LEFT) {
        // 그리드 경계 체크
        bool inBounds = true;
        if (!m_camera.IsGridUnlimited()) {
            glm::ivec2 minBounds = m_camera.GetMinGridBounds();
            glm::ivec2 maxBounds = m_camera.GetMaxGridBounds();
            
            if (gridPos.x < minBounds.x || gridPos.x > maxBounds.x ||
                gridPos.y < minBounds.y || gridPos.y > maxBounds.y) {
                inBounds = false;
            }
        }
        
        // 경계 내에 있을 때만 선택 시작
        if (inBounds) {
            m_isSelecting = true;
            m_selectionStart = gridPos;
            
            if (!m_ctrlPressed) {
                m_gridRenderer.ClearSelection();
            }
            
            m_selectedCells.clear();
            m_selectedCells.push_back(gridPos);
            m_gridRenderer.SetSelectedCells(m_selectedCells);
        }
    }
}

void InputHandler::OnMouseUp(int button, int x, int y) {
    if (button == SDL_BUTTON_MIDDLE || button == SDL_BUTTON_RIGHT) {
        m_isPanning = false;
    } else if (button == SDL_BUTTON_LEFT) {
        m_isSelecting = false;
    }
}

void InputHandler::OnMouseWheel(float delta) {
    float zoomFactor = delta > 0 ? 1.1f : 0.9f;
    m_camera.Zoom(zoomFactor, m_lastMousePos);
}

void InputHandler::OnKeyDown(SDL_Keycode key) {
    switch (key) {
        case SDLK_LCTRL:
        case SDLK_RCTRL:
            m_ctrlPressed = true;
            break;
            
        case SDLK_LSHIFT:
        case SDLK_RSHIFT:
            m_shiftPressed = true;
            break;
            
        case SDLK_HOME:
            m_camera.Reset();
            break;
            
        case SDLK_ESCAPE:
            m_gridRenderer.ClearSelection();
            break;
            
        case SDLK_UP:
        case SDLK_w:
            m_keyUp = true;
            break;
            
        case SDLK_DOWN:
        case SDLK_s:
            m_keyDown = true;
            break;
            
        case SDLK_LEFT:
        case SDLK_a:
            m_keyLeft = true;
            break;
            
        case SDLK_RIGHT:
        case SDLK_d:
            m_keyRight = true;
            break;
            
        case SDLK_EQUALS:
        case SDLK_KP_PLUS:
            if (m_ctrlPressed) {
                m_camera.Zoom(1.2f, m_camera.GetScreenSize() * 0.5f);
            }
            break;
            
        case SDLK_MINUS:
        case SDLK_KP_MINUS:
            if (m_ctrlPressed) {
                m_camera.Zoom(0.8f, m_camera.GetScreenSize() * 0.5f);
            }
            break;
            
        case SDLK_0:
            if (m_ctrlPressed) {
                m_camera.SetZoom(1.0f);
            }
            break;
            
        case SDLK_g:
            m_gridRenderer.SetGridVisible(!m_gridRenderer.IsGridVisible());
            break;
    }
}

void InputHandler::OnKeyUp(SDL_Keycode key) {
    switch (key) {
        case SDLK_LCTRL:
        case SDLK_RCTRL:
            m_ctrlPressed = false;
            break;
            
        case SDLK_LSHIFT:
        case SDLK_RSHIFT:
            m_shiftPressed = false;
            break;
            
        case SDLK_UP:
        case SDLK_w:
            m_keyUp = false;
            break;
            
        case SDLK_DOWN:
        case SDLK_s:
            m_keyDown = false;
            break;
            
        case SDLK_LEFT:
        case SDLK_a:
            m_keyLeft = false;
            break;
            
        case SDLK_RIGHT:
        case SDLK_d:
            m_keyRight = false;
            break;
    }
}

void InputHandler::Update(float deltaTime) {
    const float panSpeed = 500.0f * deltaTime / m_camera.GetZoom();
    glm::vec2 panDelta(0, 0);
    
    // 키보드 입력은 스크린 좌표계 기준
    // 위/아래는 스크린 Y축 기준 (아래가 양수)
    if (m_keyUp) panDelta.y -= panSpeed;    // 위 키 -> 화면을 아래로 끌기 (위 내용 보기)
    if (m_keyDown) panDelta.y += panSpeed;  // 아래 키 -> 화면을 위로 끌기 (아래 내용 보기)
    if (m_keyLeft) panDelta.x -= panSpeed;  // 왼쪽 키 -> 화면을 오른쪽으로 끌기 (왼쪽 내용 보기)
    if (m_keyRight) panDelta.x += panSpeed; // 오른쪽 키 -> 화면을 왼쪽으로 끌기 (오른쪽 내용 보기)
    
    if (panDelta.x != 0 || panDelta.y != 0) {
        m_camera.Pan(panDelta);
    }
}

void InputHandler::UpdateSelection(const glm::ivec2& currentCell) {
    m_selectedCells.clear();
    
    int minX = std::min(m_selectionStart.x, currentCell.x);
    int maxX = std::max(m_selectionStart.x, currentCell.x);
    int minY = std::min(m_selectionStart.y, currentCell.y);
    int maxY = std::max(m_selectionStart.y, currentCell.y);
    
    // 그리드 경계로 제한
    if (!m_camera.IsGridUnlimited()) {
        glm::ivec2 minBounds = m_camera.GetMinGridBounds();
        glm::ivec2 maxBounds = m_camera.GetMaxGridBounds();
        
        minX = std::max(minX, minBounds.x);
        maxX = std::min(maxX, maxBounds.x);
        minY = std::max(minY, minBounds.y);
        maxY = std::min(maxY, maxBounds.y);
    }
    
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            m_selectedCells.push_back(glm::ivec2(x, y));
        }
    }
    
    m_gridRenderer.SetSelectedCells(m_selectedCells);
}