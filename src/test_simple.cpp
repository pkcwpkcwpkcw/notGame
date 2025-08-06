#include <iostream>
#include <SDL.h>

int main(int argc, char* argv[]) {
    std::cout << "Test 1: Starting SDL2 test" << std::endl;
    
    // Initialize SDL
    std::cout << "Test 2: Calling SDL_Init..." << std::endl;
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return -1;
    }
    
    std::cout << "Test 3: SDL initialized successfully" << std::endl;
    
    // Create window
    SDL_Window* window = SDL_CreateWindow(
        "Simple SDL Test",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        640, 480,
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }
    
    std::cout << "Test 4: Window created successfully" << std::endl;
    std::cout << "Press Enter to close..." << std::endl;
    std::cin.get();
    
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    std::cout << "Test 5: Cleanup complete" << std::endl;
    return 0;
}