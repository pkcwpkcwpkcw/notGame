#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "core/Gate.h"
#include "render/ShaderProgram.h"
#include "render/Camera.h"

class GateRenderer {
public:
    GateRenderer();
    ~GateRenderer();
    
    bool Initialize();
    void Cleanup();
    
    void BeginFrame();
    void RenderGates(const std::vector<Gate>& gates, const Camera& camera);
    void RenderGatePreview(const glm::vec2& position, GateType type, bool isValid, const Camera& camera);
    void RenderGateHighlight(const Gate& gate, const Camera& camera);
    void EndFrame();
    
    void SetGateSize(float size) { m_gateSize = size; }
    void EnableInstancing(bool enable) { m_useInstancing = enable; }
    
    float GetGateSize() const { return m_gateSize; }
    bool IsInstancingEnabled() const { return m_useInstancing; }
    
private:
    struct GateInstance {
        glm::vec2 position;
        glm::vec4 color;
        float rotation;
        float scale;
        float padding[2];
    };
    
    struct PortData {
        glm::vec2 offset;
        glm::vec4 color;
        bool isInput;
    };
    
    void SetupGeometry();
    void SetupShaders();
    void UpdateInstanceBuffer(const std::vector<Gate>& gates);
    void RenderBatch(const std::vector<GateInstance>& instances);
    void RenderSingleGate(const Gate& gate, const glm::mat4& mvp);
    void RenderPorts(const Gate& gate, const glm::mat4& mvp);
    
    glm::vec4 GetGateColor(const Gate& gate) const;
    glm::vec4 GetPortColor(bool hasSignal) const;
    
    std::vector<Gate> FrustumCull(const std::vector<Gate>& gates, const Camera& camera) const;
    
    GLuint m_vaoGate;
    GLuint m_vboGate;
    GLuint m_eboGate;
    GLuint m_vboInstance;
    GLuint m_vaoPort;
    GLuint m_vboPort;
    
    std::unique_ptr<ShaderProgram> m_gateShader;
    std::unique_ptr<ShaderProgram> m_portShader;
    
    float m_gateSize;
    bool m_useInstancing;
    size_t m_maxInstances;
    
    std::vector<GateInstance> m_instanceData;
    std::vector<float> m_vertices;
    std::vector<unsigned int> m_indices;
    
    bool m_initialized;
};