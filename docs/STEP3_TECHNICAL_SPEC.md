# Step 3-3, 3-4 기술 명세서

## 1. 시스템 아키텍처

### 1.1 렌더링 파이프라인 구조
```
Application Layer
    ├── Circuit (게임 로직)
    ├── InputHandler (입력 처리)
    └── GameState (상태 관리)
           ↓
Rendering Layer
    ├── RenderManager (렌더링 총괄)
    │   ├── GridRenderer
    │   ├── GateRenderer ← Step 3-3
    │   └── WireRenderer ← Step 3-4
    └── Camera (뷰 변환)
           ↓
Graphics API Layer
    ├── OpenGL 3.3+
    ├── Shader Management
    └── Buffer Management
```

### 1.2 데이터 플로우
```
게임 데이터 → 렌더 데이터 변환 → GPU 버퍼 업데이트 → 셰이더 렌더링
   Circuit      RenderData          VAO/VBO           Shader Program
```

## 2. Step 3-3: 게이트 렌더링 기술 구현

### 2.1 클래스 설계

#### GateRenderer.h
```cpp
#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "core/Gate.h"
#include "render/Shader.h"
#include "render/Camera.h"

class GateRenderer {
public:
    GateRenderer();
    ~GateRenderer();
    
    bool initialize();
    void cleanup();
    
    void beginFrame();
    void renderGates(const std::vector<Gate>& gates, const Camera& camera);
    void endFrame();
    
    void setGateSize(float size) { m_gateSize = size; }
    void enableInstancing(bool enable) { m_useInstancing = enable; }
    
private:
    struct GateInstance {
        glm::vec2 position;
        glm::vec4 color;
        float rotation;
        float scale;
    };
    
    struct PortData {
        glm::vec2 offset;
        glm::vec4 color;
        bool isInput;
    };
    
    void setupGeometry();
    void setupShaders();
    void updateInstanceBuffer(const std::vector<Gate>& gates);
    void renderBatch(const std::vector<GateInstance>& instances);
    void renderSingleGate(const Gate& gate, const glm::mat4& mvp);
    
    // OpenGL 리소스
    GLuint m_vaoGate;
    GLuint m_vboGate;
    GLuint m_eboGate;
    GLuint m_vboInstance;
    GLuint m_vaoPort;
    GLuint m_vboPort;
    
    // 셰이더
    std::unique_ptr<Shader> m_gateShader;
    std::unique_ptr<Shader> m_portShader;
    
    // 렌더링 설정
    float m_gateSize;
    bool m_useInstancing;
    size_t m_maxInstances;
    
    // 버퍼 데이터
    std::vector<GateInstance> m_instanceData;
    std::vector<float> m_vertices;
    std::vector<unsigned int> m_indices;
};
```

### 2.2 버텍스 데이터 구조

#### 게이트 지오메트리
```cpp
// 게이트 본체 버텍스 (단위 사각형)
const float gateVertices[] = {
    // 위치 (x, y)    // 텍스처 좌표 (u, v)
    -0.5f, -0.5f,     0.0f, 0.0f,  // 좌하단
     0.5f, -0.5f,     1.0f, 0.0f,  // 우하단
     0.5f,  0.5f,     1.0f, 1.0f,  // 우상단
    -0.5f,  0.5f,     0.0f, 1.0f   // 좌상단
};

const unsigned int gateIndices[] = {
    0, 1, 2,  // 첫 번째 삼각형
    2, 3, 0   // 두 번째 삼각형
};

// 포트 지오메트리 (작은 원)
const int portSegments = 8;
std::vector<float> generatePortVertices(float radius) {
    std::vector<float> vertices;
    for (int i = 0; i <= portSegments; i++) {
        float angle = 2.0f * M_PI * i / portSegments;
        vertices.push_back(radius * cos(angle));
        vertices.push_back(radius * sin(angle));
    }
    return vertices;
}
```

### 2.3 셰이더 프로그램

#### gate_vertex.glsl
```glsl
#version 330 core

// 버텍스 속성
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

// 인스턴스 속성
layout (location = 2) in vec2 aInstancePos;
layout (location = 3) in vec4 aInstanceColor;
layout (location = 4) in float aInstanceRotation;
layout (location = 5) in float aInstanceScale;

// 유니폼
uniform mat4 uProjection;
uniform mat4 uView;
uniform float uGridSize;

// 출력
out vec2 TexCoord;
out vec4 GateColor;

void main() {
    // 회전 변환
    float cosR = cos(aInstanceRotation);
    float sinR = sin(aInstanceRotation);
    mat2 rotation = mat2(cosR, -sinR, sinR, cosR);
    
    // 로컬 좌표 변환
    vec2 localPos = rotation * (aPos * aInstanceScale * uGridSize);
    vec2 worldPos = localPos + aInstancePos * uGridSize;
    
    // 최종 위치
    gl_Position = uProjection * uView * vec4(worldPos, 0.0, 1.0);
    
    // 프래그먼트 셰이더로 전달
    TexCoord = aTexCoord;
    GateColor = aInstanceColor;
}
```

#### gate_fragment.glsl
```glsl
#version 330 core

in vec2 TexCoord;
in vec4 GateColor;

uniform sampler2D uTexture;
uniform bool uUseTexture;
uniform vec4 uBorderColor;
uniform float uBorderWidth;

out vec4 FragColor;

void main() {
    if (uUseTexture) {
        vec4 texColor = texture(uTexture, TexCoord);
        FragColor = texColor * GateColor;
    } else {
        // 테두리 체크
        float border = uBorderWidth / 100.0;
        if (TexCoord.x < border || TexCoord.x > 1.0 - border ||
            TexCoord.y < border || TexCoord.y > 1.0 - border) {
            FragColor = uBorderColor;
        } else {
            FragColor = GateColor;
        }
    }
    
    // 감마 보정
    FragColor.rgb = pow(FragColor.rgb, vec3(2.2));
}
```

### 2.4 인스턴스 렌더링 구현

```cpp
void GateRenderer::renderGates(const std::vector<Gate>& gates, const Camera& camera) {
    if (gates.empty()) return;
    
    // 프러스텀 컬링
    auto visibleGates = frustumCull(gates, camera);
    
    if (m_useInstancing && visibleGates.size() > 10) {
        // 인스턴스 데이터 준비
        m_instanceData.clear();
        m_instanceData.reserve(visibleGates.size());
        
        for (const auto& gate : visibleGates) {
            GateInstance instance;
            instance.position = gate.position;
            instance.color = getGateColor(gate);
            instance.rotation = gate.rotation;
            instance.scale = 1.0f;
            m_instanceData.push_back(instance);
        }
        
        // GPU 버퍼 업데이트
        glBindBuffer(GL_ARRAY_BUFFER, m_vboInstance);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 
                       sizeof(GateInstance) * m_instanceData.size(),
                       m_instanceData.data());
        
        // 인스턴스 렌더링
        m_gateShader->use();
        m_gateShader->setMat4("uProjection", camera.getProjection());
        m_gateShader->setMat4("uView", camera.getView());
        
        glBindVertexArray(m_vaoGate);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0,
                               m_instanceData.size());
    } else {
        // 개별 렌더링 (소규모)
        for (const auto& gate : visibleGates) {
            renderSingleGate(gate, camera.getViewProjection());
        }
    }
}
```

### 2.5 메모리 관리

```cpp
class GateBufferManager {
private:
    struct BufferPool {
        GLuint vbo;
        size_t capacity;
        size_t used;
        void* mappedPtr;
    };
    
    std::vector<BufferPool> m_pools;
    size_t m_currentPool;
    
public:
    void* allocate(size_t size) {
        if (m_pools[m_currentPool].used + size > m_pools[m_currentPool].capacity) {
            createNewPool(size * 2);
        }
        
        void* ptr = static_cast<char*>(m_pools[m_currentPool].mappedPtr) 
                  + m_pools[m_currentPool].used;
        m_pools[m_currentPool].used += size;
        return ptr;
    }
    
    void reset() {
        for (auto& pool : m_pools) {
            pool.used = 0;
        }
        m_currentPool = 0;
    }
};
```

## 3. Step 3-4: 와이어 렌더링 기술 구현

### 3.1 클래스 설계

#### WireRenderer.h
```cpp
#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "core/Wire.h"
#include "render/Shader.h"
#include "render/Camera.h"

class WireRenderer {
public:
    WireRenderer();
    ~WireRenderer();
    
    bool initialize();
    void cleanup();
    
    void renderWires(const std::vector<Wire>& wires, const Camera& camera);
    void renderDraggingWire(const glm::vec2& start, const glm::vec2& end, 
                           const Camera& camera);
    
    void setLineWidth(float width) { m_lineWidth = width; }
    void setAntialiasing(bool enable) { m_antialiasing = enable; }
    
private:
    struct WirePath {
        std::vector<glm::vec2> points;
        glm::vec4 color;
        float thickness;
        bool animated;
        float animationPhase;
    };
    
    void setupShaders();
    void calculatePath(const Wire& wire, WirePath& path);
    void renderPath(const WirePath& path, const glm::mat4& mvp);
    void updateDynamicBuffer(const std::vector<WirePath>& paths);
    
    // 경로 계산 알고리즘
    std::vector<glm::vec2> calculateManhattanPath(const glm::vec2& start, 
                                                  const glm::vec2& end);
    std::vector<glm::vec2> calculateSmartPath(const glm::vec2& start,
                                              const glm::vec2& end,
                                              const std::vector<glm::vec2>& obstacles);
    
    // OpenGL 리소스
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_vaoJoint;
    GLuint m_vboJoint;
    
    // 셰이더
    std::unique_ptr<Shader> m_wireShader;
    std::unique_ptr<Shader> m_jointShader;
    
    // 렌더링 설정
    float m_lineWidth;
    bool m_antialiasing;
    
    // 버퍼 관리
    std::vector<float> m_vertexBuffer;
    size_t m_maxVertices;
};
```

### 3.2 경로 계산 알고리즘

#### Manhattan 경로 계산
```cpp
std::vector<glm::vec2> WireRenderer::calculateManhattanPath(
    const glm::vec2& start, const glm::vec2& end) {
    
    std::vector<glm::vec2> path;
    path.push_back(start);
    
    glm::vec2 diff = end - start;
    
    // 직선 경로
    if (abs(diff.x) < 0.001f || abs(diff.y) < 0.001f) {
        path.push_back(end);
        return path;
    }
    
    // L자 경로 (수평 먼저)
    if (abs(diff.x) > abs(diff.y)) {
        path.push_back(glm::vec2(end.x, start.y));
        path.push_back(end);
    } 
    // L자 경로 (수직 먼저)
    else {
        path.push_back(glm::vec2(start.x, end.y));
        path.push_back(end);
    }
    
    return path;
}
```

#### A* 경로 탐색 (복잡한 경우)
```cpp
struct PathNode {
    glm::ivec2 pos;
    float g, h, f;
    PathNode* parent;
    
    bool operator<(const PathNode& other) const {
        return f > other.f; // 우선순위 큐용
    }
};

std::vector<glm::vec2> WireRenderer::calculateSmartPath(
    const glm::vec2& start, const glm::vec2& end,
    const std::vector<glm::vec2>& obstacles) {
    
    std::priority_queue<PathNode> openSet;
    std::unordered_set<glm::ivec2> closedSet;
    
    PathNode startNode;
    startNode.pos = glm::ivec2(start);
    startNode.g = 0;
    startNode.h = manhattan_distance(start, end);
    startNode.f = startNode.h;
    startNode.parent = nullptr;
    
    openSet.push(startNode);
    
    while (!openSet.empty()) {
        PathNode current = openSet.top();
        openSet.pop();
        
        if (current.pos == glm::ivec2(end)) {
            return reconstructPath(current);
        }
        
        closedSet.insert(current.pos);
        
        // 4방향 탐색
        const glm::ivec2 directions[] = {
            {0, 1}, {1, 0}, {0, -1}, {-1, 0}
        };
        
        for (const auto& dir : directions) {
            glm::ivec2 neighborPos = current.pos + dir;
            
            if (closedSet.count(neighborPos) || isObstacle(neighborPos, obstacles)) {
                continue;
            }
            
            PathNode neighbor;
            neighbor.pos = neighborPos;
            neighbor.g = current.g + 1;
            neighbor.h = manhattan_distance(glm::vec2(neighborPos), end);
            neighbor.f = neighbor.g + neighbor.h;
            neighbor.parent = &current;
            
            openSet.push(neighbor);
        }
    }
    
    // 경로를 찾지 못한 경우 직선 경로 반환
    return {start, end};
}
```

### 3.3 와이어 셰이더

#### wire_vertex.glsl
```glsl
#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in float aThickness;

uniform mat4 uProjection;
uniform mat4 uView;
uniform vec2 uResolution;

out vec4 WireColor;
out float Thickness;

void main() {
    vec4 worldPos = uView * vec4(aPos, 0.0, 1.0);
    gl_Position = uProjection * worldPos;
    
    WireColor = aColor;
    Thickness = aThickness;
    
    // 선 두께를 픽셀 단위로 설정
    gl_PointSize = aThickness;
}
```

#### wire_geometry.glsl (선 두께 처리)
```glsl
#version 330 core

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

in vec4 WireColor[];
in float Thickness[];

uniform mat4 uProjection;
uniform vec2 uResolution;

out vec4 FragColor;
out vec2 UV;

void main() {
    vec4 p0 = gl_in[0].gl_Position;
    vec4 p1 = gl_in[1].gl_Position;
    
    // 화면 공간으로 변환
    vec2 screen0 = (p0.xy / p0.w) * uResolution * 0.5;
    vec2 screen1 = (p1.xy / p1.w) * uResolution * 0.5;
    
    // 선의 방향과 수직 벡터 계산
    vec2 dir = normalize(screen1 - screen0);
    vec2 normal = vec2(-dir.y, dir.x);
    
    // 두께만큼 확장
    float halfThickness = Thickness[0] * 0.5;
    normal *= halfThickness / uResolution;
    
    // 사각형 버텍스 생성
    FragColor = WireColor[0];
    
    UV = vec2(0, 0);
    gl_Position = p0 + vec4(normal, 0, 0);
    EmitVertex();
    
    UV = vec2(0, 1);
    gl_Position = p0 - vec4(normal, 0, 0);
    EmitVertex();
    
    UV = vec2(1, 0);
    gl_Position = p1 + vec4(normal, 0, 0);
    EmitVertex();
    
    UV = vec2(1, 1);
    gl_Position = p1 - vec4(normal, 0, 0);
    EmitVertex();
    
    EndPrimitive();
}
```

#### wire_fragment.glsl
```glsl
#version 330 core

in vec4 FragColor;
in vec2 UV;

uniform float uTime;
uniform bool uAnimated;
uniform float uAnimationSpeed;

out vec4 OutColor;

void main() {
    vec4 color = FragColor;
    
    // 신호 애니메이션
    if (uAnimated) {
        float phase = fract(UV.x - uTime * uAnimationSpeed);
        float pulse = smoothstep(0.0, 0.1, phase) * smoothstep(0.3, 0.1, phase);
        color.rgb += vec3(pulse * 0.5);
    }
    
    // 안티앨리어싱
    float alpha = 1.0 - smoothstep(0.4, 0.5, abs(UV.y - 0.5));
    color.a *= alpha;
    
    OutColor = color;
}
```

### 3.4 동적 버퍼 관리

```cpp
class DynamicVertexBuffer {
private:
    GLuint m_vbo;
    size_t m_capacity;
    size_t m_used;
    bool m_mapped;
    void* m_mappedPtr;
    
public:
    DynamicVertexBuffer(size_t initialCapacity) 
        : m_capacity(initialCapacity), m_used(0), m_mapped(false) {
        
        glGenBuffers(1, &m_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, m_capacity, nullptr, GL_DYNAMIC_DRAW);
    }
    
    void begin() {
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        m_mappedPtr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        m_mapped = true;
        m_used = 0;
    }
    
    void* allocate(size_t size) {
        if (m_used + size > m_capacity) {
            resize(m_capacity * 2);
        }
        
        void* ptr = static_cast<char*>(m_mappedPtr) + m_used;
        m_used += size;
        return ptr;
    }
    
    void end() {
        glUnmapBuffer(GL_ARRAY_BUFFER);
        m_mapped = false;
    }
    
    void resize(size_t newCapacity) {
        // 새 버퍼 생성 및 데이터 복사
        GLuint newVbo;
        glGenBuffers(1, &newVbo);
        glBindBuffer(GL_ARRAY_BUFFER, newVbo);
        glBufferData(GL_ARRAY_BUFFER, newCapacity, nullptr, GL_DYNAMIC_DRAW);
        
        // 기존 데이터 복사
        glBindBuffer(GL_COPY_READ_BUFFER, m_vbo);
        glBindBuffer(GL_COPY_WRITE_BUFFER, newVbo);
        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 
                           0, 0, m_used);
        
        glDeleteBuffers(1, &m_vbo);
        m_vbo = newVbo;
        m_capacity = newCapacity;
    }
};
```

## 4. 성능 최적화 기법

### 4.1 배치 처리
```cpp
class BatchRenderer {
private:
    struct Batch {
        GLenum primitive;
        GLuint vao;
        size_t vertexCount;
        size_t instanceCount;
        Shader* shader;
        glm::mat4 transform;
    };
    
    std::vector<Batch> m_batches;
    
public:
    void addBatch(const Batch& batch) {
        // 같은 상태의 배치 병합
        for (auto& existing : m_batches) {
            if (canMerge(existing, batch)) {
                existing.vertexCount += batch.vertexCount;
                return;
            }
        }
        m_batches.push_back(batch);
    }
    
    void flush() {
        // 상태 변경 최소화하며 렌더링
        GLuint lastVAO = 0;
        Shader* lastShader = nullptr;
        
        for (const auto& batch : m_batches) {
            if (batch.shader != lastShader) {
                batch.shader->use();
                lastShader = batch.shader;
            }
            
            if (batch.vao != lastVAO) {
                glBindVertexArray(batch.vao);
                lastVAO = batch.vao;
            }
            
            if (batch.instanceCount > 0) {
                glDrawElementsInstanced(batch.primitive, batch.vertexCount,
                                      GL_UNSIGNED_INT, 0, batch.instanceCount);
            } else {
                glDrawElements(batch.primitive, batch.vertexCount,
                             GL_UNSIGNED_INT, 0);
            }
        }
        
        m_batches.clear();
    }
};
```

### 4.2 프러스텀 컬링
```cpp
class FrustumCuller {
private:
    struct Plane {
        glm::vec3 normal;
        float distance;
    };
    
    Plane m_planes[6]; // 6개 절두체 평면
    
public:
    void update(const glm::mat4& viewProjection) {
        // 절두체 평면 추출
        extractPlanes(viewProjection);
    }
    
    bool isVisible(const glm::vec2& pos, float radius) const {
        for (const auto& plane : m_planes) {
            float dist = glm::dot(glm::vec3(pos, 0), plane.normal) + plane.distance;
            if (dist < -radius) {
                return false;
            }
        }
        return true;
    }
    
    template<typename T>
    std::vector<T> cull(const std::vector<T>& objects, 
                        const std::function<glm::vec2(const T&)>& getPos) {
        std::vector<T> visible;
        visible.reserve(objects.size());
        
        for (const auto& obj : objects) {
            if (isVisible(getPos(obj), 1.0f)) {
                visible.push_back(obj);
            }
        }
        
        return visible;
    }
};
```

### 4.3 LOD 시스템
```cpp
enum class LODLevel {
    HIGH = 0,    // 전체 디테일
    MEDIUM = 1,  // 중간 디테일
    LOW = 2,     // 최소 디테일
    CULLED = 3   // 렌더링 안함
};

class LODManager {
private:
    float m_lodDistances[3] = {100.0f, 500.0f, 1000.0f};
    
public:
    LODLevel calculateLOD(const glm::vec2& objectPos, const Camera& camera) {
        float distance = glm::distance(objectPos, camera.getPosition());
        
        for (int i = 0; i < 3; i++) {
            if (distance < m_lodDistances[i]) {
                return static_cast<LODLevel>(i);
            }
        }
        
        return LODLevel::CULLED;
    }
    
    void renderWithLOD(const Gate& gate, LODLevel lod) {
        switch (lod) {
            case LODLevel::HIGH:
                renderFullGate(gate);
                renderPorts(gate);
                renderLabel(gate);
                break;
            case LODLevel::MEDIUM:
                renderSimpleGate(gate);
                renderPorts(gate);
                break;
            case LODLevel::LOW:
                renderSimpleGate(gate);
                break;
            case LODLevel::CULLED:
                // 렌더링 안함
                break;
        }
    }
};
```

## 5. 메모리 레이아웃 최적화

### 5.1 Structure of Arrays (SoA)
```cpp
struct GatesSOA {
    alignas(64) std::vector<glm::vec2> positions;
    alignas(64) std::vector<uint32_t> types;
    alignas(64) std::vector<uint8_t> signals;
    alignas(64) std::vector<uint32_t> connections;
    
    void reserve(size_t count) {
        positions.reserve(count);
        types.reserve(count);
        signals.reserve(count);
        connections.reserve(count * 4); // 4 connections per gate
    }
    
    size_t size() const { return positions.size(); }
};
```

### 5.2 캐시 친화적 데이터 구조
```cpp
struct CacheOptimizedWire {
    // 64바이트 캐시 라인에 맞춤
    glm::vec2 start;        // 8 bytes
    glm::vec2 end;          // 8 bytes
    glm::vec4 color;        // 16 bytes
    uint32_t connections[2]; // 8 bytes
    uint8_t signal;         // 1 byte
    uint8_t padding[23];    // 23 bytes padding
}; // Total: 64 bytes
static_assert(sizeof(CacheOptimizedWire) == 64);
```

## 6. 디버깅 및 프로파일링

### 6.1 렌더링 통계
```cpp
struct RenderStats {
    uint32_t drawCalls;
    uint32_t vertices;
    uint32_t triangles;
    uint32_t gatesRendered;
    uint32_t wiresRendered;
    float frameTime;
    float gpuTime;
    
    void reset() {
        memset(this, 0, sizeof(RenderStats));
    }
    
    void print() const {
        printf("Frame: %.2fms, GPU: %.2fms\n", frameTime, gpuTime);
        printf("Draw Calls: %u, Vertices: %u\n", drawCalls, vertices);
        printf("Gates: %u, Wires: %u\n", gatesRendered, wiresRendered);
    }
};
```

### 6.2 GPU 타이머
```cpp
class GPUTimer {
private:
    GLuint m_queries[2];
    int m_current;
    
public:
    void begin() {
        glBeginQuery(GL_TIME_ELAPSED, m_queries[m_current]);
    }
    
    void end() {
        glEndQuery(GL_TIME_ELAPSED);
    }
    
    float getTime() {
        GLuint64 time;
        glGetQueryObjectui64v(m_queries[m_current], GL_QUERY_RESULT, &time);
        m_current = 1 - m_current;
        return time / 1000000.0f; // 나노초를 밀리초로 변환
    }
};
```

## 7. 에러 처리 및 검증

### 7.1 OpenGL 에러 체크
```cpp
#ifdef DEBUG
    #define GL_CHECK(x) \
        x; \
        { \
            GLenum err = glGetError(); \
            if (err != GL_NO_ERROR) { \
                printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); \
            } \
        }
#else
    #define GL_CHECK(x) x
#endif
```

### 7.2 셰이더 컴파일 검증
```cpp
bool compileShader(GLuint shader, const char* source) {
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        printf("Shader compilation failed: %s\n", infoLog);
        return false;
    }
    
    return true;
}
```

## 8. 플랫폼별 최적화

### 8.1 Windows (MSVC)
```cpp
#ifdef _MSC_VER
    #pragma intrinsic(_mm_prefetch)
    #define PREFETCH(addr) _mm_prefetch((const char*)(addr), _MM_HINT_T0)
#else
    #define PREFETCH(addr) __builtin_prefetch(addr)
#endif
```

### 8.2 SIMD 최적화
```cpp
#include <immintrin.h>

void transformPositionsSIMD(float* positions, size_t count, 
                           const glm::mat4& transform) {
    __m256 row0 = _mm256_set1_ps(transform[0][0]);
    __m256 row1 = _mm256_set1_ps(transform[1][1]);
    __m256 row2 = _mm256_set1_ps(transform[3][0]);
    __m256 row3 = _mm256_set1_ps(transform[3][1]);
    
    for (size_t i = 0; i < count; i += 8) {
        __m256 x = _mm256_load_ps(&positions[i * 2]);
        __m256 y = _mm256_load_ps(&positions[i * 2 + 8]);
        
        __m256 tx = _mm256_fmadd_ps(x, row0, row2);
        __m256 ty = _mm256_fmadd_ps(y, row1, row3);
        
        _mm256_store_ps(&positions[i * 2], tx);
        _mm256_store_ps(&positions[i * 2 + 8], ty);
    }
}
```