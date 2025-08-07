#pragma once

#include "ShaderProgram.h"
#include <memory>
#include <unordered_map>
#include <string>
#include <chrono>

class ShaderManager {
public:
    ShaderManager();
    ~ShaderManager();

    // Load all shaders
    bool LoadAllShaders();
    
    // Load individual shader
    bool LoadShader(const std::string& name, 
                   const std::string& vertexPath, 
                   const std::string& fragmentPath);
    
    // Load shader from source
    bool LoadShaderFromSource(const std::string& name,
                             const std::string& vertexSource,
                             const std::string& fragmentSource);

    // Get shader by name
    ShaderProgram* GetShader(const std::string& name);
    
    // Check if shader exists
    bool HasShader(const std::string& name) const;
    
    // Reload all shaders (for hot reloading)
    void ReloadShaders();
    
    // Reload specific shader
    bool ReloadShader(const std::string& name);
    
    // Remove shader
    void RemoveShader(const std::string& name);
    
    // Clear all shaders
    void Clear();

    // Get list of loaded shaders
    std::vector<std::string> GetShaderNames() const;

    // Enable/disable hot reloading
    void SetHotReloadEnabled(bool enabled) { m_hotReloadEnabled = enabled; }
    bool IsHotReloadEnabled() const { return m_hotReloadEnabled; }
    
    // Check for modified shaders (call periodically for hot reload)
    void CheckForModifiedShaders();

private:
    struct ShaderInfo {
        std::unique_ptr<ShaderProgram> program;
        std::string vertexPath;
        std::string fragmentPath;
        std::chrono::system_clock::time_point lastModified;
    };

    std::unordered_map<std::string, ShaderInfo> m_shaders;
    bool m_hotReloadEnabled;
    
    // Default shader paths
    std::string m_shaderDirectory;
    
    // Helper functions
    std::chrono::system_clock::time_point GetFileModificationTime(const std::string& path);
    bool IsFileModified(const std::string& path, const std::chrono::system_clock::time_point& lastTime);
    
    // Create default fallback shader
    void CreateFallbackShader();
};