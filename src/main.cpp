#include "core/Application.h"
#include "utils/Logger.h"
#include <iostream>
#include <cstdlib>

int main(int /*argc*/, char* /*argv*/[]) {
    std::cout << "Starting NOT Gate Sandbox..." << std::endl;
    
    // Initialize logger
    Logger::Initialize("notgate.log");
    Logger::SetMinLevel(LogLevel::DEBUG);
    
    std::cout << "Logger initialized" << std::endl;
    
    Logger::Info("=================================");
    Logger::Info("   NOT Gate Sandbox v0.1.0      ");
    Logger::Info("=================================");
    Logger::Info("Build: " + std::string(__DATE__) + " " + std::string(__TIME__));
    
    std::cout << "Creating application..." << std::endl;
    
    // Create and run application
    Application app;
    
    std::cout << "Application created, initializing..." << std::endl;
    
    if (!app.Initialize()) {
        Logger::Critical("Failed to initialize application");
        Logger::Shutdown();
        return EXIT_FAILURE;
    }
    
    app.Run();
    app.Shutdown();
    
    Logger::Info("Application terminated successfully");
    Logger::Shutdown();
    
    return EXIT_SUCCESS;
}