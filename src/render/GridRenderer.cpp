#include "GridRenderer.h"
#include "Camera.h"
#include <SDL.h>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>
#include <climits>

const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec2 aPosition;
layout(location = 1) in float aIntensity;

uniform mat4 uViewProjMatrix;

out float vIntensity;

void main() {
    gl_Position = uViewProjMatrix * vec4(aPosition, 0.0, 1.0);
    vIntensity = aIntensity;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in float vIntensity;

uniform vec4 uGridColor;
uniform float uGridOpacity;

out vec4 FragColor;

void main() {
    FragColor = vec4(uGridColor.rgb, uGridColor.a * uGridOpacity * vIntensity);
}
)";

GridRenderer::GridRenderer() 
    : m_gridVAO(0)
    , m_gridVBO(0)
    , m_highlightVAO(0)
    , m_highlightVBO(0)
    , m_shaderProgram(0)
    , m_viewProjMatrixLoc(-1)
    , m_gridColorLoc(-1)
    , m_gridOpacityLoc(-1)
    , m_cellSizeLoc(-1)
    , m_isGridVisible(true)
    , m_gridOpacity(0.5f)
    , m_cellSize(32.0f)
    , m_hoveredCell(INT_MIN, INT_MIN)  // 특별한 값으로 "없음" 표시
    , m_screenWidth(800)
    , m_screenHeight(600)
    , m_gridVertexCount(0)
    , m_highlightVertexCount(0) {
}

GridRenderer::~GridRenderer() {
    Shutdown();
}

bool GridRenderer::Initialize(int screenWidth, int screenHeight) {
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;
    
    if (!CompileShaders()) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to compile shaders");
        return false;
    }
    
    CreateGridMesh();
    CreateHighlightMesh();
    
    m_isGridVisible = true;
    m_gridOpacity = 0.5f;
    m_cellSize = 32.0f;
    
    CheckGLError("GridRenderer::Initialize");
    
    return true;
}

void GridRenderer::Shutdown() {
    if (m_gridVAO) {
        glDeleteVertexArrays(1, &m_gridVAO);
        m_gridVAO = 0;
    }
    if (m_gridVBO) {
        glDeleteBuffers(1, &m_gridVBO);
        m_gridVBO = 0;
    }
    if (m_highlightVAO) {
        glDeleteVertexArrays(1, &m_highlightVAO);
        m_highlightVAO = 0;
    }
    if (m_highlightVBO) {
        glDeleteBuffers(1, &m_highlightVBO);
        m_highlightVBO = 0;
    }
    if (m_shaderProgram) {
        glDeleteProgram(m_shaderProgram);
        m_shaderProgram = 0;
    }
}

void GridRenderer::Render(const Camera& camera) {
    if (!m_isGridVisible) return;
    
    // Save OpenGL state
    GLint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    
    // Enable blending for grid transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glUseProgram(m_shaderProgram);
    
    glm::mat4 viewProj = camera.GetViewProjectionMatrix();
    glUniformMatrix4fv(m_viewProjMatrixLoc, 1, GL_FALSE, glm::value_ptr(viewProj));
    
    glUniform4f(m_gridColorLoc, 0.3f, 0.3f, 0.3f, 1.0f);
    glUniform1f(m_gridOpacityLoc, m_gridOpacity);
    
    UpdateGridBuffer(camera);
    RenderGrid();
    
    if (m_hoveredCell.x >= 0 || !m_selectedCells.empty()) {
        UpdateHighlightBuffer();
        RenderHighlights();
    }
    
    // Restore OpenGL state
    glUseProgram(currentProgram);
    
    // Keep blending enabled for ImGui
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void GridRenderer::RenderGrid() {
    glBindVertexArray(m_gridVAO);
    glDrawArrays(GL_LINES, 0, m_gridVertexCount);
    glBindVertexArray(0);
}

void GridRenderer::RenderHighlights() {
    if (m_highlightVertexCount == 0) return;
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glUniform4f(m_gridColorLoc, 0.4f, 0.5f, 1.0f, 0.3f);
    
    glBindVertexArray(m_highlightVAO);
    glDrawArrays(GL_TRIANGLES, 0, m_highlightVertexCount);
    glBindVertexArray(0);
    
    glDisable(GL_BLEND);
}

void GridRenderer::SetHoveredCell(const glm::ivec2& cell) {
    // 그리드 경계 체크는 Camera에서 처리하도록 하고,
    // 여기서는 단순히 저장만 함
    m_hoveredCell = cell;
}

void GridRenderer::SetSelectedCells(const std::vector<glm::ivec2>& cells) {
    m_selectedCells = cells;
}

void GridRenderer::ClearSelection() {
    m_selectedCells.clear();
    m_hoveredCell = glm::ivec2(INT_MIN, INT_MIN);
}

void GridRenderer::OnResize(int width, int height) {
    m_screenWidth = width;
    m_screenHeight = height;
}

void GridRenderer::CreateGridMesh() {
    glGenVertexArrays(1, &m_gridVAO);
    glGenBuffers(1, &m_gridVBO);
    
    glBindVertexArray(m_gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_gridVBO);
    
    size_t bufferSize = sizeof(float) * 3 * 10000;
    glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}

void GridRenderer::CreateHighlightMesh() {
    glGenVertexArrays(1, &m_highlightVAO);
    glGenBuffers(1, &m_highlightVBO);
    
    glBindVertexArray(m_highlightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_highlightVBO);
    
    size_t bufferSize = sizeof(float) * 3 * 6 * 100;
    glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}

bool GridRenderer::CompileShaders() {
    GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
    if (!vertexShader) return false;
    
    GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    if (!fragmentShader) {
        glDeleteShader(vertexShader);
        return false;
    }
    
    if (!LinkShaderProgram(vertexShader, fragmentShader)) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    m_viewProjMatrixLoc = glGetUniformLocation(m_shaderProgram, "uViewProjMatrix");
    m_gridColorLoc = glGetUniformLocation(m_shaderProgram, "uGridColor");
    m_gridOpacityLoc = glGetUniformLocation(m_shaderProgram, "uGridOpacity");
    
    return true;
}

void GridRenderer::UpdateGridBuffer(const Camera& camera) {
    std::vector<float> vertices;
    vertices.reserve(10000);
    
    glm::vec4 bounds = camera.GetVisibleBounds();
    
    int startX = static_cast<int>(floor(bounds.x));
    int endX = static_cast<int>(ceil(bounds.z));
    int startY = static_cast<int>(floor(bounds.y));
    int endY = static_cast<int>(ceil(bounds.w));
    
    // Apply grid bounds if not unlimited
    if (!camera.IsGridUnlimited()) {
        glm::ivec2 minBounds = camera.GetMinGridBounds();
        glm::ivec2 maxBounds = camera.GetMaxGridBounds();
        
        // Apply bounds without logging
        startX = std::max(startX, minBounds.x);
        endX = std::min(endX, maxBounds.x + 1);  // +1 for drawing the last line
        startY = std::max(startY, minBounds.y);
        endY = std::min(endY, maxBounds.y + 1);  // +1 for drawing the last line
    } else {
        // Default limits for unlimited grid (can still be very large)
        startX = std::max(startX, -1000);
        endX = std::min(endX, 1000);
        startY = std::max(startY, -1000);
        endY = std::min(endY, 1000);
    }
    
    // Draw vertical lines (그리드 좌표를 월드 좌표로 사용)
    for (int x = startX; x <= endX; ++x) {
        float intensity = (x % 10 == 0) ? 1.0f : 0.5f;
        if (x == 0) intensity = 1.5f;
        
        // 그리드 좌표를 직접 사용 (셰이더에서 변환)
        vertices.push_back(static_cast<float>(x));
        vertices.push_back(static_cast<float>(startY));
        vertices.push_back(intensity);
        
        vertices.push_back(static_cast<float>(x));
        vertices.push_back(static_cast<float>(endY));
        vertices.push_back(intensity);
    }
    
    // Draw horizontal lines
    for (int y = startY; y <= endY; ++y) {
        float intensity = (y % 10 == 0) ? 1.0f : 0.5f;
        if (y == 0) intensity = 1.5f;
        
        vertices.push_back(static_cast<float>(startX));
        vertices.push_back(static_cast<float>(y));
        vertices.push_back(intensity);
        
        vertices.push_back(static_cast<float>(endX));
        vertices.push_back(static_cast<float>(y));
        vertices.push_back(intensity);
    }
    
    m_gridVertexCount = vertices.size() / 3;
    
    glBindBuffer(GL_ARRAY_BUFFER, m_gridVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * vertices.size(), vertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GridRenderer::UpdateHighlightBuffer() {
    std::vector<float> vertices;
    vertices.reserve(600);
    
    auto addQuad = [&](float x, float y, float size) {
        // 그리드 좌표를 직접 사용 (size는 1.0으로 고정)
        vertices.push_back(x); vertices.push_back(y); vertices.push_back(1.0f);
        vertices.push_back(x + 1.0f); vertices.push_back(y); vertices.push_back(1.0f);
        vertices.push_back(x + 1.0f); vertices.push_back(y + 1.0f); vertices.push_back(1.0f);
        
        vertices.push_back(x); vertices.push_back(y); vertices.push_back(1.0f);
        vertices.push_back(x + 1.0f); vertices.push_back(y + 1.0f); vertices.push_back(1.0f);
        vertices.push_back(x); vertices.push_back(y + 1.0f); vertices.push_back(1.0f);
    };
    
    // 호버 셀이 유효한 경우에만 그림 (INT_MIN은 "없음"을 의미)
    if (m_hoveredCell.x != INT_MIN && m_hoveredCell.y != INT_MIN) {
        addQuad(static_cast<float>(m_hoveredCell.x), 
                static_cast<float>(m_hoveredCell.y), 1.0f);
    }
    
    for (const auto& cell : m_selectedCells) {
        addQuad(static_cast<float>(cell.x), 
                static_cast<float>(cell.y), 1.0f);
    }
    
    m_highlightVertexCount = vertices.size() / 3;
    
    if (m_highlightVertexCount > 0) {
        glBindBuffer(GL_ARRAY_BUFFER, m_highlightVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * vertices.size(), vertices.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

GLuint GridRenderer::CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Shader compilation failed: %s", infoLog);
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

bool GridRenderer::LinkShaderProgram(GLuint vertexShader, GLuint fragmentShader) {
    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vertexShader);
    glAttachShader(m_shaderProgram, fragmentShader);
    glLinkProgram(m_shaderProgram);
    
    GLint success;
    glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &success);
    
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_shaderProgram, 512, nullptr, infoLog);
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Shader linking failed: %s", infoLog);
        glDeleteProgram(m_shaderProgram);
        m_shaderProgram = 0;
        return false;
    }
    
    return true;
}

void GridRenderer::CheckGLError(const char* operation) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "OpenGL error after %s: 0x%x", operation, error);
    }
}