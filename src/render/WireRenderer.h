#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <functional>
#include "render/RenderTypes.h"
#include "render/ShaderProgram.h"
#include "render/Camera.h"

class WireRenderer {
public:
    WireRenderer();
    ~WireRenderer();
    
    bool Initialize();
    void Cleanup();
    
    void RenderWires(const std::vector<RenderWire>& wires, const Camera& camera);
    void RenderDraggingWire(const glm::vec2& start, const glm::vec2& end, 
                           const Camera& camera);
    
    void SetLineWidth(float width) { m_lineWidth = width; }
    void SetAntialiasing(bool enable) { m_antialiasing = enable; }
    
    float GetLineWidth() const { return m_lineWidth; }
    bool IsAntialiasingEnabled() const { return m_antialiasing; }
    
private:
    struct WirePath {
        std::vector<glm::vec2> points;
        glm::vec4 color;
        float thickness;
        bool animated;
        float animationPhase;
    };
    
    void SetupShaders();
    void CalculatePath(const RenderWire& wire, WirePath& path);
    void RenderPath(const WirePath& path, const glm::mat4& mvp);
    void UpdateDynamicBuffer(const std::vector<WirePath>& paths);
    
    std::vector<glm::vec2> CalculateManhattanPath(const glm::vec2& start, 
                                                  const glm::vec2& end);
    std::vector<glm::vec2> CalculateSmartPath(const glm::vec2& start,
                                              const glm::vec2& end,
                                              const std::vector<glm::vec2>& obstacles);
    
    glm::vec4 GetWireColor(const RenderWire& wire) const;
    std::vector<RenderWire> FrustumCull(const std::vector<RenderWire>& wires, const Camera& camera) const;
    
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_vaoJoint;
    GLuint m_vboJoint;
    
    std::unique_ptr<ShaderProgram> m_wireShader;
    std::unique_ptr<ShaderProgram> m_jointShader;
    
    float m_lineWidth;
    bool m_antialiasing;
    float m_animationTime;
    
    std::vector<float> m_vertexBuffer;
    size_t m_maxVertices;
    
    bool m_initialized;
};