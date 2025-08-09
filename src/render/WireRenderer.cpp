#include "WireRenderer.h"
#include <iostream>
#include <algorithm>
#include <cmath>

WireRenderer::WireRenderer()
    : m_vao(0)
    , m_vbo(0)
    , m_vaoJoint(0)
    , m_vboJoint(0)
    , m_lineWidth(2.0f)
    , m_antialiasing(true)
    , m_animationTime(0.0f)
    , m_maxVertices(100000)
    , m_initialized(false) {
}

WireRenderer::~WireRenderer() {
    Cleanup();
}

bool WireRenderer::Initialize() {
    if (m_initialized) {
        return true;
    }
    
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_maxVertices * 6 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    const int jointSegments = 8;
    std::vector<float> jointVertices;
    jointVertices.reserve((jointSegments + 2) * 2);
    
    jointVertices.push_back(0.0f);
    jointVertices.push_back(0.0f);
    
    for (int i = 0; i <= jointSegments; i++) {
        float angle = 2.0f * 3.14159f * i / jointSegments;
        jointVertices.push_back(0.05f * cos(angle));
        jointVertices.push_back(0.05f * sin(angle));
    }
    
    glGenVertexArrays(1, &m_vaoJoint);
    glGenBuffers(1, &m_vboJoint);
    
    glBindVertexArray(m_vaoJoint);
    glBindBuffer(GL_ARRAY_BUFFER, m_vboJoint);
    glBufferData(GL_ARRAY_BUFFER, jointVertices.size() * sizeof(float), 
                jointVertices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
    
    SetupShaders();
    
    m_vertexBuffer.reserve(m_maxVertices * 6);
    
    m_initialized = true;
    return true;
}

void WireRenderer::Cleanup() {
    if (!m_initialized) {
        return;
    }
    
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vaoJoint) glDeleteVertexArrays(1, &m_vaoJoint);
    if (m_vboJoint) glDeleteBuffers(1, &m_vboJoint);
    
    m_wireShader.reset();
    m_jointShader.reset();
    
    m_initialized = false;
}

void WireRenderer::SetupShaders() {
    const char* wireVertexShader = R"(
#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;

uniform mat4 uProjection;
uniform mat4 uView;

out vec4 WireColor;

void main() {
    gl_Position = uProjection * uView * vec4(aPos, 0.0, 1.0);
    WireColor = aColor;
}
)";

    const char* wireFragmentShader = R"(
#version 330 core

in vec4 WireColor;

uniform float uTime;
uniform bool uAnimated;
uniform float uAnimationSpeed;

out vec4 FragColor;

void main() {
    vec4 color = WireColor;
    
    if (uAnimated) {
        float pulse = sin(uTime * uAnimationSpeed) * 0.5 + 0.5;
        color.rgb = mix(color.rgb, vec3(1.0, 1.0, 0.0), pulse * 0.3);
    }
    
    FragColor = color;
}
)";

    const char* jointVertexShader = R"(
#version 330 core

layout (location = 0) in vec2 aPos;

uniform mat4 uMVP;
uniform vec2 uOffset;

void main() {
    gl_Position = uMVP * vec4(aPos + uOffset, 0.0, 1.0);
}
)";

    const char* jointFragmentShader = R"(
#version 330 core

uniform vec4 uColor;

out vec4 FragColor;

void main() {
    FragColor = uColor;
}
)";

    m_wireShader = std::make_unique<ShaderProgram>();
    if (!m_wireShader->LoadFromSource(wireVertexShader, wireFragmentShader)) {
        std::cerr << "Failed to compile wire shader!" << std::endl;
    }
    
    m_jointShader = std::make_unique<ShaderProgram>();
    if (!m_jointShader->LoadFromSource(jointVertexShader, jointFragmentShader)) {
        std::cerr << "Failed to compile joint shader!" << std::endl;
    }
}

void WireRenderer::RenderWires(const std::vector<RenderWire>& wires, const Camera& camera) {
    if (wires.empty() || !m_wireShader || !m_jointShader) {
        return;
    }
    
    auto visibleWires = FrustumCull(wires, camera);
    
    m_vertexBuffer.clear();
    std::vector<glm::vec2> joints;
    
    for (const auto& wire : visibleWires) {
        WirePath path;
        CalculatePath(wire, path);
        
        glm::vec4 color = GetWireColor(wire);
        
        for (size_t i = 0; i < path.points.size() - 1; i++) {
            m_vertexBuffer.push_back(path.points[i].x);
            m_vertexBuffer.push_back(path.points[i].y);
            m_vertexBuffer.push_back(color.r);
            m_vertexBuffer.push_back(color.g);
            m_vertexBuffer.push_back(color.b);
            m_vertexBuffer.push_back(color.a);
            
            m_vertexBuffer.push_back(path.points[i + 1].x);
            m_vertexBuffer.push_back(path.points[i + 1].y);
            m_vertexBuffer.push_back(color.r);
            m_vertexBuffer.push_back(color.g);
            m_vertexBuffer.push_back(color.b);
            m_vertexBuffer.push_back(color.a);
        }
        
        joints.push_back(path.points.front());
        joints.push_back(path.points.back());
        
        for (size_t i = 1; i < path.points.size() - 1; i++) {
            joints.push_back(path.points[i]);
        }
    }
    
    if (!m_vertexBuffer.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 
                       m_vertexBuffer.size() * sizeof(float),
                       m_vertexBuffer.data());
        
        m_wireShader->Use();
        m_wireShader->SetUniform("uProjection", camera.GetProjectionMatrix());
        m_wireShader->SetUniform("uView", camera.GetViewMatrix());
        m_wireShader->SetUniform("uTime", m_animationTime);
        m_wireShader->SetUniform("uAnimated", false);
        m_wireShader->SetUniform("uAnimationSpeed", 3.0f);
        
        glBindVertexArray(m_vao);
        glLineWidth(m_lineWidth);
        
        if (m_antialiasing) {
            glEnable(GL_LINE_SMOOTH);
            glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        }
        
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_vertexBuffer.size() / 6));
        
        if (m_antialiasing) {
            glDisable(GL_LINE_SMOOTH);
        }
    }
    
    if (!joints.empty()) {
        m_jointShader->Use();
        m_jointShader->SetUniform("uMVP", camera.GetViewProjectionMatrix());
        
        glBindVertexArray(m_vaoJoint);
        
        for (const auto& joint : joints) {
            m_jointShader->SetUniform("uOffset", joint);
            m_jointShader->SetUniform("uColor", glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
            glDrawArrays(GL_TRIANGLE_FAN, 0, 10);
        }
    }
    
    glBindVertexArray(0);
    
    m_animationTime += 0.016f;
}

void WireRenderer::RenderDraggingWire(const glm::vec2& start, const glm::vec2& end, 
                                      const Camera& camera) {
    if (!m_wireShader) {
        return;
    }
    
    auto path = CalculateManhattanPath(start, end);
    
    m_vertexBuffer.clear();
    glm::vec4 color(0.6f, 0.6f, 1.0f, 0.8f);
    
    for (size_t i = 0; i < path.size() - 1; i++) {
        m_vertexBuffer.push_back(path[i].x);
        m_vertexBuffer.push_back(path[i].y);
        m_vertexBuffer.push_back(color.r);
        m_vertexBuffer.push_back(color.g);
        m_vertexBuffer.push_back(color.b);
        m_vertexBuffer.push_back(color.a);
        
        m_vertexBuffer.push_back(path[i + 1].x);
        m_vertexBuffer.push_back(path[i + 1].y);
        m_vertexBuffer.push_back(color.r);
        m_vertexBuffer.push_back(color.g);
        m_vertexBuffer.push_back(color.b);
        m_vertexBuffer.push_back(color.a);
    }
    
    if (!m_vertexBuffer.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 
                       m_vertexBuffer.size() * sizeof(float),
                       m_vertexBuffer.data());
        
        m_wireShader->Use();
        m_wireShader->SetUniform("uProjection", camera.GetProjectionMatrix());
        m_wireShader->SetUniform("uView", camera.GetViewMatrix());
        m_wireShader->SetUniform("uAnimated", true);
        m_wireShader->SetUniform("uTime", m_animationTime);
        m_wireShader->SetUniform("uAnimationSpeed", 5.0f);
        
        glBindVertexArray(m_vao);
        glLineWidth(m_lineWidth * 1.5f);
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_vertexBuffer.size() / 6));
        
        glDisable(GL_BLEND);
        glBindVertexArray(0);
    }
}

void WireRenderer::CalculatePath(const RenderWire& wire, WirePath& path) {
    path.points = CalculateManhattanPath(wire.start, wire.end);
    path.color = GetWireColor(wire);
    path.thickness = m_lineWidth;
    path.animated = wire.hasSignal;
    path.animationPhase = 0.0f;
}

std::vector<glm::vec2> WireRenderer::CalculateManhattanPath(const glm::vec2& start, 
                                                            const glm::vec2& end) {
    std::vector<glm::vec2> path;
    
    // Cell-to-cell wires already have correct world coordinates, don't adjust
    // Only add offset if coordinates appear to be grid positions (integers)
    glm::vec2 adjustedStart = start;
    glm::vec2 adjustedEnd = end;
    
    // Check if coordinates look like grid positions (close to integers)
    bool isGridStart = (std::abs(start.x - std::round(start.x)) < 0.1f && 
                        std::abs(start.y - std::round(start.y)) < 0.1f);
    bool isGridEnd = (std::abs(end.x - std::round(end.x)) < 0.1f && 
                      std::abs(end.y - std::round(end.y)) < 0.1f);
    
    if (isGridStart) {
        adjustedStart = start + glm::vec2(0.5f, 0.5f);
    }
    if (isGridEnd) {
        adjustedEnd = end + glm::vec2(0.5f, 0.5f);
    }
    
    path.push_back(adjustedStart);
    
    glm::vec2 diff = adjustedEnd - adjustedStart;
    
    if (std::abs(diff.x) < 0.001f || std::abs(diff.y) < 0.001f) {
        path.push_back(adjustedEnd);
        return path;
    }
    
    if (std::abs(diff.x) > std::abs(diff.y)) {
        path.push_back(glm::vec2(adjustedEnd.x, adjustedStart.y));
        path.push_back(adjustedEnd);
    } else {
        path.push_back(glm::vec2(adjustedStart.x, adjustedEnd.y));
        path.push_back(adjustedEnd);
    }
    
    return path;
}

std::vector<glm::vec2> WireRenderer::CalculateSmartPath(const glm::vec2& start,
                                                        const glm::vec2& end,
                                                        const std::vector<glm::vec2>& obstacles) {
    return CalculateManhattanPath(start, end);
}

glm::vec4 WireRenderer::GetWireColor(const RenderWire& wire) const {
    if (wire.hasSignal) {
        return glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);  // 초록색 (신호 있음)
    }
    return glm::vec4(0.4f, 0.4f, 0.4f, 1.0f);  // 회색 (신호 없음)
}

std::vector<RenderWire> WireRenderer::FrustumCull(const std::vector<RenderWire>& wires, const Camera& camera) const {
    std::vector<RenderWire> visible;
    visible.reserve(wires.size());
    
    glm::vec2 cameraPos = camera.GetPosition();
    float viewDistance = camera.GetZoom() * 50.0f;
    
    for (const auto& wire : wires) {
        glm::vec2 center = (wire.start + wire.end) * 0.5f;
        float distance = glm::length(center - cameraPos);
        
        if (distance < viewDistance) {
            visible.push_back(wire);
        }
    }
    
    return visible;
}