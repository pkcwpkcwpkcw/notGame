#include <SDL.h>
#include <glad/glad.h>
#include <iostream>
#include <memory>
#include "render/Window.h"
#include "game/DemoScene.h"

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return -1;
    }
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    
    auto window = std::make_unique<Window>();
    if (!window->Initialize("NOT Gate Render Test", 1280, 720, false)) {
        std::cerr << "Failed to create window!" << std::endl;
        SDL_Quit();
        return -1;
    }
    
    SDL_GLContext glContext = SDL_GL_CreateContext(window->GetSDLWindow());
    if (!glContext) {
        std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }
    
    SDL_GL_SetSwapInterval(1);
    
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        SDL_GL_DeleteContext(glContext);
        SDL_Quit();
        return -1;
    }
    
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    
    auto scene = std::make_unique<DemoScene>();
    if (!scene->Initialize(window.get())) {
        std::cerr << "Failed to initialize demo scene!" << std::endl;
        SDL_GL_DeleteContext(glContext);
        SDL_Quit();
        return -1;
    }
    
    std::cout << "\n=== NOT Gate Render System Test ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  Mouse Wheel: Zoom in/out" << std::endl;
    std::cout << "  Middle Mouse + Drag: Pan camera" << std::endl;
    std::cout << "  Left Mouse + Drag: Draw wire" << std::endl;
    std::cout << "  R: Reset camera" << std::endl;
    std::cout << "  G: Toggle grid" << std::endl;
    std::cout << "  Space: Recreate demo circuit" << std::endl;
    std::cout << "  ESC: Exit" << std::endl;
    std::cout << "===================================\n" << std::endl;
    
    bool running = true;
    SDL_Event event;
    Uint32 lastTime = SDL_GetTicks();
    
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            } else {
                scene->HandleInput(event);
            }
        }
        
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;
        
        scene->Update(deltaTime);
        scene->Render();
        
        SDL_GL_SwapWindow(window->GetSDLWindow());
    }
    
    scene.reset();
    SDL_GL_DeleteContext(glContext);
    window.reset();
    SDL_Quit();
    
    std::cout << "Test completed successfully!" << std::endl;
    return 0;
}