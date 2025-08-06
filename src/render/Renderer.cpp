#include "Renderer.h"
#include "Window.h"
#include "../utils/Logger.h"

#ifdef HAS_OPENGL
#ifdef USE_GLAD
#include <glad/glad.h>
#endif
#endif

#ifdef HAS_SDL2
#include <SDL.h>
#endif

Renderer::Renderer()
    : m_window(nullptr)
    , m_initialized(false)
    , m_viewportX(0)
    , m_viewportY(0)
    , m_viewportWidth(0)
    , m_viewportHeight(0)
{
}

Renderer::~Renderer() {
    if (m_initialized) {
        Shutdown();
    }
}

bool Renderer::Initialize(Window* window) {
    if (!window) {
        Logger::Error("Cannot initialize renderer without a window");
        return false;
    }

    m_window = window;
    m_viewportWidth = window->GetWidth();
    m_viewportHeight = window->GetHeight();

#ifdef HAS_OPENGL
    if (!InitializeOpenGL()) {
        Logger::Error("Failed to initialize OpenGL");
        return false;
    }
#else
    Logger::Warning("OpenGL not available - using software rendering");
#endif

    m_initialized = true;
    Logger::Info("Renderer initialized successfully");
    return true;
}

bool Renderer::InitializeOpenGL() {
#ifdef HAS_OPENGL
    // Set viewport
    glViewport(m_viewportX, m_viewportY, m_viewportWidth, m_viewportHeight);
    
    // Set up default OpenGL state
    SetupDefaultGLState();
    
    // Enable debug output in debug builds
#ifdef DEBUG
    if (glDebugMessageCallback) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity,
                                 GLsizei length, const GLchar* message, const void* userParam) {
            if (severity == GL_DEBUG_SEVERITY_HIGH) {
                Logger::Error("OpenGL Error: " + std::string(message));
            } else if (severity == GL_DEBUG_SEVERITY_MEDIUM) {
                Logger::Warning("OpenGL Warning: " + std::string(message));
            }
        }, nullptr);
    }
#endif

    PrintGLInfo();
    CheckGLErrors("OpenGL initialization");
    
    return true;
#else
    return false;
#endif
}

void Renderer::SetupDefaultGLState() {
#ifdef HAS_OPENGL
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // Enable face culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Set clear color to dark blue
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    
    // Enable multisampling if available
    glEnable(GL_MULTISAMPLE);
    
    // Set line width
    glLineWidth(1.0f);
    
    // Polygon mode (fill by default)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
}

void Renderer::Shutdown() {
    if (!m_initialized) {
        return;
    }

    m_initialized = false;
    Logger::Info("Renderer shut down");
}

void Renderer::BeginFrame() {
#ifdef HAS_OPENGL
    // Nothing specific needed at frame start
#endif
}

void Renderer::EndFrame() {
#ifdef HAS_OPENGL
    // Ensure all OpenGL commands are executed
    glFlush();
#endif
}

void Renderer::Clear(float r, float g, float b, float a) {
#ifdef HAS_OPENGL
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
#endif
}

void Renderer::SetViewport(int x, int y, int width, int height) {
    m_viewportX = x;
    m_viewportY = y;
    m_viewportWidth = width;
    m_viewportHeight = height;

#ifdef HAS_OPENGL
    glViewport(x, y, width, height);
#endif
    
    Logger::Info("Viewport set to: " + std::to_string(width) + "x" + std::to_string(height) + 
                " at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
}

void Renderer::PrintGLInfo() {
#ifdef HAS_OPENGL
    Logger::Info("=== OpenGL Information ===");
    
    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    const GLubyte* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
    
    if (vendor) Logger::Info("Vendor: " + std::string((const char*)vendor));
    if (renderer) Logger::Info("Renderer: " + std::string((const char*)renderer));
    if (version) Logger::Info("OpenGL Version: " + std::string((const char*)version));
    if (glslVersion) Logger::Info("GLSL Version: " + std::string((const char*)glslVersion));
    
    GLint maxTextureSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    Logger::Info("Max Texture Size: " + std::to_string(maxTextureSize));
    
    GLint maxViewportDims[2];
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, maxViewportDims);
    Logger::Info("Max Viewport Size: " + std::to_string(maxViewportDims[0]) + "x" + std::to_string(maxViewportDims[1]));
    
    Logger::Info("==========================");
#endif
}

bool Renderer::CheckGLErrors(const std::string& location) {
#ifdef HAS_OPENGL
    bool hasError = false;
    GLenum error;
    
    while ((error = glGetError()) != GL_NO_ERROR) {
        hasError = true;
        std::string errorStr;
        
        switch (error) {
            case GL_INVALID_ENUM:
                errorStr = "GL_INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                errorStr = "GL_INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                errorStr = "GL_INVALID_OPERATION";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                errorStr = "GL_INVALID_FRAMEBUFFER_OPERATION";
                break;
            case GL_OUT_OF_MEMORY:
                errorStr = "GL_OUT_OF_MEMORY";
                break;
            default:
                errorStr = "Unknown error code: " + std::to_string(error);
                break;
        }
        
        Logger::Error("OpenGL Error at " + location + ": " + errorStr);
    }
    
    return !hasError;
#else
    return true;
#endif
}