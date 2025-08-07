#include "ShaderManager.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

ShaderManager::ShaderManager() 
    : m_hotReloadEnabled(false), m_shaderDirectory("shaders/") {
    CreateFallbackShader();
}

ShaderManager::~ShaderManager() {
    Clear();
}

bool ShaderManager::LoadAllShaders() {
    bool success = true;
    
    // Load core shaders
    success &= LoadShader("grid", 
                         m_shaderDirectory + "grid.vert", 
                         m_shaderDirectory + "grid.frag");
    
    success &= LoadShader("sprite", 
                         m_shaderDirectory + "sprite.vert", 
                         m_shaderDirectory + "sprite.frag");
    
    success &= LoadShader("line", 
                         m_shaderDirectory + "line.vert", 
                         m_shaderDirectory + "line.frag");
    
    success &= LoadShader("ui", 
                         m_shaderDirectory + "ui.vert", 
                         m_shaderDirectory + "ui.frag");
    
    if (!success) {
        std::cerr << "Warning: Some shaders failed to load\n";
    }
    
    return success;
}

bool ShaderManager::LoadShader(const std::string& name, 
                               const std::string& vertexPath, 
                               const std::string& fragmentPath) {
    auto shader = std::make_unique<ShaderProgram>();
    
    if (!shader->Load(vertexPath, fragmentPath)) {
        std::cerr << "Failed to load shader: " << name << std::endl;
        std::cerr << "  Vertex: " << vertexPath << std::endl;
        std::cerr << "  Fragment: " << fragmentPath << std::endl;
        return false;
    }
    
    // Store shader info
    ShaderInfo info;
    info.program = std::move(shader);
    info.vertexPath = vertexPath;
    info.fragmentPath = fragmentPath;
    info.lastModified = std::chrono::system_clock::now();
    
    m_shaders[name] = std::move(info);
    
    std::cout << "Loaded shader: " << name << std::endl;
    
    return true;
}

bool ShaderManager::LoadShaderFromSource(const std::string& name,
                                         const std::string& vertexSource,
                                         const std::string& fragmentSource) {
    auto shader = std::make_unique<ShaderProgram>();
    
    if (!shader->LoadFromSource(vertexSource, fragmentSource)) {
        std::cerr << "Failed to load shader from source: " << name << std::endl;
        return false;
    }
    
    // Store shader info (no file paths for source-based shaders)
    ShaderInfo info;
    info.program = std::move(shader);
    info.lastModified = std::chrono::system_clock::now();
    
    m_shaders[name] = std::move(info);
    
    std::cout << "Loaded shader from source: " << name << std::endl;
    
    return true;
}

ShaderProgram* ShaderManager::GetShader(const std::string& name) {
    auto it = m_shaders.find(name);
    if (it != m_shaders.end()) {
        return it->second.program.get();
    }
    
    // Return fallback shader if requested shader not found
    auto fallback = m_shaders.find("fallback");
    if (fallback != m_shaders.end()) {
        std::cerr << "Warning: Shader '" << name << "' not found, using fallback\n";
        return fallback->second.program.get();
    }
    
    return nullptr;
}

bool ShaderManager::HasShader(const std::string& name) const {
    return m_shaders.find(name) != m_shaders.end();
}

void ShaderManager::ReloadShaders() {
    std::cout << "Reloading all shaders...\n";
    
    for (auto& [name, info] : m_shaders) {
        // Skip shaders without file paths (loaded from source)
        if (info.vertexPath.empty() || info.fragmentPath.empty()) {
            continue;
        }
        
        ReloadShader(name);
    }
}

bool ShaderManager::ReloadShader(const std::string& name) {
    auto it = m_shaders.find(name);
    if (it == m_shaders.end()) {
        std::cerr << "Shader not found: " << name << std::endl;
        return false;
    }
    
    ShaderInfo& info = it->second;
    
    // Skip if no file paths
    if (info.vertexPath.empty() || info.fragmentPath.empty()) {
        std::cerr << "Cannot reload shader without file paths: " << name << std::endl;
        return false;
    }
    
    // Create new shader
    auto newShader = std::make_unique<ShaderProgram>();
    
    if (!newShader->Load(info.vertexPath, info.fragmentPath)) {
        std::cerr << "Failed to reload shader: " << name << std::endl;
        return false;
    }
    
    // Replace old shader
    info.program = std::move(newShader);
    info.lastModified = std::chrono::system_clock::now();
    
    std::cout << "Reloaded shader: " << name << std::endl;
    
    return true;
}

void ShaderManager::RemoveShader(const std::string& name) {
    m_shaders.erase(name);
}

void ShaderManager::Clear() {
    m_shaders.clear();
}

std::vector<std::string> ShaderManager::GetShaderNames() const {
    std::vector<std::string> names;
    names.reserve(m_shaders.size());
    
    for (const auto& [name, info] : m_shaders) {
        names.push_back(name);
    }
    
    return names;
}

void ShaderManager::CheckForModifiedShaders() {
    if (!m_hotReloadEnabled) return;
    
    for (auto& [name, info] : m_shaders) {
        // Skip shaders without file paths
        if (info.vertexPath.empty() || info.fragmentPath.empty()) {
            continue;
        }
        
        // Check if either file has been modified
        bool modified = IsFileModified(info.vertexPath, info.lastModified) ||
                       IsFileModified(info.fragmentPath, info.lastModified);
        
        if (modified) {
            std::cout << "Detected change in shader: " << name << std::endl;
            ReloadShader(name);
        }
    }
}

std::chrono::system_clock::time_point ShaderManager::GetFileModificationTime(const std::string& path) {
    try {
        auto ftime = fs::last_write_time(path);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
        );
        return sctp;
    } catch (const std::exception& e) {
        std::cerr << "Error getting file modification time: " << e.what() << std::endl;
        return std::chrono::system_clock::time_point();
    }
}

bool ShaderManager::IsFileModified(const std::string& path, 
                                   const std::chrono::system_clock::time_point& lastTime) {
    auto currentTime = GetFileModificationTime(path);
    return currentTime > lastTime;
}

void ShaderManager::CreateFallbackShader() {
    // Simple fallback shader for debugging
    const std::string fallbackVert = R"(
#version 330 core
layout(location = 0) in vec2 aPosition;
uniform mat4 uProjection;
uniform mat4 uView;
void main() {
    gl_Position = uProjection * uView * vec4(aPosition, 0.0, 1.0);
}
)";
    
    const std::string fallbackFrag = R"(
#version 330 core
out vec4 fragColor;
void main() {
    fragColor = vec4(1.0, 0.0, 1.0, 1.0); // Magenta for error visibility
}
)";
    
    LoadShaderFromSource("fallback", fallbackVert, fallbackFrag);
}