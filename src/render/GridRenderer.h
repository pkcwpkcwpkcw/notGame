#ifndef GRID_RENDERER_H
#define GRID_RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>

class Camera;

class GridRenderer {
public:
    GridRenderer();
    ~GridRenderer();
    
    bool Initialize(int screenWidth, int screenHeight);
    void Shutdown();
    
    void Render(const Camera& camera);
    void RenderGrid();
    void RenderHighlights();
    
    void SetGridVisible(bool visible) { m_isGridVisible = visible; }
    void SetGridOpacity(float opacity) { m_gridOpacity = opacity; }
    void SetCellSize(float size) { m_cellSize = size; }
    void SetGridSize(float size) { m_cellSize = size; }
    void SetGridColor(const glm::vec4& color) { /* TODO: implement */ }
    bool IsVisible() const { return m_isGridVisible; }
    
    void SetHoveredCell(const glm::ivec2& cell);
    void SetSelectedCells(const std::vector<glm::ivec2>& cells);
    void ClearSelection();
    
    void OnResize(int width, int height);
    
    bool IsGridVisible() const { return m_isGridVisible; }
    float GetGridOpacity() const { return m_gridOpacity; }
    float GetCellSize() const { return m_cellSize; }
    
private:
    GLuint m_gridVAO;
    GLuint m_gridVBO;
    GLuint m_highlightVAO;
    GLuint m_highlightVBO;
    GLuint m_shaderProgram;
    
    GLint m_viewProjMatrixLoc;
    GLint m_gridColorLoc;
    GLint m_gridOpacityLoc;
    GLint m_cellSizeLoc;
    
    bool m_isGridVisible;
    float m_gridOpacity;
    float m_cellSize;
    
    glm::ivec2 m_hoveredCell;
    std::vector<glm::ivec2> m_selectedCells;
    
    int m_screenWidth;
    int m_screenHeight;
    
    size_t m_gridVertexCount;
    size_t m_highlightVertexCount;
    
    void CreateGridMesh();
    void CreateHighlightMesh();
    bool CompileShaders();
    void UpdateGridBuffer(const Camera& camera);
    void UpdateHighlightBuffer();
    
    GLuint CompileShader(GLenum type, const char* source);
    bool LinkShaderProgram(GLuint vertexShader, GLuint fragmentShader);
    void CheckGLError(const char* operation);
};

#endif // GRID_RENDERER_H