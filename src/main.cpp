#include "core/Application.h"
#include <iostream>
#include <SDL.h>

#ifdef _WIN32
    #include <windows.h>
    #ifdef main
        #undef main
    #endif
#endif

int main(int argc, char* argv[]) {
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
    
    AppConfig config;
    config.windowWidth = 1280;
    config.windowHeight = 720;
    config.windowTitle = "NOT Gate Game";
    config.fullscreen = false;
    config.vsync = true;
    config.targetFPS = 60;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--fullscreen") {
            config.fullscreen = true;
        } else if (arg == "--no-vsync") {
            config.vsync = false;
        } else if (arg == "--fps" && i + 1 < argc) {
            config.targetFPS = std::stoi(argv[++i]);
        } else if (arg == "--width" && i + 1 < argc) {
            config.windowWidth = std::stoi(argv[++i]);
        } else if (arg == "--height" && i + 1 < argc) {
            config.windowHeight = std::stoi(argv[++i]);
        }
    }
    
    {
        Application app;
        
        if (!app.initialize(config)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, 
                        "Failed to initialize application");
            return -1;
        }
        
        app.run();
    }
    
    SDL_Log("Program terminated successfully");
    return 0;
}