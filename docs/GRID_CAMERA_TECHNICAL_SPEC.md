# Grid Rendering & Camera System Technical Specification

## 1. 기술 아키텍처

### 1.1 시스템 구성도
```
┌─────────────────────────────────────────┐
│            Application Layer             │
├─────────────────────────────────────────┤
│         InputHandler                     │
│              ↓                           │
│    ┌─────────────┬──────────────┐       │
│    │   Camera    │ GridRenderer │       │
│    └─────┬───────┴──────┬───────┘       │
├──────────┼──────────────┼───────────────┤
│          │   OpenGL API │               │
│          └──────┬───────┘               │
│                 ↓                        │
│         GPU (Shaders)                   │
└─────────────────────────────────────────┘
```

### 1.2 의존성
- **OpenGL 3.3 Core Profile**
- **GLM 0.9.9+** (수학 라이브러리)
- **SDL2 2.0.14+** (윈도우/입력)
- **GLAD** (OpenGL 로더)

## 2. 메모리 레이아웃

### 2.1 Vertex Data Structure
```cpp
// 그리드 정점 구조체 (32 bytes aligned)
struct GridVertex {
    glm::vec2 position;    // 8 bytes
    glm::vec2 texCoord;    // 8 bytes  
    glm::vec4 color;       // 16 bytes
};

// 캐시 라인 최적화 (64 bytes)
struct alignas(64) GridVertexBuffer {
    GridVertex vertices[2];  // 64 bytes total
};
```

### 2.2 Uniform Buffer Object (UBO)
```cpp
// 카메라 UBO (std140 layout)
struct alignas(16) CameraUBO {
    glm::mat4 viewMatrix;       // 64 bytes
    glm::mat4 projMatrix;       // 64 bytes
    glm::mat4 viewProjMatrix;   // 64 bytes
    glm::vec4 viewport;         // 16 bytes (x, y, width, height)
    glm::vec2 mousePos;         // 8 bytes
    float zoom;                 // 4 bytes
    float time;                 // 4 bytes
    // padding                  // 8 bytes
};  // Total: 224 bytes
```

### 2.3 메모리 풀
```cpp
class MemoryPool {
    static constexpr size_t GRID_VERTEX_POOL_SIZE = 1024 * 1024;  // 1MB
    static constexpr size_t HIGHLIGHT_POOL_SIZE = 256 * 1024;     // 256KB
    
    alignas(64) uint8_t gridVertexPool[GRID_VERTEX_POOL_SIZE];
    alignas(64) uint8_t highlightPool[HIGHLIGHT_POOL_SIZE];
    
    size_t gridVertexOffset = 0;
    size_t highlightOffset = 0;
};
```

## 3. OpenGL 리소스 관리

### 3.1 VAO/VBO 구성
```cpp
class GLResourceManager {
    // 그리드 리소스
    GLuint gridVAO;
    GLuint gridVBO;
    GLuint gridEBO;
    
    // 하이라이트 리소스  
    GLuint highlightVAO;
    GLuint highlightVBO;
    GLuint highlightInstanceVBO;  // 인스턴싱용
    
    // 셰이더 프로그램
    GLuint gridShaderProgram;
    GLuint highlightShaderProgram;
    
    // UBO
    GLuint cameraUBO;
    GLuint gridSettingsUBO;
};
```

### 3.2 버퍼 업데이트 전략
```cpp
enum class BufferUpdateStrategy {
    STATIC_DRAW,     // 그리드 메시 (변경 없음)
    DYNAMIC_DRAW,    // 하이라이트 (프레임마다 변경 가능)
    STREAM_DRAW      // 임시 디버그 렌더링
};

void UpdateBuffer(GLuint vbo, const void* data, size_t size, 
                  BufferUpdateStrategy strategy) {
    GLenum usage = GL_STATIC_DRAW;
    switch (strategy) {
        case BufferUpdateStrategy::DYNAMIC_DRAW:
            usage = GL_DYNAMIC_DRAW;
            break;
        case BufferUpdateStrategy::STREAM_DRAW:
            usage = GL_STREAM_DRAW;
            break;
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size, data, usage);
}
```

## 4. 렌더링 파이프라인

### 4.1 렌더링 순서
```cpp
void RenderFrame() {
    // 1. 프레임 시작
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 2. UBO 업데이트
    UpdateCameraUBO();
    
    // 3. 그리드 렌더링 (불투명)
    RenderOpaqueGrid();
    
    // 4. 하이라이트 렌더링 (반투명)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    RenderHighlights();
    glDisable(GL_BLEND);
    
    // 5. UI 렌더링 (ImGui)
    RenderUI();
    
    // 6. 프레임 완료
    SDL_GL_SwapWindow(window);
}
```

### 4.2 Frustum Culling 구현
```cpp
class FrustumCuller {
    struct Plane {
        glm::vec3 normal;
        float distance;
    };
    
    Plane frustumPlanes[6];  // Left, Right, Top, Bottom, Near, Far
    
public:
    void UpdateFrustum(const glm::mat4& viewProj) {
        // Extract frustum planes from view-projection matrix
        ExtractPlanesFromMatrix(viewProj, frustumPlanes);
    }
    
    bool IsGridCellVisible(const glm::ivec2& gridPos, float cellSize) {
        // AABB vs Frustum test
        glm::vec3 min(gridPos.x * cellSize, gridPos.y * cellSize, -0.1f);
        glm::vec3 max((gridPos.x + 1) * cellSize, (gridPos.y + 1) * cellSize, 0.1f);
        
        return AABBInFrustum(min, max);
    }
    
private:
    bool AABBInFrustum(const glm::vec3& min, const glm::vec3& max) {
        for (const auto& plane : frustumPlanes) {
            glm::vec3 positive = min;
            if (plane.normal.x >= 0) positive.x = max.x;
            if (plane.normal.y >= 0) positive.y = max.y;
            if (plane.normal.z >= 0) positive.z = max.z;
            
            if (glm::dot(plane.normal, positive) + plane.distance < 0) {
                return false;  // Outside frustum
            }
        }
        return true;
    }
};
```

## 5. 셰이더 기술 명세

### 5.1 Vertex Shader 구현
```glsl
#version 330 core

// Vertex attributes
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;

// Instance attributes (for instanced rendering)
layout(location = 3) in mat4 aInstanceMatrix;
layout(location = 7) in vec4 aInstanceColor;

// Uniform blocks
layout(std140) uniform CameraData {
    mat4 viewMatrix;
    mat4 projMatrix;
    mat4 viewProjMatrix;
    vec4 viewport;
    vec2 mousePos;
    float zoom;
    float time;
} camera;

// Outputs
out vec2 vTexCoord;
out vec4 vColor;
out float vDistance;  // Distance from camera for LOD

void main() {
    vec4 worldPos = aInstanceMatrix * vec4(aPosition, 0.0, 1.0);
    gl_Position = camera.viewProjMatrix * worldPos;
    
    vTexCoord = aTexCoord;
    vColor = aColor * aInstanceColor;
    
    // Calculate distance for LOD
    vec4 viewPos = camera.viewMatrix * worldPos;
    vDistance = length(viewPos.xyz);
}
```

### 5.2 Fragment Shader 구현
```glsl
#version 330 core

// Inputs
in vec2 vTexCoord;
in vec4 vColor;
in float vDistance;

// Uniforms
uniform float uGridOpacity;
uniform float uLineWidth;
uniform bool uEnableAA;

// Outputs
out vec4 FragColor;

float computeGridIntensity(vec2 coord) {
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / fwidth(coord);
    float line = min(grid.x, grid.y);
    
    if (uEnableAA) {
        // Anti-aliased lines
        return 1.0 - min(line, 1.0);
    } else {
        // Sharp lines
        return step(line, uLineWidth);
    }
}

void main() {
    float intensity = computeGridIntensity(vTexCoord * 32.0);
    
    // LOD based on distance
    float lodFactor = smoothstep(100.0, 500.0, vDistance);
    intensity *= (1.0 - lodFactor * 0.5);
    
    FragColor = vec4(vColor.rgb, vColor.a * intensity * uGridOpacity);
    
    // Discard fully transparent pixels
    if (FragColor.a < 0.01) {
        discard;
    }
}
```

## 6. 카메라 시스템 기술 구현

### 6.1 투영 매트릭스 계산
```cpp
glm::mat4 Camera::GetProjectionMatrix() const {
    // Orthographic projection for 2D grid
    float halfWidth = m_screenSize.x * 0.5f / (CELL_SIZE * m_zoom);
    float halfHeight = m_screenSize.y * 0.5f / (CELL_SIZE * m_zoom);
    
    return glm::ortho(
        -halfWidth,   // left
        halfWidth,    // right
        -halfHeight,  // bottom
        halfHeight,   // top
        -1.0f,        // near
        1.0f          // far
    );
}
```

### 6.2 뷰 매트릭스 계산
```cpp
glm::mat4 Camera::GetViewMatrix() const {
    // 2D camera transform
    glm::mat4 view(1.0f);
    
    // Translation
    view = glm::translate(view, glm::vec3(-m_position, 0.0f));
    
    // No rotation for 2D grid
    
    return view;
}
```

### 6.3 스무스 카메라 이동
```cpp
class SmoothCamera : public Camera {
    glm::vec2 m_targetPosition;
    float m_targetZoom;
    float m_smoothSpeed = 10.0f;
    
public:
    void Update(float deltaTime) {
        // Smooth position interpolation
        m_position = glm::mix(m_position, m_targetPosition, 
                              1.0f - exp(-m_smoothSpeed * deltaTime));
        
        // Smooth zoom interpolation
        m_zoom = glm::mix(m_zoom, m_targetZoom,
                          1.0f - exp(-m_smoothSpeed * deltaTime));
    }
    
    void SetTargetPosition(const glm::vec2& target) {
        m_targetPosition = target;
    }
    
    void SetTargetZoom(float zoom) {
        m_targetZoom = glm::clamp(zoom, MIN_ZOOM, MAX_ZOOM);
    }
};
```

## 7. 최적화 기법

### 7.1 인스턴스 렌더링
```cpp
void RenderGridInstanced() {
    // 그리드 셀 인스턴스 데이터 준비
    std::vector<glm::mat4> instanceMatrices;
    instanceMatrices.reserve(MAX_VISIBLE_CELLS);
    
    // Frustum culling
    glm::vec4 bounds = camera.GetVisibleBounds();
    for (int y = floor(bounds.y); y <= ceil(bounds.w); ++y) {
        for (int x = floor(bounds.x); x <= ceil(bounds.z); ++x) {
            glm::mat4 model(1.0f);
            model = glm::translate(model, glm::vec3(x * CELL_SIZE, y * CELL_SIZE, 0.0f));
            instanceMatrices.push_back(model);
        }
    }
    
    // 인스턴스 버퍼 업데이트
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, 
                 sizeof(glm::mat4) * instanceMatrices.size(),
                 instanceMatrices.data(), 
                 GL_DYNAMIC_DRAW);
    
    // 인스턴스 렌더링
    glBindVertexArray(gridVAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, instanceMatrices.size());
}
```

### 7.2 Spatial Hashing
```cpp
class SpatialHashGrid {
    static constexpr int CELL_SIZE = 32;
    static constexpr int HASH_SIZE = 1024;
    
    struct Cell {
        std::vector<uint32_t> entities;
    };
    
    std::array<Cell, HASH_SIZE> hashTable;
    
    int Hash(int x, int y) const {
        // Simple spatial hash function
        const int h1 = 0x8da6b343;
        const int h2 = 0xd8163841;
        int n = h1 * x + h2 * y;
        return std::abs(n) % HASH_SIZE;
    }
    
public:
    void Insert(uint32_t entityId, const glm::vec2& position) {
        int gridX = floor(position.x / CELL_SIZE);
        int gridY = floor(position.y / CELL_SIZE);
        int hash = Hash(gridX, gridY);
        hashTable[hash].entities.push_back(entityId);
    }
    
    std::vector<uint32_t> Query(const glm::vec4& bounds) {
        std::vector<uint32_t> result;
        
        int minX = floor(bounds.x / CELL_SIZE);
        int maxX = ceil(bounds.z / CELL_SIZE);
        int minY = floor(bounds.y / CELL_SIZE);
        int maxY = ceil(bounds.w / CELL_SIZE);
        
        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                int hash = Hash(x, y);
                const auto& cell = hashTable[hash];
                result.insert(result.end(), cell.entities.begin(), cell.entities.end());
            }
        }
        
        return result;
    }
};
```

### 7.3 멀티스레드 업데이트
```cpp
class ThreadedGridUpdater {
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex taskMutex;
    std::condition_variable cv;
    std::atomic<bool> running{true};
    
    void WorkerThread() {
        while (running) {
            std::unique_lock<std::mutex> lock(taskMutex);
            cv.wait(lock, [this] { return !tasks.empty() || !running; });
            
            if (!running) break;
            
            auto task = tasks.front();
            tasks.pop();
            lock.unlock();
            
            task();
        }
    }
    
public:
    ThreadedGridUpdater(int numThreads = 4) {
        for (int i = 0; i < numThreads; ++i) {
            workers.emplace_back(&ThreadedGridUpdater::WorkerThread, this);
        }
    }
    
    void UpdateGridSection(int startX, int endX, int startY, int endY) {
        std::lock_guard<std::mutex> lock(taskMutex);
        tasks.push([=] {
            // Update grid vertices for this section
            UpdateVertices(startX, endX, startY, endY);
        });
        cv.notify_one();
    }
};
```

## 8. 성능 프로파일링

### 8.1 GPU 타이머
```cpp
class GPUTimer {
    GLuint queryIds[2];
    int currentQuery = 0;
    
public:
    void Begin() {
        glGenQueries(1, &queryIds[currentQuery]);
        glBeginQuery(GL_TIME_ELAPSED, queryIds[currentQuery]);
    }
    
    void End() {
        glEndQuery(GL_TIME_ELAPSED);
    }
    
    float GetElapsedTime() {
        GLuint64 elapsed;
        glGetQueryObjectui64v(queryIds[currentQuery], GL_QUERY_RESULT, &elapsed);
        currentQuery = 1 - currentQuery;
        return elapsed / 1000000.0f;  // Convert to milliseconds
    }
};
```

### 8.2 성능 메트릭스
```cpp
struct PerformanceMetrics {
    float frameTime;           // ms
    float updateTime;          // ms
    float renderTime;          // ms
    float gpuTime;            // ms
    int drawCalls;
    int verticesRendered;
    int gridCellsVisible;
    float memoryUsageMB;
    
    void Log() const {
        SDL_Log("Frame: %.2fms (%.1f FPS)", frameTime, 1000.0f / frameTime);
        SDL_Log("  Update: %.2fms", updateTime);
        SDL_Log("  Render: %.2fms (GPU: %.2fms)", renderTime, gpuTime);
        SDL_Log("  Draw Calls: %d", drawCalls);
        SDL_Log("  Vertices: %d", verticesRendered);
        SDL_Log("  Visible Cells: %d", gridCellsVisible);
        SDL_Log("  Memory: %.1fMB", memoryUsageMB);
    }
};
```

## 9. 플랫폼별 고려사항

### 9.1 Windows
```cpp
#ifdef _WIN32
    // Enable high-performance GPU
    extern "C" {
        __declspec(dllexport) DWORD NvOptimusEnablement = 1;
        __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
    }
    
    // Set process DPI awareness
    SetProcessDPIAware();
#endif
```

### 9.2 macOS
```cpp
#ifdef __APPLE__
    // Request high-resolution backing
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    
    // Enable Retina support
    int drawableW, drawableH;
    SDL_GL_GetDrawableSize(window, &drawableW, &drawableH);
    float pixelRatio = drawableW / windowW;
#endif
```

### 9.3 Linux
```cpp
#ifdef __linux__
    // Set OpenGL context flags
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    
    // Enable vsync
    SDL_GL_SetSwapInterval(1);
#endif
```

## 10. 디버깅 도구

### 10.1 OpenGL 디버그 콜백
```cpp
void GLAPIENTRY DebugCallback(GLenum source, GLenum type, GLuint id,
                               GLenum severity, GLsizei length,
                               const GLchar* message, const void* userParam) {
    if (severity == GL_DEBUG_SEVERITY_HIGH) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "GL Error: %s", message);
        assert(false);  // Break in debug mode
    } else if (severity == GL_DEBUG_SEVERITY_MEDIUM) {
        SDL_LogWarn(SDL_LOG_CATEGORY_RENDER, "GL Warning: %s", message);
    }
}

void EnableGLDebug() {
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(DebugCallback, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, 
                          GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
}
```

### 10.2 렌더링 상태 검증
```cpp
class RenderStateValidator {
public:
    static bool ValidateVAO(GLuint vao) {
        GLint currentVAO;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);
        return currentVAO == vao;
    }
    
    static bool ValidateShaderProgram(GLuint program) {
        glValidateProgram(program);
        GLint status;
        glGetProgramiv(program, GL_VALIDATE_STATUS, &status);
        return status == GL_TRUE;
    }
    
    static void CheckGLError(const char* operation) {
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            SDL_LogError(SDL_LOG_CATEGORY_RENDER, 
                        "OpenGL error after %s: 0x%x", operation, error);
        }
    }
};
```

## 11. 빌드 설정

### 11.1 CMake 설정
```cmake
# OpenGL 설정
find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)

# 컴파일 플래그
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4           # Warning level 4
        /fp:fast      # Fast floating point
        /arch:AVX2    # Enable AVX2
        /GL           # Whole program optimization
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -O3
        -march=native
        -ffast-math
    )
endif()

# 링크 옵션
target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenGL::GL
    ${SDL2_LIBRARIES}
    glm::glm
)
```

### 11.2 프리프로세서 정의
```cpp
// 디버그 모드
#ifdef DEBUG
    #define GL_CHECK(x) do { \
        x; \
        RenderStateValidator::CheckGLError(#x); \
    } while(0)
    
    #define ENABLE_PROFILING 1
    #define ENABLE_GL_DEBUG 1
#else
    #define GL_CHECK(x) x
    #define ENABLE_PROFILING 0
    #define ENABLE_GL_DEBUG 0
#endif

// 성능 최적화
#define USE_INSTANCED_RENDERING 1
#define USE_FRUSTUM_CULLING 1
#define USE_SPATIAL_HASHING 1
#define MAX_GRID_SIZE 10000
#define VERTEX_BUFFER_SIZE (1024 * 1024)  // 1MB
```