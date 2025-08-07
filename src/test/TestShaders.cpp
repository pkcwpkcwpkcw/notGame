#include "../render/ShaderManager.h"
#include "../render/ShaderProgram.h"
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>

class ShaderTest {
private:
    SDL_Window* m_window;
    SDL_GLContext m_context;
    ShaderManager m_shaderManager;
    
    GLuint m_gridVAO, m_gridVBO;
    GLuint m_spriteVAO, m_spriteVBO, m_spriteEBO;
    
    glm::mat4 m_projection;
    glm::mat4 m_view;
    glm::vec2 m_cameraPos;
    float m_zoom;
    float m_time;

public:
    ShaderTest() : m_window(nullptr), m_context(nullptr), 
                   m_cameraPos(0.0f), m_zoom(1.0f), m_time(0.0f) {}
    
    ~ShaderTest() {
        Cleanup();
    }
    
    bool Initialize() {
        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // Set OpenGL attributes
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        
        // Create window
        m_window = SDL_CreateWindow(
            "Shader Test - NOT Gate Game",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            1280, 720,
            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
        );
        
        if (!m_window) {
            std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // Create OpenGL context
        m_context = SDL_GL_CreateContext(m_window);
        if (!m_context) {
            std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // Initialize GLAD
        if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
            std::cerr << "Failed to initialize GLAD" << std::endl;
            return false;
        }
        
        // Set up OpenGL state
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        
        // Print OpenGL info
        std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
        std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
        std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
        
        // Load shaders
        if (!m_shaderManager.LoadAllShaders()) {
            std::cerr << "Failed to load shaders" << std::endl;
            return false;
        }
        
        // Enable hot reload in debug mode
        #ifdef DEBUG
        m_shaderManager.SetHotReloadEnabled(true);
        #endif
        
        // Set up geometry
        SetupGeometry();
        
        // Set up matrices
        int width, height;
        SDL_GetWindowSize(m_window, &width, &height);
        float aspect = (float)width / (float)height;
        m_projection = glm::ortho(-aspect * 10.0f, aspect * 10.0f, -10.0f, 10.0f);
        m_view = glm::mat4(1.0f);
        
        return true;
    }
    
    void SetupGeometry() {
        // Grid quad (fullscreen)
        float gridVertices[] = {
            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f
        };
        
        glGenVertexArrays(1, &m_gridVAO);
        glGenBuffers(1, &m_gridVBO);
        
        glBindVertexArray(m_gridVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_gridVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(gridVertices), gridVertices, GL_STATIC_DRAW);
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        
        // Sprite quad
        float spriteVertices[] = {
            // Position    TexCoord
            -0.5f, -0.5f,  0.0f, 0.0f,
             0.5f, -0.5f,  1.0f, 0.0f,
             0.5f,  0.5f,  1.0f, 1.0f,
            -0.5f,  0.5f,  0.0f, 1.0f
        };
        
        unsigned int spriteIndices[] = {
            0, 1, 2,
            2, 3, 0
        };
        
        glGenVertexArrays(1, &m_spriteVAO);
        glGenBuffers(1, &m_spriteVBO);
        glGenBuffers(1, &m_spriteEBO);
        
        glBindVertexArray(m_spriteVAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, m_spriteVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(spriteVertices), spriteVertices, GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_spriteEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(spriteIndices), spriteIndices, GL_STATIC_DRAW);
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        
        glBindVertexArray(0);
    }
    
    void Run() {
        bool running = true;
        SDL_Event event;
        
        Uint32 lastTime = SDL_GetTicks();
        
        while (running) {
            // Handle events
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                } else if (event.type == SDL_KEYDOWN) {
                    HandleKeyPress(event.key.keysym.sym);
                } else if (event.type == SDL_MOUSEWHEEL) {
                    HandleMouseWheel(event.wheel.y);
                }
            }
            
            // Update time
            Uint32 currentTime = SDL_GetTicks();
            float deltaTime = (currentTime - lastTime) / 1000.0f;
            lastTime = currentTime;
            m_time += deltaTime;
            
            // Check for shader modifications
            m_shaderManager.CheckForModifiedShaders();
            
            // Render
            Render();
            
            SDL_GL_SwapWindow(m_window);
            SDL_Delay(16); // ~60 FPS
        }
    }
    
    void Render() {
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Calculate inverse view-projection for grid shader
        glm::mat4 viewProj = m_projection * m_view;
        glm::mat4 invViewProj = glm::inverse(viewProj);
        
        // Render grid
        ShaderProgram* gridShader = m_shaderManager.GetShader("grid");
        if (gridShader && gridShader->IsValid()) {
            gridShader->Use();
            gridShader->SetUniform("uProjection", m_projection);
            gridShader->SetUniform("uView", m_view);
            gridShader->SetUniform("uInvViewProj", invViewProj);
            gridShader->SetUniform("uGridSize", 1.0f);
            gridShader->SetUniform("uGridColor", glm::vec4(0.5f, 0.5f, 0.5f, 0.5f));
            gridShader->SetUniform("uSubGridColor", glm::vec4(0.3f, 0.3f, 0.3f, 0.3f));
            gridShader->SetUniform("uCameraPos", m_cameraPos);
            gridShader->SetUniform("uZoom", m_zoom);
            
            glBindVertexArray(m_gridVAO);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        }
        
        // Render test sprites (NOT gates)
        ShaderProgram* spriteShader = m_shaderManager.GetShader("sprite");
        if (spriteShader && spriteShader->IsValid()) {
            spriteShader->Use();
            spriteShader->SetUniform("uProjection", m_projection);
            spriteShader->SetUniform("uView", m_view);
            spriteShader->SetUniform("uModel", glm::mat4(1.0f));
            spriteShader->SetUniform("uScale", glm::vec2(1.0f));
            spriteShader->SetUniform("uTintColor", glm::vec4(1.0f));
            spriteShader->SetUniform("uUseTexture", false);
            spriteShader->SetUniform("uUseInstancing", false);
            
            // Draw a few test sprites
            glBindVertexArray(m_spriteVAO);
            
            for (int i = -2; i <= 2; i++) {
                for (int j = -2; j <= 2; j++) {
                    glm::vec2 pos(i * 2.0f, j * 2.0f);
                    float rotation = m_time + (i + j) * 0.5f;
                    
                    spriteShader->SetUniform("uPosition", pos);
                    spriteShader->SetUniform("uRotation", rotation);
                    spriteShader->SetUniform("uSelected", (i == 0 && j == 0) ? 1.0f : 0.0f);
                    spriteShader->SetUniform("uActive", sin(m_time * 2.0f) * 0.5f + 0.5f);
                    
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                }
            }
        }
        
        // Test line shader
        TestLineShader();
        
        glBindVertexArray(0);
    }
    
    void TestLineShader() {
        ShaderProgram* lineShader = m_shaderManager.GetShader("line");
        if (!lineShader || !lineShader->IsValid()) return;
        
        // Create line geometry on the fly
        GLuint lineVAO, lineVBO;
        glGenVertexArrays(1, &lineVAO);
        glGenBuffers(1, &lineVBO);
        
        // Line strip vertices
        float lineVertices[] = {
            // Position  Side  TexCoord
            0.0f,       -1.0f,  0.0f,
            0.0f,        1.0f,  0.0f,
            1.0f,        1.0f,  1.0f,
            1.0f,       -1.0f,  1.0f
        };
        
        glBindVertexArray(lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), lineVertices, GL_DYNAMIC_DRAW);
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(sizeof(float)));
        
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(2 * sizeof(float)));
        
        lineShader->Use();
        lineShader->SetUniform("uProjection", m_projection);
        lineShader->SetUniform("uView", m_view);
        lineShader->SetUniform("uLineThickness", 0.1f);
        lineShader->SetUniform("uSignalOnColor", glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
        lineShader->SetUniform("uSignalOffColor", glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));
        lineShader->SetUniform("uTime", m_time);
        lineShader->SetUniform("uFlowSpeed", 2.0f);
        lineShader->SetUniform("uSelected", false);
        
        // Draw test lines
        for (int i = 0; i < 3; i++) {
            glm::vec2 start(-5.0f, -3.0f + i * 3.0f);
            glm::vec2 end(5.0f, -3.0f + i * 3.0f);
            
            lineShader->SetUniform("uStartPos", start);
            lineShader->SetUniform("uEndPos", end);
            lineShader->SetUniform("uSignalState", (float)(i % 2));
            
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
        
        // Cleanup
        glDeleteVertexArrays(1, &lineVAO);
        glDeleteBuffers(1, &lineVBO);
    }
    
    void HandleKeyPress(SDL_Keycode key) {
        const float moveSpeed = 1.0f;
        
        switch(key) {
            case SDLK_w:
                m_cameraPos.y += moveSpeed;
                break;
            case SDLK_s:
                m_cameraPos.y -= moveSpeed;
                break;
            case SDLK_a:
                m_cameraPos.x -= moveSpeed;
                break;
            case SDLK_d:
                m_cameraPos.x += moveSpeed;
                break;
            case SDLK_r:
                std::cout << "Reloading shaders..." << std::endl;
                m_shaderManager.ReloadShaders();
                break;
            case SDLK_ESCAPE:
                SDL_Event quit;
                quit.type = SDL_QUIT;
                SDL_PushEvent(&quit);
                break;
        }
        
        // Update view matrix
        m_view = glm::translate(glm::mat4(1.0f), glm::vec3(-m_cameraPos, 0.0f));
    }
    
    void HandleMouseWheel(int y) {
        if (y > 0) {
            m_zoom *= 1.2f;
        } else if (y < 0) {
            m_zoom /= 1.2f;
        }
        
        m_zoom = glm::clamp(m_zoom, 0.1f, 10.0f);
        
        // Update projection for zoom
        float aspect = 1280.0f / 720.0f;
        float size = 10.0f / m_zoom;
        m_projection = glm::ortho(-aspect * size, aspect * size, -size, size);
    }
    
    void Cleanup() {
        if (m_gridVAO) {
            glDeleteVertexArrays(1, &m_gridVAO);
            glDeleteBuffers(1, &m_gridVBO);
        }
        
        if (m_spriteVAO) {
            glDeleteVertexArrays(1, &m_spriteVAO);
            glDeleteBuffers(1, &m_spriteVBO);
            glDeleteBuffers(1, &m_spriteEBO);
        }
        
        if (m_context) {
            SDL_GL_DeleteContext(m_context);
        }
        
        if (m_window) {
            SDL_DestroyWindow(m_window);
        }
        
        SDL_Quit();
    }
};

int main(int argc, char* argv[]) {
    ShaderTest test;
    
    if (!test.Initialize()) {
        std::cerr << "Failed to initialize shader test" << std::endl;
        return -1;
    }
    
    std::cout << "\n=== Shader Test Controls ===" << std::endl;
    std::cout << "WASD: Move camera" << std::endl;
    std::cout << "Mouse Wheel: Zoom in/out" << std::endl;
    std::cout << "R: Reload shaders" << std::endl;
    std::cout << "ESC: Exit" << std::endl;
    std::cout << "===========================\n" << std::endl;
    
    test.Run();
    
    return 0;
}