#pragma once

#include <SDL.h>
#include <glad/glad.h>
#include <imgui.h>
#include <memory>

class ImGuiManager {
public:
    ImGuiManager() = default;
    ~ImGuiManager();

    bool Initialize(SDL_Window* window, SDL_GLContext gl_context);
    void Shutdown();

    void BeginFrame();
    void EndFrame();
    void ProcessEvent(const SDL_Event& event);

    bool WantCaptureMouse() const;
    bool WantCaptureKeyboard() const;

    void ShowDemoWindow() { m_showDemo = true; }
    void HideDemoWindow() { m_showDemo = false; }

private:
    bool LoadFonts();
    void ApplyDefaultTheme();
    void SetupDockSpace();

private:
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext = nullptr;
    ImGuiContext* m_context = nullptr;
    
    bool m_initialized = false;
    bool m_showDemo = false;
    bool m_dockingEnabled = false;
};