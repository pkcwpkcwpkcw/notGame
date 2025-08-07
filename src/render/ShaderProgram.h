#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <unordered_map>
#include <memory>

class ShaderProgram {
public:
    ShaderProgram();
    ~ShaderProgram();

    // Prevent copying
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    // Allow moving
    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    // Load and compile shaders
    bool Load(const std::string& vertexPath, const std::string& fragmentPath);
    bool LoadFromSource(const std::string& vertexSource, const std::string& fragmentSource);

    // Use this shader program
    void Use() const;
    
    // Get program ID
    GLuint GetProgram() const { return m_program; }
    
    // Check if shader is valid
    bool IsValid() const { return m_program != 0; }

    // Uniform setters
    void SetUniform(const std::string& name, bool value);
    void SetUniform(const std::string& name, int value);
    void SetUniform(const std::string& name, float value);
    void SetUniform(const std::string& name, const glm::vec2& value);
    void SetUniform(const std::string& name, const glm::vec3& value);
    void SetUniform(const std::string& name, const glm::vec4& value);
    void SetUniform(const std::string& name, const glm::mat3& value);
    void SetUniform(const std::string& name, const glm::mat4& value);

    // Array uniform setters
    void SetUniformArray(const std::string& name, const float* values, size_t count);
    void SetUniformArray(const std::string& name, const glm::vec2* values, size_t count);
    void SetUniformArray(const std::string& name, const glm::vec3* values, size_t count);
    void SetUniformArray(const std::string& name, const glm::vec4* values, size_t count);

    // Get uniform location (cached)
    GLint GetUniformLocation(const std::string& name);

    // Debug info
    void PrintActiveUniforms() const;
    void PrintActiveAttributes() const;

private:
    GLuint m_program;
    mutable std::unordered_map<std::string, GLint> m_uniformLocationCache;

    // Shader compilation helpers
    GLuint CompileShader(GLenum type, const std::string& source);
    bool LinkProgram(GLuint vertexShader, GLuint fragmentShader);
    
    // File loading
    std::string LoadShaderFile(const std::string& path);
    std::string ProcessIncludes(const std::string& source, const std::string& basePath);
    
    // Error handling
    std::string GetShaderInfoLog(GLuint shader);
    std::string GetProgramInfoLog(GLuint program);
    void ParseShaderError(const std::string& log, const std::string& source);

    // Cache all uniform locations
    void CacheUniformLocations();

    // Cleanup
    void Cleanup();
};