#include "Camera.h"
#include <algorithm>
#include <cmath>

Camera::Camera(int screenWidth, int screenHeight)
    : m_position(0.0f, 0.0f)
    , m_zoom(1.0f)
    , m_screenSize(static_cast<float>(screenWidth), static_cast<float>(screenHeight))
    , m_unlimitedGrid(true)
    , m_minGridBounds(-100, -100)
    , m_maxGridBounds(100, 100) {
}

glm::mat4 Camera::GetViewMatrix() const {
    glm::mat4 view(1.0f);
    view = glm::translate(view, glm::vec3(-m_position.x, -m_position.y, 0.0f));
    return view;
}

glm::mat4 Camera::GetProjectionMatrix() const {
    float halfWidth = m_screenSize.x * 0.5f / (DEFAULT_CELL_SIZE * m_zoom);
    float halfHeight = m_screenSize.y * 0.5f / (DEFAULT_CELL_SIZE * m_zoom);
    
    return glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, -1.0f, 1.0f);
}

glm::mat4 Camera::GetViewProjectionMatrix() const {
    return GetProjectionMatrix() * GetViewMatrix();
}

glm::vec2 Camera::ScreenToWorld(const glm::vec2& screenPos) const {
    // 스크린 좌표를 중심 기준으로 변환
    glm::vec2 offset = screenPos - m_screenSize * 0.5f;
    // Y축 반전 (스크린 Y는 아래로 증가, 월드 Y는 위로 증가)
    offset.y = -offset.y;
    return m_position + offset / (DEFAULT_CELL_SIZE * m_zoom);
}

glm::vec2 Camera::WorldToScreen(const glm::vec2& worldPos) const {
    glm::vec2 offset = (worldPos - m_position) * (DEFAULT_CELL_SIZE * m_zoom);
    // Y축 반전 (월드 Y는 위로 증가, 스크린 Y는 아래로 증가)
    offset.y = -offset.y;
    return offset + m_screenSize * 0.5f;
}

glm::ivec2 Camera::ScreenToGrid(const glm::vec2& screenPos) const {
    glm::vec2 worldPos = ScreenToWorld(screenPos);
    return glm::ivec2(floor(worldPos.x), floor(worldPos.y));
}

glm::vec2 Camera::GridToScreen(const glm::ivec2& gridPos) const {
    glm::vec2 worldPos(gridPos.x + 0.5f, gridPos.y + 0.5f);
    return WorldToScreen(worldPos);
}

void Camera::Pan(const glm::vec2& delta) {
    // delta는 스크린 좌표계 (Y축 아래가 양수)
    glm::vec2 worldDelta = delta / (DEFAULT_CELL_SIZE * m_zoom);
    // 스크린 Y축을 월드 Y축으로 변환 (반전)
    worldDelta.y = -worldDelta.y;
    // 카메라를 반대 방향으로 이동 (끌어당기는 효과)
    m_position -= worldDelta;
    
    if (!m_unlimitedGrid) {
        ClampCameraPosition();
    }
}

void Camera::Zoom(float factor, const glm::vec2& screenPos) {
    glm::vec2 worldBeforeZoom = ScreenToWorld(screenPos);
    
    m_zoom = glm::clamp(m_zoom * factor, MIN_ZOOM, MAX_ZOOM);
    
    glm::vec2 worldAfterZoom = ScreenToWorld(screenPos);
    
    m_position += worldBeforeZoom - worldAfterZoom;
    
    if (!m_unlimitedGrid) {
        ClampCameraPosition();
    }
}

void Camera::Reset() {
    m_position = glm::vec2(0.0f, 0.0f);
    m_zoom = 1.0f;
    
    if (!m_unlimitedGrid) {
        ClampCameraPosition();
    }
}

void Camera::SetZoom(float zoom) {
    m_zoom = glm::clamp(zoom, MIN_ZOOM, MAX_ZOOM);
}

void Camera::SetScreenSize(int width, int height) {
    m_screenSize.x = static_cast<float>(width);
    m_screenSize.y = static_cast<float>(height);
}

glm::vec4 Camera::GetVisibleBounds() const {
    // 스크린의 네 모서리를 월드 좌표로 변환
    glm::vec2 topLeft = ScreenToWorld(glm::vec2(0, 0));
    glm::vec2 bottomRight = ScreenToWorld(m_screenSize);
    
    // Y축이 반전되므로 min/max를 올바르게 설정
    float minX = std::min(topLeft.x, bottomRight.x);
    float maxX = std::max(topLeft.x, bottomRight.x);
    float minY = std::min(topLeft.y, bottomRight.y);
    float maxY = std::max(topLeft.y, bottomRight.y);
    
    return glm::vec4(minX, minY, maxX, maxY);
}

bool Camera::IsGridCellVisible(const glm::ivec2& gridPos) const {
    glm::vec4 bounds = GetVisibleBounds();
    
    return gridPos.x >= floor(bounds.x) - 1 && 
           gridPos.x <= ceil(bounds.z) + 1 &&
           gridPos.y >= floor(bounds.y) - 1 && 
           gridPos.y <= ceil(bounds.w) + 1;
}

void Camera::SetGridBounds(const glm::ivec2& minBounds, const glm::ivec2& maxBounds) {
    m_minGridBounds = minBounds;
    m_maxGridBounds = maxBounds;
    m_unlimitedGrid = false;
    
    // Clamp current position to new bounds
    ClampCameraPosition();
}

void Camera::SetUnlimitedGrid(bool unlimited) {
    m_unlimitedGrid = unlimited;
}

void Camera::ClampCameraPosition() {
    if (m_unlimitedGrid) return;
    
    // Calculate visible area in grid units
    float halfWidth = m_screenSize.x * 0.5f / (DEFAULT_CELL_SIZE * m_zoom);
    float halfHeight = m_screenSize.y * 0.5f / (DEFAULT_CELL_SIZE * m_zoom);
    
    // Calculate the camera bounds that would keep the grid visible
    float minCamX = m_minGridBounds.x + halfWidth;
    float maxCamX = m_maxGridBounds.x - halfWidth;
    float minCamY = m_minGridBounds.y + halfHeight;
    float maxCamY = m_maxGridBounds.y - halfHeight;
    
    // Handle case where grid is smaller than view
    if (minCamX > maxCamX) {
        // Center camera horizontally on the grid
        m_position.x = (m_minGridBounds.x + m_maxGridBounds.x) * 0.5f;
    } else {
        m_position.x = glm::clamp(m_position.x, minCamX, maxCamX);
    }
    
    if (minCamY > maxCamY) {
        // Center camera vertically on the grid
        m_position.y = (m_minGridBounds.y + m_maxGridBounds.y) * 0.5f;
    } else {
        m_position.y = glm::clamp(m_position.y, minCamY, maxCamY);
    }
}