#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include "../render/Camera.h"

namespace Input {

class CoordinateTransformer {
private:
    Camera* m_camera = nullptr;
    glm::vec2 m_viewportSize{800, 600};
    float m_gridSize = 1.0f;
    float m_dpiScale = 1.0f;
    float m_pixelScale = 1.0f;
    
    struct TransformCache {
        glm::mat4 viewProj;
        glm::mat4 invViewProj;
        uint32_t frameNumber = 0;
        bool valid = false;
    } m_cache;
    
public:
    CoordinateTransformer() = default;
    
    void setCamera(Camera* camera) { 
        m_camera = camera;
        m_cache.valid = false;
    }
    
    void setViewport(float width, float height) {
        m_viewportSize = glm::vec2(width, height);
        m_cache.valid = false;
    }
    
    void setGridSize(float size) { 
        m_gridSize = size; 
    }
    
    void setDPIScale(float scale) { 
        m_dpiScale = scale; 
    }
    
    void setPixelScale(float scale) { 
        m_pixelScale = scale; 
    }
    
    void updateCache(uint32_t frameNumber) {
        if (!m_camera || (m_cache.valid && m_cache.frameNumber == frameNumber)) {
            return;
        }
        
        m_cache.viewProj = m_camera->GetProjectionMatrix() * m_camera->GetViewMatrix();
        m_cache.invViewProj = glm::inverse(m_cache.viewProj);
        m_cache.frameNumber = frameNumber;
        m_cache.valid = true;
    }
    
    glm::vec2 screenToNDC(const glm::vec2& screenPos) const {
        glm::vec2 adjustedPos = screenPos * m_pixelScale;
        return glm::vec2(
            2.0f * adjustedPos.x / m_viewportSize.x - 1.0f,
            1.0f - 2.0f * adjustedPos.y / m_viewportSize.y
        );
    }
    
    glm::vec2 ndcToWorld(const glm::vec2& ndcPos) const {
        if (!m_cache.valid) return ndcPos;
        
        glm::vec4 worldPos = m_cache.invViewProj * glm::vec4(ndcPos, 0.0f, 1.0f);
        if (worldPos.w != 0) {
            return glm::vec2(worldPos.x / worldPos.w, worldPos.y / worldPos.w);
        }
        return glm::vec2(worldPos.x, worldPos.y);
    }
    
    glm::vec2 screenToWorld(const glm::vec2& screenPos) {
        updateCache(0); 
        return ndcToWorld(screenToNDC(screenPos));
    }
    
    glm::ivec2 worldToGrid(const glm::vec2& worldPos) const {
        return glm::ivec2(
            static_cast<int>(std::floor(worldPos.x / m_gridSize)),
            static_cast<int>(std::floor(worldPos.y / m_gridSize))
        );
    }
    
    glm::vec2 gridToWorld(const glm::ivec2& gridPos) const {
        return glm::vec2(
            (gridPos.x + 0.5f) * m_gridSize,
            (gridPos.y + 0.5f) * m_gridSize
        );
    }
    
    glm::ivec2 screenToGrid(const glm::vec2& screenPos) {
        return worldToGrid(screenToWorld(screenPos));
    }
    
    glm::vec2 worldToScreen(const glm::vec2& worldPos) const {
        if (!m_cache.valid) return worldPos;
        
        glm::vec4 clipPos = m_cache.viewProj * glm::vec4(worldPos, 0.0f, 1.0f);
        if (clipPos.w != 0) {
            glm::vec2 ndcPos = glm::vec2(clipPos.x / clipPos.w, clipPos.y / clipPos.w);
            return glm::vec2(
                (ndcPos.x + 1.0f) * 0.5f * m_viewportSize.x,
                (1.0f - ndcPos.y) * 0.5f * m_viewportSize.y
            ) / m_pixelScale;
        }
        return worldPos;
    }
    
    float getGridSize() const { return m_gridSize; }
    glm::vec2 getViewportSize() const { return m_viewportSize; }
};

} // namespace Input