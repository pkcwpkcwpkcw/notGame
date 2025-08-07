#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    Camera(int screenWidth, int screenHeight);
    
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;
    glm::mat4 GetViewProjectionMatrix() const;
    
    glm::vec2 ScreenToWorld(const glm::vec2& screenPos) const;
    glm::vec2 WorldToScreen(const glm::vec2& worldPos) const;
    glm::ivec2 ScreenToGrid(const glm::vec2& screenPos) const;
    glm::vec2 GridToScreen(const glm::ivec2& gridPos) const;
    
    void Pan(const glm::vec2& delta);
    void Zoom(float factor, const glm::vec2& screenPos);
    void Reset();
    
    void SetPosition(const glm::vec2& position) { m_position = position; }
    glm::vec2 GetPosition() const { return m_position; }
    
    void SetZoom(float zoom);
    float GetZoom() const { return m_zoom; }
    
    void SetScreenSize(int width, int height);
    glm::vec2 GetScreenSize() const { return m_screenSize; }
    
    // Grid boundary management
    void SetGridBounds(const glm::ivec2& minBounds, const glm::ivec2& maxBounds);
    void SetUnlimitedGrid(bool unlimited = true);
    bool IsGridUnlimited() const { return m_unlimitedGrid; }
    glm::ivec2 GetMinGridBounds() const { return m_minGridBounds; }
    glm::ivec2 GetMaxGridBounds() const { return m_maxGridBounds; }
    
    glm::vec4 GetVisibleBounds() const;
    bool IsGridCellVisible(const glm::ivec2& gridPos) const;
    
    static constexpr float MIN_ZOOM = 0.1f;
    static constexpr float MAX_ZOOM = 10.0f;
    static constexpr float DEFAULT_CELL_SIZE = 32.0f;
    
private:
    glm::vec2 m_position;
    float m_zoom;
    glm::vec2 m_screenSize;
    
    // Grid boundaries
    bool m_unlimitedGrid;
    glm::ivec2 m_minGridBounds;
    glm::ivec2 m_maxGridBounds;
    
    // Helper function to clamp camera position
    void ClampCameraPosition();
    
    float GetPixelsPerGrid() const { return DEFAULT_CELL_SIZE * m_zoom; }
};

#endif // CAMERA_H