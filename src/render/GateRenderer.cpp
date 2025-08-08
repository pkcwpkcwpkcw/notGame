#include "GateRenderer.h"
#include <iostream>
#include <algorithm>
#include <SDL.h>

GateRenderer::GateRenderer()
    : m_vaoGate(0)
    , m_vboGate(0)
    , m_eboGate(0)
    , m_vboInstance(0)
    , m_vaoPort(0)
    , m_vboPort(0)
    , m_gateSize(1.0f)
    , m_useInstancing(true)
    , m_maxInstances(10000)
    , m_initialized(false) {
}

GateRenderer::~GateRenderer() {
    Cleanup();
}

bool GateRenderer::Initialize() {
    if (m_initialized) {
        return true;
    }
    
    SetupGeometry();
    SetupShaders();
    
    m_instanceData.reserve(m_maxInstances);
    
    m_initialized = true;
    return true;
}

void GateRenderer::Cleanup() {
    if (!m_initialized) {
        return;
    }
    
    if (m_vaoGate) glDeleteVertexArrays(1, &m_vaoGate);
    if (m_vboGate) glDeleteBuffers(1, &m_vboGate);
    if (m_eboGate) glDeleteBuffers(1, &m_eboGate);
    if (m_vboInstance) glDeleteBuffers(1, &m_vboInstance);
    if (m_vaoPort) glDeleteVertexArrays(1, &m_vaoPort);
    if (m_vboPort) glDeleteBuffers(1, &m_vboPort);
    
    m_gateShader.reset();
    m_portShader.reset();
    
    m_initialized = false;
}

void GateRenderer::SetupGeometry() {
    // 게이트를 셀 크기의 70%로 설정 (셀 중앙에 배치)
    const float gateSize = 0.35f;  // 셀 크기의 70%
    const float gateVertices[] = {
        -gateSize, -gateSize,  0.0f, 0.0f,
         gateSize, -gateSize,  1.0f, 0.0f,
         gateSize,  gateSize,  1.0f, 1.0f,
        -gateSize,  gateSize,  0.0f, 1.0f
    };
    
    const unsigned int gateIndices[] = {
        0, 1, 2,
        2, 3, 0
    };
    
    glGenVertexArrays(1, &m_vaoGate);
    glGenBuffers(1, &m_vboGate);
    glGenBuffers(1, &m_eboGate);
    
    glBindVertexArray(m_vaoGate);
    
    glBindBuffer(GL_ARRAY_BUFFER, m_vboGate);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gateVertices), gateVertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboGate);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gateIndices), gateIndices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    if (m_useInstancing) {
        glGenBuffers(1, &m_vboInstance);
        glBindBuffer(GL_ARRAY_BUFFER, m_vboInstance);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GateInstance) * m_maxInstances, nullptr, GL_DYNAMIC_DRAW);
        
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GateInstance), 
                            (void*)offsetof(GateInstance, position));
        glEnableVertexAttribArray(2);
        glVertexAttribDivisor(2, 1);
        
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(GateInstance), 
                            (void*)offsetof(GateInstance, color));
        glEnableVertexAttribArray(3);
        glVertexAttribDivisor(3, 1);
        
        glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(GateInstance), 
                            (void*)offsetof(GateInstance, rotation));
        glEnableVertexAttribArray(4);
        glVertexAttribDivisor(4, 1);
        
        glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(GateInstance), 
                            (void*)offsetof(GateInstance, scale));
        glEnableVertexAttribArray(5);
        glVertexAttribDivisor(5, 1);
    }
    
    const int portSegments = 8;
    std::vector<float> portVertices;
    portVertices.reserve((portSegments + 2) * 2);
    
    portVertices.push_back(0.0f);
    portVertices.push_back(0.0f);
    
    for (int i = 0; i <= portSegments; i++) {
        float angle = 2.0f * 3.14159f * i / portSegments;
        portVertices.push_back(0.1f * cos(angle));
        portVertices.push_back(0.1f * sin(angle));
    }
    
    glGenVertexArrays(1, &m_vaoPort);
    glGenBuffers(1, &m_vboPort);
    
    glBindVertexArray(m_vaoPort);
    glBindBuffer(GL_ARRAY_BUFFER, m_vboPort);
    glBufferData(GL_ARRAY_BUFFER, portVertices.size() * sizeof(float), 
                portVertices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
}

void GateRenderer::SetupShaders() {
    const char* gateVertexShader = R"(
#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec2 aInstancePos;
layout (location = 3) in vec4 aInstanceColor;
layout (location = 4) in float aInstanceRotation;
layout (location = 5) in float aInstanceScale;

uniform mat4 uProjection;
uniform mat4 uView;
uniform float uGridSize;

out vec2 TexCoord;
out vec4 GateColor;

void main() {
    float cosR = cos(aInstanceRotation);
    float sinR = sin(aInstanceRotation);
    mat2 rotation = mat2(cosR, -sinR, sinR, cosR);
    
    vec2 localPos = rotation * (aPos * aInstanceScale * uGridSize);
    // 셀 중심에 배치 (0.5 오프셋 추가)
    vec2 worldPos = localPos + (aInstancePos + vec2(0.5, 0.5)) * uGridSize;
    
    gl_Position = uProjection * uView * vec4(worldPos, 0.0, 1.0);
    
    TexCoord = aTexCoord;
    GateColor = aInstanceColor;
}
)";

    const char* gateFragmentShader = R"(
#version 330 core

in vec2 TexCoord;
in vec4 GateColor;

uniform vec4 uBorderColor;
uniform float uBorderWidth;

out vec4 FragColor;

void main() {
    float border = uBorderWidth / 100.0;
    if (TexCoord.x < border || TexCoord.x > 1.0 - border ||
        TexCoord.y < border || TexCoord.y > 1.0 - border) {
        FragColor = uBorderColor;
    } else {
        FragColor = GateColor;
    }
}
)";

    const char* simpleVertexShader = R"(
#version 330 core

layout (location = 0) in vec2 aPos;

uniform mat4 uMVP;
uniform vec2 uOffset;

void main() {
    gl_Position = uMVP * vec4(aPos + uOffset, 0.0, 1.0);
}
)";

    const char* simpleFragmentShader = R"(
#version 330 core

uniform vec4 uColor;

out vec4 FragColor;

void main() {
    FragColor = uColor;
}
)";

    m_gateShader = std::make_unique<ShaderProgram>();
    if (!m_gateShader->LoadFromSource(gateVertexShader, gateFragmentShader)) {
        std::cerr << "Failed to compile gate shader!" << std::endl;
    }
    
    m_portShader = std::make_unique<ShaderProgram>();
    if (!m_portShader->LoadFromSource(simpleVertexShader, simpleFragmentShader)) {
        std::cerr << "Failed to compile port shader!" << std::endl;
    }
}

void GateRenderer::BeginFrame() {
    m_instanceData.clear();
}

void GateRenderer::RenderGates(const std::vector<Gate>& gates, const Camera& camera) {
    if (gates.empty() || !m_gateShader || !m_portShader) {
        return;
    }
    
    auto visibleGates = FrustumCull(gates, camera);
    
    if (m_useInstancing && visibleGates.size() > 0) {  // 게이트가 하나라도 있으면 인스턴싱 사용
        m_instanceData.clear();
        m_instanceData.reserve(visibleGates.size());
        
        for (const auto& gate : visibleGates) {
            GateInstance instance;
            instance.position = glm::vec2(gate.position.x, gate.position.y);
            instance.color = GetGateColor(gate);
            instance.rotation = 0.0f;
            instance.scale = 1.0f;
            m_instanceData.push_back(instance);
            
            // 디버깅 로그 제거
        }
        
        if (!m_instanceData.empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, m_vboInstance);
            glBufferSubData(GL_ARRAY_BUFFER, 0, 
                          sizeof(GateInstance) * m_instanceData.size(),
                          m_instanceData.data());
            
            m_gateShader->Use();
            m_gateShader->SetUniform("uProjection", camera.GetProjectionMatrix());
            m_gateShader->SetUniform("uView", camera.GetViewMatrix());
            m_gateShader->SetUniform("uGridSize", m_gateSize);
            m_gateShader->SetUniform("uBorderColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));  // 밝은 흰색 테두리
            m_gateShader->SetUniform("uBorderWidth", 5.0f);  // 더 두껍게
            
            glBindVertexArray(m_vaoGate);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  // 채우기
            
            glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0,
                                  static_cast<GLsizei>(m_instanceData.size()));
        }
        
        for (const auto& gate : visibleGates) {
            RenderPorts(gate, camera.GetViewProjectionMatrix());
        }
    } else {
        for (const auto& gate : visibleGates) {
            RenderSingleGate(gate, camera.GetViewProjectionMatrix());
            RenderPorts(gate, camera.GetViewProjectionMatrix());
        }
    }
    
    glBindVertexArray(0);
}

void GateRenderer::EndFrame() {
}

void GateRenderer::RenderSingleGate(const Gate& gate, const glm::mat4& mvp) {
    m_gateShader->Use();
    
    // 셀 중심에 배치
    glm::vec3 cellCenter = glm::vec3((gate.position.x + 0.5f) * m_gateSize, (gate.position.y + 0.5f) * m_gateSize, 0.0f);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), cellCenter);
    model = glm::scale(model, glm::vec3(m_gateSize));
    
    m_gateShader->SetUniform("uProjection", mvp);
    m_gateShader->SetUniform("uView", glm::mat4(1.0f));
    m_gateShader->SetUniform("uGridSize", 1.0f);
    m_gateShader->SetUniform("uBorderColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));  // 흰색 테두리
    m_gateShader->SetUniform("uBorderWidth", 5.0f);  // 더 두껍게
    
    glBindVertexArray(m_vaoGate);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void GateRenderer::RenderPorts(const Gate& gate, const glm::mat4& mvp) {
    m_portShader->Use();
    m_portShader->SetUniform("uMVP", mvp);
    
    glBindVertexArray(m_vaoPort);
    
    // 셀 중심에 맞춰 조정
    glm::vec2 gateCenter = (glm::vec2(gate.position.x, gate.position.y) + glm::vec2(0.5f, 0.5f)) * m_gateSize;
    
    // 입력 포트를 제거하고 출력 포트만 렌더링
    // 출력 포트는 오른쪽에 1개
    glm::vec2 outputOffset(0.35f, 0.0f);  // 게이트 경계에 가까이
    glm::vec2 outputPos = gateCenter + outputOffset * m_gateSize;
    m_portShader->SetUniform("uOffset", outputPos);
    
    // 출력 포트는 신호 상태에 따라 색상 변경 (빨간색/초록색)
    if (gate.currentOutput == SignalState::HIGH) {
        m_portShader->SetUniform("uColor", glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));  // 초록색
    } else {
        m_portShader->SetUniform("uColor", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));  // 빨간색
    }
    
    glDrawArrays(GL_TRIANGLE_FAN, 0, 10);
}

glm::vec4 GateRenderer::GetGateColor(const Gate& gate) const {
    // 선택된 게이트는 노란색 하이라이트
    if (gate.isSelected) {
        SDL_Log("[GateRenderer] Rendering selected gate %u with yellow color", gate.id);
        return glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);  // 노란색
    }
    // NOT 게이트 본체는 진한 회색
    return glm::vec4(0.4f, 0.4f, 0.4f, 1.0f);
}

glm::vec4 GateRenderer::GetPortColor(bool hasSignal) const {
    if (hasSignal) {
        return glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    }
    return glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
}

std::vector<Gate> GateRenderer::FrustumCull(const std::vector<Gate>& gates, const Camera& camera) const {
    std::vector<Gate> visible;
    visible.reserve(gates.size());
    
    glm::vec2 cameraPos = camera.GetPosition();
    float viewDistance = camera.GetZoom() * 50.0f;
    
    for (const auto& gate : gates) {
        float distance = glm::length(glm::vec2(gate.position.x, gate.position.y) - cameraPos);
        if (distance < viewDistance) {
            visible.push_back(gate);
        }
    }
    
    return visible;
}

void GateRenderer::RenderGatePreview(const glm::vec2& position, GateType type, bool isValid, const Camera& camera) {
    if (!m_gateShader || !m_initialized) {
        return;
    }
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Set preview color based on validity
    glm::vec4 previewColor;
    if (isValid) {
        previewColor = glm::vec4(0.0f, 1.0f, 0.0f, 0.4f);  // Green semi-transparent
    } else {
        previewColor = glm::vec4(1.0f, 0.0f, 0.0f, 0.4f);  // Red semi-transparent
    }
    
    // Create instance data for preview
    GateInstance previewInstance;
    previewInstance.position = position;
    previewInstance.color = previewColor;
    previewInstance.rotation = 0.0f;
    previewInstance.scale = 1.0f;
    
    // Update instance buffer with preview data
    glBindBuffer(GL_ARRAY_BUFFER, m_vboInstance);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GateInstance), &previewInstance);
    
    // Use instanced rendering shader
    m_gateShader->Use();
    m_gateShader->SetUniform("uProjection", camera.GetProjectionMatrix());
    m_gateShader->SetUniform("uView", camera.GetViewMatrix());
    m_gateShader->SetUniform("uGridSize", m_gateSize);
    m_gateShader->SetUniform("uBorderColor", glm::vec4(1.0f, 1.0f, 1.0f, 0.8f));  // White border
    m_gateShader->SetUniform("uBorderWidth", 3.0f);
    
    // Draw the preview gate (single instance)
    glBindVertexArray(m_vaoGate);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, 1);
    
    glBindVertexArray(0);
    glDisable(GL_BLEND);
}

void GateRenderer::RenderGateHighlight(const Gate& gate, const Camera& camera) {
    if (!m_gateShader || !m_initialized) {
        return;
    }
    
    // Calculate MVP matrix
    glm::mat4 mvp = camera.GetProjectionMatrix() * camera.GetViewMatrix();
    
    // Draw highlight outline
    glLineWidth(3.0f);
    
    float highlightSize = m_gateSize * 1.1f;  // Slightly larger than gate
    glm::vec2 gatePos(gate.position.x, gate.position.y);
    
    float outlineVertices[] = {
        gatePos.x - highlightSize/2, gatePos.y - highlightSize/2, 0.0f,
        gatePos.x + highlightSize/2, gatePos.y - highlightSize/2, 0.0f,
        gatePos.x + highlightSize/2, gatePos.y + highlightSize/2, 0.0f,
        gatePos.x - highlightSize/2, gatePos.y + highlightSize/2, 0.0f
    };
    
    GLuint outlineVBO, outlineVAO;
    glGenVertexArrays(1, &outlineVAO);
    glGenBuffers(1, &outlineVBO);
    
    glBindVertexArray(outlineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, outlineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(outlineVertices), outlineVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    m_portShader->Use();
    m_portShader->SetUniform("uMVP", mvp);
    m_portShader->SetUniform("uOffset", glm::vec2(0.0f, 0.0f));
    
    // Yellow highlight for selected gates
    if (gate.isSelected) {
        m_portShader->SetUniform("uColor", glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
    }
    // Light blue for hovered gates  
    else if (gate.isHovered) {
        m_portShader->SetUniform("uColor", glm::vec4(0.5f, 0.8f, 1.0f, 1.0f));
    }
    
    glDrawArrays(GL_LINE_LOOP, 0, 4);
    
    glDeleteVertexArrays(1, &outlineVAO);
    glDeleteBuffers(1, &outlineVBO);
    glLineWidth(1.0f);
    
    glBindVertexArray(0);
}