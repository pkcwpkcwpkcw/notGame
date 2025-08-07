#include "../render/ShaderManager.h"
#include "../render/ShaderProgram.h"
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <iostream>

int main(int argc, char* argv[]) {
    // Initialize SDL with video subsystem only
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return -1;
    }
    
    // Set OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    
    // Create hidden window for OpenGL context
    SDL_Window* window = SDL_CreateWindow(
        "Shader Compile Test",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        1, 1,
        SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN
    );
    
    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }
    
    // Create OpenGL context
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) {
        std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    
    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    
    // Print OpenGL info
    std::cout << "=== OpenGL Information ===" << std::endl;
    std::cout << "Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << std::endl;
    
    // Test shader compilation
    std::cout << "=== Shader Compilation Test ===" << std::endl;
    
    ShaderManager shaderManager;
    
    // Test loading each shader
    bool allShadersLoaded = true;
    
    std::cout << "Loading grid shader... ";
    if (shaderManager.LoadShader("grid", "shaders/grid.vert", "shaders/grid.frag")) {
        std::cout << "SUCCESS" << std::endl;
        
        // Print active uniforms
        ShaderProgram* gridShader = shaderManager.GetShader("grid");
        if (gridShader) {
            std::cout << "  Grid shader uniforms:" << std::endl;
            gridShader->PrintActiveUniforms();
        }
    } else {
        std::cout << "FAILED" << std::endl;
        allShadersLoaded = false;
    }
    
    std::cout << "Loading sprite shader... ";
    if (shaderManager.LoadShader("sprite", "shaders/sprite.vert", "shaders/sprite.frag")) {
        std::cout << "SUCCESS" << std::endl;
        
        ShaderProgram* spriteShader = shaderManager.GetShader("sprite");
        if (spriteShader) {
            std::cout << "  Sprite shader uniforms:" << std::endl;
            spriteShader->PrintActiveUniforms();
        }
    } else {
        std::cout << "FAILED" << std::endl;
        allShadersLoaded = false;
    }
    
    std::cout << "Loading line shader... ";
    if (shaderManager.LoadShader("line", "shaders/line.vert", "shaders/line.frag")) {
        std::cout << "SUCCESS" << std::endl;
        
        ShaderProgram* lineShader = shaderManager.GetShader("line");
        if (lineShader) {
            std::cout << "  Line shader uniforms:" << std::endl;
            lineShader->PrintActiveUniforms();
        }
    } else {
        std::cout << "FAILED" << std::endl;
        allShadersLoaded = false;
    }
    
    std::cout << "Loading UI shader... ";
    if (shaderManager.LoadShader("ui", "shaders/ui.vert", "shaders/ui.frag")) {
        std::cout << "SUCCESS" << std::endl;
        
        ShaderProgram* uiShader = shaderManager.GetShader("ui");
        if (uiShader) {
            std::cout << "  UI shader uniforms:" << std::endl;
            uiShader->PrintActiveUniforms();
        }
    } else {
        std::cout << "FAILED" << std::endl;
        allShadersLoaded = false;
    }
    
    std::cout << std::endl;
    
    // Summary
    if (allShadersLoaded) {
        std::cout << "=== SUCCESS: All shaders compiled successfully! ===" << std::endl;
    } else {
        std::cout << "=== FAILURE: Some shaders failed to compile ===" << std::endl;
    }
    
    // Test shader reload
    std::cout << "\n=== Testing Shader Reload ===" << std::endl;
    if (shaderManager.ReloadShader("grid")) {
        std::cout << "Grid shader reload: SUCCESS" << std::endl;
    } else {
        std::cout << "Grid shader reload: FAILED" << std::endl;
    }
    
    // List all loaded shaders
    std::cout << "\n=== Loaded Shaders ===" << std::endl;
    auto shaderNames = shaderManager.GetShaderNames();
    for (const auto& name : shaderNames) {
        std::cout << "  - " << name << std::endl;
    }
    
    // Cleanup
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return allShadersLoaded ? 0 : -1;
}