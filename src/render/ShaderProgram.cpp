#include "ShaderProgram.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>
#include <filesystem>

namespace fs = std::filesystem;

ShaderProgram::ShaderProgram() : m_program(0) {}

ShaderProgram::~ShaderProgram() {
    Cleanup();
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
    : m_program(other.m_program), m_uniformLocationCache(std::move(other.m_uniformLocationCache)) {
    other.m_program = 0;
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
    if (this != &other) {
        Cleanup();
        m_program = other.m_program;
        m_uniformLocationCache = std::move(other.m_uniformLocationCache);
        other.m_program = 0;
    }
    return *this;
}

bool ShaderProgram::Load(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexSource = LoadShaderFile(vertexPath);
    std::string fragmentSource = LoadShaderFile(fragmentPath);
    
    if (vertexSource.empty() || fragmentSource.empty()) {
        return false;
    }

    // Process includes
    fs::path vertBasePath = fs::path(vertexPath).parent_path();
    fs::path fragBasePath = fs::path(fragmentPath).parent_path();
    
    vertexSource = ProcessIncludes(vertexSource, vertBasePath.string());
    fragmentSource = ProcessIncludes(fragmentSource, fragBasePath.string());

    return LoadFromSource(vertexSource, fragmentSource);
}

bool ShaderProgram::LoadFromSource(const std::string& vertexSource, const std::string& fragmentSource) {
    GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSource);
    if (!vertexShader) {
        return false;
    }

    GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (!fragmentShader) {
        glDeleteShader(vertexShader);
        return false;
    }

    if (!LinkProgram(vertexShader, fragmentShader)) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    // Shaders can be deleted after linking
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Cache uniform locations
    CacheUniformLocations();

    return true;
}

void ShaderProgram::Use() const {
    glUseProgram(m_program);
}

GLuint ShaderProgram::CompileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* sourceCStr = source.c_str();
    glShaderSource(shader, 1, &sourceCStr, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        std::string log = GetShaderInfoLog(shader);
        std::cerr << "Shader compilation failed:\n" << log << std::endl;
        ParseShaderError(log, source);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

bool ShaderProgram::LinkProgram(GLuint vertexShader, GLuint fragmentShader) {
    m_program = glCreateProgram();
    
    glAttachShader(m_program, vertexShader);
    glAttachShader(m_program, fragmentShader);
    
    // Bind default attribute locations
    glBindAttribLocation(m_program, 0, "aPosition");
    glBindAttribLocation(m_program, 1, "aTexCoord");
    glBindAttribLocation(m_program, 2, "aColor");
    
    // Bind fragment output
    glBindFragDataLocation(m_program, 0, "fragColor");
    
    glLinkProgram(m_program);

    GLint success;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);

    if (!success) {
        std::string log = GetProgramInfoLog(m_program);
        std::cerr << "Shader linking failed:\n" << log << std::endl;
        glDeleteProgram(m_program);
        m_program = 0;
        return false;
    }

    // Validate program (debug only)
    #ifdef DEBUG
    glValidateProgram(m_program);
    glGetProgramiv(m_program, GL_VALIDATE_STATUS, &success);
    if (!success) {
        std::string log = GetProgramInfoLog(m_program);
        std::cerr << "Shader validation warning:\n" << log << std::endl;
    }
    #endif

    return true;
}

std::string ShaderProgram::LoadShaderFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << path << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string ShaderProgram::ProcessIncludes(const std::string& source, const std::string& basePath) {
    std::string result = source;
    std::regex includeRegex("#include\\s+\"([^\"]+)\"");
    std::smatch match;

    size_t maxIterations = 10; // Prevent infinite recursion
    size_t iteration = 0;

    while (iteration < maxIterations && std::regex_search(result, match, includeRegex)) {
        std::string includePath = match[1];
        
        // Resolve path relative to base
        fs::path fullPath = fs::path(basePath) / includePath;
        
        std::string includeContent = LoadShaderFile(fullPath.string());
        if (includeContent.empty()) {
            std::cerr << "Failed to include file: " << fullPath << std::endl;
            includeContent = "// Failed to include: " + includePath;
        }

        // Replace include directive with file content
        result.replace(match.position(), match.length(), includeContent);
        iteration++;
    }

    return result;
}

std::string ShaderProgram::GetShaderInfoLog(GLuint shader) {
    GLint logLength;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    
    if (logLength > 0) {
        std::vector<char> log(logLength);
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        return std::string(log.data());
    }
    
    return "";
}

std::string ShaderProgram::GetProgramInfoLog(GLuint program) {
    GLint logLength;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    
    if (logLength > 0) {
        std::vector<char> log(logLength);
        glGetProgramInfoLog(program, logLength, nullptr, log.data());
        return std::string(log.data());
    }
    
    return "";
}

void ShaderProgram::ParseShaderError(const std::string& log, const std::string& source) {
    // Parse error format from different vendors
    // NVIDIA: "0(15) : error C0000: syntax error"
    // AMD: "ERROR: 0:15: syntax error"
    // Intel: "ERROR: 0:15: '' : syntax error"
    
    std::regex nvidiaRegex(R"((\d+)\((\d+)\)\s*:\s*(.+))");
    std::regex amdRegex(R"(ERROR:\s*(\d+):(\d+):\s*(.+))");
    
    std::smatch match;
    int lineNumber = -1;
    std::string errorMsg;
    
    if (std::regex_search(log, match, nvidiaRegex)) {
        lineNumber = std::stoi(match[2]);
        errorMsg = match[3];
    } else if (std::regex_search(log, match, amdRegex)) {
        lineNumber = std::stoi(match[2]);
        errorMsg = match[3];
    }
    
    if (lineNumber > 0) {
        // Print source lines around error
        std::istringstream sourceStream(source);
        std::string line;
        int currentLine = 1;
        
        std::cerr << "\nShader source around error (line " << lineNumber << "):\n";
        while (std::getline(sourceStream, line)) {
            if (currentLine >= lineNumber - 2 && currentLine <= lineNumber + 2) {
                if (currentLine == lineNumber) {
                    std::cerr << ">>> ";
                } else {
                    std::cerr << "    ";
                }
                std::cerr << currentLine << ": " << line << "\n";
            }
            currentLine++;
        }
        std::cerr << "Error: " << errorMsg << "\n";
    }
}

void ShaderProgram::CacheUniformLocations() {
    if (!m_program) return;

    m_uniformLocationCache.clear();

    GLint uniformCount;
    glGetProgramiv(m_program, GL_ACTIVE_UNIFORMS, &uniformCount);

    for (GLint i = 0; i < uniformCount; i++) {
        char name[256];
        GLsizei length;
        GLint size;
        GLenum type;
        
        glGetActiveUniform(m_program, i, sizeof(name), &length, &size, &type, name);
        
        // Remove array brackets if present
        char* bracket = strchr(name, '[');
        if (bracket) *bracket = '\0';
        
        GLint location = glGetUniformLocation(m_program, name);
        if (location != -1) {
            m_uniformLocationCache[name] = location;
        }
    }
}

GLint ShaderProgram::GetUniformLocation(const std::string& name) {
    auto it = m_uniformLocationCache.find(name);
    if (it != m_uniformLocationCache.end()) {
        return it->second;
    }

    // Not in cache, try to get it
    GLint location = glGetUniformLocation(m_program, name.c_str());
    if (location != -1) {
        m_uniformLocationCache[name] = location;
    }
    
    return location;
}

// Uniform setters
void ShaderProgram::SetUniform(const std::string& name, bool value) {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
        glUniform1i(location, value ? 1 : 0);
    }
}

void ShaderProgram::SetUniform(const std::string& name, int value) {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
        glUniform1i(location, value);
    }
}

void ShaderProgram::SetUniform(const std::string& name, float value) {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
        glUniform1f(location, value);
    }
}

void ShaderProgram::SetUniform(const std::string& name, const glm::vec2& value) {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
        glUniform2fv(location, 1, glm::value_ptr(value));
    }
}

void ShaderProgram::SetUniform(const std::string& name, const glm::vec3& value) {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
        glUniform3fv(location, 1, glm::value_ptr(value));
    }
}

void ShaderProgram::SetUniform(const std::string& name, const glm::vec4& value) {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
        glUniform4fv(location, 1, glm::value_ptr(value));
    }
}

void ShaderProgram::SetUniform(const std::string& name, const glm::mat3& value) {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
        glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }
}

void ShaderProgram::SetUniform(const std::string& name, const glm::mat4& value) {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }
}

// Array uniform setters
void ShaderProgram::SetUniformArray(const std::string& name, const float* values, size_t count) {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
        glUniform1fv(location, static_cast<GLsizei>(count), values);
    }
}

void ShaderProgram::SetUniformArray(const std::string& name, const glm::vec2* values, size_t count) {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
        glUniform2fv(location, static_cast<GLsizei>(count), reinterpret_cast<const float*>(values));
    }
}

void ShaderProgram::SetUniformArray(const std::string& name, const glm::vec3* values, size_t count) {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
        glUniform3fv(location, static_cast<GLsizei>(count), reinterpret_cast<const float*>(values));
    }
}

void ShaderProgram::SetUniformArray(const std::string& name, const glm::vec4* values, size_t count) {
    GLint location = GetUniformLocation(name);
    if (location != -1) {
        glUniform4fv(location, static_cast<GLsizei>(count), reinterpret_cast<const float*>(values));
    }
}

void ShaderProgram::PrintActiveUniforms() const {
    if (!m_program) return;

    GLint uniformCount;
    glGetProgramiv(m_program, GL_ACTIVE_UNIFORMS, &uniformCount);

    std::cout << "Active uniforms (" << uniformCount << "):\n";
    for (GLint i = 0; i < uniformCount; i++) {
        char name[256];
        GLsizei length;
        GLint size;
        GLenum type;
        
        glGetActiveUniform(m_program, i, sizeof(name), &length, &size, &type, name);
        GLint location = glGetUniformLocation(m_program, name);
        
        std::cout << "  " << name << " (location: " << location << ", size: " << size << ")\n";
    }
}

void ShaderProgram::PrintActiveAttributes() const {
    if (!m_program) return;

    GLint attribCount;
    glGetProgramiv(m_program, GL_ACTIVE_ATTRIBUTES, &attribCount);

    std::cout << "Active attributes (" << attribCount << "):\n";
    for (GLint i = 0; i < attribCount; i++) {
        char name[256];
        GLsizei length;
        GLint size;
        GLenum type;
        
        glGetActiveAttrib(m_program, i, sizeof(name), &length, &size, &type, name);
        GLint location = glGetAttribLocation(m_program, name);
        
        std::cout << "  " << name << " (location: " << location << ", size: " << size << ")\n";
    }
}

void ShaderProgram::Cleanup() {
    if (m_program) {
        glDeleteProgram(m_program);
        m_program = 0;
    }
    m_uniformLocationCache.clear();
}