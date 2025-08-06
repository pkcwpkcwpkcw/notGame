#include "render/Camera.h"
#include <algorithm>
#include <cmath>

namespace notgame {
namespace render {

Camera::Camera()
    : x_(0.0f)
    , y_(0.0f)
    , zoom_(1.0f)
    , viewportWidth_(800)
    , viewportHeight_(600) {
}

Camera::~Camera() {
}

void Camera::setPosition(float x, float y) {
    x_ = x;
    y_ = y;
}

void Camera::setZoom(float zoom) {
    zoom_ = std::clamp(zoom, MIN_ZOOM, MAX_ZOOM);
}

void Camera::zoomIn(float factor) {
    setZoom(zoom_ * factor);
}

void Camera::zoomOut(float factor) {
    setZoom(zoom_ / factor);
}

void Camera::move(float dx, float dy) {
    x_ += dx;
    y_ += dy;
}

void Camera::panTo(float x, float y) {
    x_ = x;
    y_ = y;
}

void Camera::setViewport(int width, int height) {
    viewportWidth_ = width;
    viewportHeight_ = height;
}

void Camera::screenToWorld(int screenX, int screenY, float& worldX, float& worldY) const {
    // Convert screen coordinates to world coordinates
    float centerX = viewportWidth_ / 2.0f;
    float centerY = viewportHeight_ / 2.0f;
    
    worldX = x_ + (screenX - centerX) / (GRID_SIZE * zoom_);
    worldY = y_ + (screenY - centerY) / (GRID_SIZE * zoom_);
}

void Camera::worldToScreen(float worldX, float worldY, int& screenX, int& screenY) const {
    // Convert world coordinates to screen coordinates
    float centerX = viewportWidth_ / 2.0f;
    float centerY = viewportHeight_ / 2.0f;
    
    screenX = static_cast<int>(centerX + (worldX - x_) * GRID_SIZE * zoom_);
    screenY = static_cast<int>(centerY + (worldY - y_) * GRID_SIZE * zoom_);
}

void Camera::gridToWorld(int gridX, int gridY, float& worldX, float& worldY) const {
    worldX = static_cast<float>(gridX);
    worldY = static_cast<float>(gridY);
}

void Camera::worldToGrid(float worldX, float worldY, int& gridX, int& gridY) const {
    gridX = static_cast<int>(std::floor(worldX + 0.5f));
    gridY = static_cast<int>(std::floor(worldY + 0.5f));
}

} // namespace render
} // namespace notgame