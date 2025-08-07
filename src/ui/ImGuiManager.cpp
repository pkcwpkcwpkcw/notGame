#include "ImGuiManager.h"
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <iostream>

ImGuiManager::~ImGuiManager() {
    if (m_initialized) {
        Shutdown();
    }
}

bool ImGuiManager::Initialize(SDL_Window* window, SDL_GLContext gl_context) {
    if (m_initialized) {
        return true;
    }

    m_window = window;
    m_glContext = gl_context;

    IMGUI_CHECKVERSION();
    m_context = ImGui::CreateContext();
    if (!m_context) {
        std::cerr << "Failed to create ImGui context" << std::endl;
        return false;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    
    // Docking is not available in the vcpkg version of ImGui
    // Will need to build from source with docking branch later
    m_dockingEnabled = false;

    ApplyDefaultTheme();

    // Load fonts before initializing backends
    if (!LoadFonts()) {
        SDL_Log("Warning: Failed to load custom fonts, using default");
    }
    
    if (!ImGui_ImplSDL2_InitForOpenGL(window, gl_context)) {
        std::cerr << "Failed to initialize ImGui SDL2 backend" << std::endl;
        ImGui::DestroyContext(m_context);
        m_context = nullptr;
        return false;
    }

    const char* glsl_version = "#version 330";
    if (!ImGui_ImplOpenGL3_Init(glsl_version)) {
        std::cerr << "Failed to initialize ImGui OpenGL3 backend" << std::endl;
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext(m_context);
        m_context = nullptr;
        return false;
    }

    m_initialized = true;
    return true;
}

void ImGuiManager::Shutdown() {
    if (!m_initialized) {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    
    if (m_context) {
        ImGui::DestroyContext(m_context);
        m_context = nullptr;
    }

    m_initialized = false;
}

void ImGuiManager::BeginFrame() {
    if (!m_initialized) {
        return;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    if (m_dockingEnabled) {
        SetupDockSpace();
    }

    if (m_showDemo) {
        ImGui::ShowDemoWindow(&m_showDemo);
    }
}

void ImGuiManager::EndFrame() {
    if (!m_initialized) {
        return;
    }

    ImGui::Render();
    
    // Don't override the viewport - let the application manage it
    // ImGuiIO& io = ImGui::GetIO();
    // glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiManager::ProcessEvent(const SDL_Event& event) {
    if (!m_initialized) {
        return;
    }

    ImGui_ImplSDL2_ProcessEvent(&event);
}

bool ImGuiManager::WantCaptureMouse() const {
    if (!m_initialized) {
        return false;
    }

    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureMouse;
}

bool ImGuiManager::WantCaptureKeyboard() const {
    if (!m_initialized) {
        return false;
    }

    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureKeyboard;
}

bool ImGuiManager::LoadFonts() {
    ImGuiIO& io = ImGui::GetIO();
    
    // Clear any previous font data
    io.Fonts->Clear();
    
    // Just use the default font - don't try to load custom fonts
    io.Fonts->AddFontDefault();
    
    SDL_Log("Using default ImGui font");
    
    // Don't manually build font atlas - let the backend handle it
    // The OpenGL3 backend will build it automatically when needed
    
    return true;
}

void ImGuiManager::ApplyDefaultTheme() {
    ImGui::StyleColorsDark();
    
    ImGuiStyle& style = ImGui::GetStyle();
    
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 3.0f;
    
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Right;
    
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 0.94f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    
    style.Colors[ImGuiCol_Button] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    
    style.Colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.0f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 0.51f);
}

void ImGuiManager::SetupDockSpace() {
    // Docking not available in vcpkg version
    // This function is kept for future implementation
    // when we build ImGui from source with docking branch
}