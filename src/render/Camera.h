#pragma once

namespace notgame {
namespace render {

class Camera {
public:
    Camera();
    ~Camera();
    
    // Position
    void setPosition(float x, float y);
    float getX() const { return x_; }
    float getY() const { return y_; }
    
    // Zoom
    void setZoom(float zoom);
    float getZoom() const { return zoom_; }
    void zoomIn(float factor = 1.2f);
    void zoomOut(float factor = 1.2f);
    
    // Movement
    void move(float dx, float dy);
    void panTo(float x, float y);
    
    // Viewport
    void setViewport(int width, int height);
    int getViewportWidth() const { return viewportWidth_; }
    int getViewportHeight() const { return viewportHeight_; }
    
    // Coordinate conversion
    void screenToWorld(int screenX, int screenY, float& worldX, float& worldY) const;
    void worldToScreen(float worldX, float worldY, int& screenX, int& screenY) const;
    
    // Grid alignment
    void gridToWorld(int gridX, int gridY, float& worldX, float& worldY) const;
    void worldToGrid(float worldX, float worldY, int& gridX, int& gridY) const;
    
private:
    float x_, y_;  // Camera position in world coordinates
    float zoom_;   // Zoom level (1.0 = normal)
    
    int viewportWidth_;
    int viewportHeight_;
    
    static constexpr float MIN_ZOOM = 0.1f;
    static constexpr float MAX_ZOOM = 10.0f;
    static constexpr float GRID_SIZE = 32.0f;  // Pixels per grid cell at zoom 1.0
};

} // namespace render
} // namespace notgame