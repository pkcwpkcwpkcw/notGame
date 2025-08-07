# OpenGL 셰이더 기술 명세서

## 1. 문서 개요

### 1.1 목적
Step 3-1 "OpenGL 셰이더 작성"의 기술적 구현 세부사항, API 사용법, 성능 최적화 기법을 정의한다.

### 1.2 범위
- GLSL 3.30 언어 사양 및 제약사항
- OpenGL 3.3 Core Profile API 사용
- GPU 아키텍처 고려사항
- 플랫폼별 차이점 및 호환성

### 1.3 관련 문서
- [SHADER_REQUIREMENTS.md](SHADER_REQUIREMENTS.md) - 셰이더 요구사항
- [SHADER_FUNCTIONAL_SPEC.md](SHADER_FUNCTIONAL_SPEC.md) - 셰이더 기능 명세

## 2. 기술 스택

### 2.1 OpenGL 버전
```cpp
// OpenGL 3.3 Core Profile
#define GL_MAJOR_VERSION 3
#define GL_MINOR_VERSION 3
#define GL_CONTEXT_PROFILE_MASK GL_CONTEXT_CORE_PROFILE_BIT
```

### 2.2 GLSL 버전
```glsl
#version 330 core
// GLSL 3.30 - OpenGL 3.3과 매칭
// core 프로파일 - deprecated 기능 제외
```

### 2.3 필수 OpenGL 확장
```cpp
// 필수 확장
GL_ARB_vertex_array_object      // VAO 지원
GL_ARB_framebuffer_object      // FBO 지원
GL_ARB_instanced_arrays        // 인스턴싱

// 선택적 확장 (성능)
GL_ARB_uniform_buffer_object   // UBO 지원
GL_ARB_timer_query             // GPU 타이머
GL_KHR_debug                   // 디버그 출력
```

## 3. 셰이더 컴파일 기술 사양

### 3.1 셰이더 소스 관리
```cpp
class ShaderSource {
private:
    std::string m_source;
    std::vector<std::string> m_includes;
    std::unordered_map<std::string, std::string> m_defines;
    
public:
    // 파일 시스템에서 로드
    bool LoadFromFile(const char* path) {
        std::ifstream file(path);
        if (!file.is_open()) return false;
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        m_source = buffer.str();
        
        ProcessIncludes();  // #include 처리
        ProcessDefines();   // #define 주입
        return true;
    }
    
    // 전처리기 지시문 처리
    void ProcessIncludes() {
        std::regex includeRegex("#include\\s+\"([^\"]+)\"");
        std::smatch match;
        
        while (std::regex_search(m_source, match, includeRegex)) {
            std::string includePath = match[1];
            std::string includeContent = LoadInclude(includePath);
            m_source.replace(match.position(), match.length(), includeContent);
        }
    }
};
```

### 3.2 컴파일 프로세스
```cpp
GLuint CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    
    // 소스 코드 설정
    glShaderSource(shader, 1, &source, nullptr);
    
    // 컴파일
    glCompileShader(shader);
    
    // 컴파일 상태 확인
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    
    if (!success) {
        GLint logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        
        std::vector<char> log(logLength);
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        
        // 에러 파싱 및 라인 번호 추출
        ParseShaderError(log.data(), source);
        
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}
```

### 3.3 링킹 프로세스
```cpp
GLuint LinkProgram(GLuint vertShader, GLuint fragShader) {
    GLuint program = glCreateProgram();
    
    // 셰이더 부착
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    
    // 속성 위치 바인딩 (링킹 전)
    glBindAttribLocation(program, 0, "aPosition");
    glBindAttribLocation(program, 1, "aTexCoord");
    glBindAttribLocation(program, 2, "aColor");
    
    // 프래그먼트 출력 바인딩
    glBindFragDataLocation(program, 0, "fragColor");
    
    // 링킹
    glLinkProgram(program);
    
    // 링킹 상태 확인
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    
    if (!success) {
        GLint logLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        
        std::vector<char> log(logLength);
        glGetProgramInfoLog(program, logLength, nullptr, log.data());
        
        std::cerr << "Shader linking failed: " << log.data() << std::endl;
        
        glDeleteProgram(program);
        return 0;
    }
    
    // 검증 (디버그 모드)
    #ifdef DEBUG
    glValidateProgram(program);
    glGetProgramiv(program, GL_VALIDATE_STATUS, &success);
    if (!success) {
        // 검증 실패 로깅
    }
    #endif
    
    return program;
}
```

## 4. 유니폼 관리 기술 사양

### 4.1 유니폼 위치 캐싱
```cpp
class UniformCache {
private:
    std::unordered_map<std::string, GLint> m_locations;
    GLuint m_program;
    
public:
    void CacheAllUniforms(GLuint program) {
        m_program = program;
        m_locations.clear();
        
        GLint uniformCount;
        glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &uniformCount);
        
        for (GLint i = 0; i < uniformCount; i++) {
            char name[256];
            GLsizei length;
            GLint size;
            GLenum type;
            
            glGetActiveUniform(program, i, sizeof(name), 
                              &length, &size, &type, name);
            
            // 배열 인덱스 제거 ([0])
            char* bracket = strchr(name, '[');
            if (bracket) *bracket = '\0';
            
            GLint location = glGetUniformLocation(program, name);
            m_locations[name] = location;
        }
    }
    
    GLint GetLocation(const std::string& name) {
        auto it = m_locations.find(name);
        if (it != m_locations.end()) {
            return it->second;
        }
        
        // 캐시 미스 - 동적 조회
        GLint location = glGetUniformLocation(m_program, name.c_str());
        m_locations[name] = location;
        return location;
    }
};
```

### 4.2 유니폼 버퍼 객체 (UBO)
```cpp
// UBO 레이아웃 (std140)
struct CommonUniformBlock {
    glm::mat4 projection;      // offset: 0
    glm::mat4 view;            // offset: 64
    glm::vec4 cameraPos;       // offset: 128 (vec2 + padding)
    glm::vec4 timeData;        // offset: 144 (time, deltaTime, ...)
};

class UniformBuffer {
private:
    GLuint m_ubo;
    size_t m_size;
    
public:
    void Create(size_t size) {
        m_size = size;
        
        glGenBuffers(1, &m_ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
        glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
        
        // 바인딩 포인트 설정
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_ubo);
    }
    
    void Update(const void* data, size_t offset, size_t size) {
        glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
        glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
    }
};
```

### 4.3 유니폼 업데이트 최적화
```cpp
class UniformState {
private:
    struct UniformValue {
        std::vector<uint8_t> data;
        size_t hash;
    };
    
    std::unordered_map<GLint, UniformValue> m_cache;
    
public:
    template<typename T>
    void SetUniform(GLint location, const T& value) {
        // 값 해싱
        size_t hash = std::hash<T>{}(value);
        
        // 캐시 확인
        auto it = m_cache.find(location);
        if (it != m_cache.end() && it->second.hash == hash) {
            return; // 동일한 값 - 스킵
        }
        
        // OpenGL 호출
        SetUniformGL(location, value);
        
        // 캐시 업데이트
        UniformValue& cached = m_cache[location];
        cached.data.resize(sizeof(T));
        memcpy(cached.data.data(), &value, sizeof(T));
        cached.hash = hash;
    }
    
private:
    void SetUniformGL(GLint location, float value) {
        glUniform1f(location, value);
    }
    
    void SetUniformGL(GLint location, const glm::vec2& value) {
        glUniform2fv(location, 1, glm::value_ptr(value));
    }
    
    void SetUniformGL(GLint location, const glm::mat4& value) {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }
};
```

## 5. 정점 데이터 기술 사양

### 5.1 정점 배열 객체 (VAO)
```cpp
class VertexArray {
private:
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ebo;
    
public:
    void Create() {
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        glGenBuffers(1, &m_ebo);
    }
    
    void SetupAttributes(const VertexLayout& layout) {
        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        
        size_t offset = 0;
        for (const auto& attrib : layout.attributes) {
            glEnableVertexAttribArray(attrib.location);
            glVertexAttribPointer(
                attrib.location,
                attrib.components,
                attrib.type,
                attrib.normalized,
                layout.stride,
                (void*)offset
            );
            offset += attrib.size;
        }
    }
};
```

### 5.2 인스턴싱 설정
```cpp
void SetupInstancing(GLuint instanceVBO) {
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    
    // 인스턴스 위치
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 
                         sizeof(InstanceData), (void*)0);
    glVertexAttribDivisor(3, 1);  // 인스턴스당 1번
    
    // 인스턴스 회전
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 
                         sizeof(InstanceData), (void*)8);
    glVertexAttribDivisor(4, 1);
    
    // 인스턴스 색상
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 
                         sizeof(InstanceData), (void*)12);
    glVertexAttribDivisor(5, 1);
}
```

## 6. GPU 최적화 기술

### 6.1 셰이더 최적화 기법
```glsl
// 1. 정밀도 지정
precision mediump float;  // 모바일/통합 그래픽

// 2. 분기 최소화
// 나쁜 예
if (condition) {
    color = vec4(1.0);
} else {
    color = vec4(0.0);
}

// 좋은 예
color = vec4(float(condition));

// 3. 텍스처 읽기 최적화
vec4 tex1 = texture(sampler, uv);  // 종속적 텍스처 읽기 피하기
vec4 tex2 = texture(sampler, uv + tex1.xy);  // 나쁜 예

// 4. 벡터 연산 활용
// 나쁜 예
result.x = a.x * b.x;
result.y = a.y * b.y;
result.z = a.z * b.z;

// 좋은 예
result.xyz = a.xyz * b.xyz;

// 5. 공통 하위 표현식 제거
float expensive = sin(angle) * cos(angle);
color.r = expensive * 0.5;
color.g = expensive * 0.7;  // expensive 재사용
```

### 6.2 GPU 메모리 정렬
```cpp
// std140 레이아웃 규칙
struct alignas(16) ShaderData {
    glm::vec4 color;        // 16 bytes
    glm::vec2 position;     // 8 bytes
    float rotation;         // 4 bytes
    float scale;            // 4 bytes
    // 자동 패딩으로 16 byte 정렬
};

// 최적 정점 구조
struct Vertex {
    glm::vec3 position;     // 12 bytes
    glm::vec2 texCoord;     // 8 bytes
    glm::vec3 normal;       // 12 bytes
    // 총 32 bytes - 캐시 라인 친화적
};
```

### 6.3 드로우 콜 배칭
```cpp
class DrawCallBatcher {
private:
    struct BatchData {
        GLuint vao;
        GLuint texture;
        GLuint shader;
        std::vector<glm::mat4> transforms;
    };
    
    std::vector<BatchData> m_batches;
    
public:
    void AddDrawCall(const DrawCall& call) {
        // 동일한 상태 찾기
        BatchData* batch = nullptr;
        for (auto& b : m_batches) {
            if (b.vao == call.vao && 
                b.texture == call.texture && 
                b.shader == call.shader) {
                batch = &b;
                break;
            }
        }
        
        if (!batch) {
            m_batches.push_back({call.vao, call.texture, call.shader});
            batch = &m_batches.back();
        }
        
        batch->transforms.push_back(call.transform);
    }
    
    void Flush() {
        for (const auto& batch : m_batches) {
            glUseProgram(batch.shader);
            glBindTexture(GL_TEXTURE_2D, batch.texture);
            glBindVertexArray(batch.vao);
            
            // 인스턴스 렌더링
            glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 
                                batch.transforms.size());
        }
        
        m_batches.clear();
    }
};
```

## 7. 플랫폼별 고려사항

### 7.1 Windows (MSVC)
```cpp
#ifdef _WIN32
    // ANGLE 지원 (DirectX 백엔드)
    #define GL_ANGLE_SUPPORT
    
    // 고정밀 타이머
    QueryPerformanceCounter(&gpuTimerStart);
#endif
```

### 7.2 Linux
```cpp
#ifdef __linux__
    // Mesa 드라이버 최적화
    setenv("MESA_GL_VERSION_OVERRIDE", "3.3", 1);
    setenv("MESA_GLSL_VERSION_OVERRIDE", "330", 1);
    
    // Intel 드라이버 최적화
    setenv("INTEL_DEBUG", "perf", 1);
#endif
```

### 7.3 macOS
```cpp
#ifdef __APPLE__
    // macOS는 최대 OpenGL 4.1 지원
    // 3.3 Core는 안정적
    
    // Retina 디스플레이 고려
    int framebufferWidth, framebufferHeight;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    float pixelRatio = framebufferWidth / windowWidth;
#endif
```

## 8. 에러 처리 및 디버깅

### 8.1 OpenGL 디버그 콜백
```cpp
void GLAPIENTRY DebugCallback(GLenum source, GLenum type, GLuint id,
                              GLenum severity, GLsizei length,
                              const GLchar* message, const void* userParam) {
    if (severity == GL_DEBUG_SEVERITY_HIGH) {
        std::cerr << "GL ERROR: " << message << std::endl;
        assert(false);  // 개발 중 중단
    } else if (severity == GL_DEBUG_SEVERITY_MEDIUM) {
        std::cerr << "GL WARNING: " << message << std::endl;
    }
}

void EnableDebugOutput() {
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(DebugCallback, nullptr);
    
    // 특정 메시지 필터링
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, 
                         GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
}
```

### 8.2 셰이더 에러 파싱
```cpp
struct ShaderError {
    int line;
    std::string file;
    std::string message;
};

ShaderError ParseGLSLError(const std::string& log) {
    // NVIDIA: "0(15) : error C0000: syntax error"
    // AMD: "ERROR: 0:15: syntax error"
    // Intel: "ERROR: 0:15: '' : syntax error"
    
    std::regex nvidiaRegex(R"((\d+)\((\d+)\)\s*:\s*(.+))");
    std::regex amdRegex(R"(ERROR:\s*(\d+):(\d+):\s*(.+))");
    
    std::smatch match;
    ShaderError error;
    
    if (std::regex_search(log, match, nvidiaRegex)) {
        error.file = match[1];
        error.line = std::stoi(match[2]);
        error.message = match[3];
    } else if (std::regex_search(log, match, amdRegex)) {
        error.file = match[1];
        error.line = std::stoi(match[2]);
        error.message = match[3];
    }
    
    return error;
}
```

## 9. 프로파일링 및 성능 측정

### 9.1 GPU 타이머 쿼리
```cpp
class GPUTimer {
private:
    GLuint m_queries[2];
    bool m_active = false;
    
public:
    void Begin() {
        if (!m_active) {
            glGenQueries(2, m_queries);
        }
        
        glQueryCounter(m_queries[0], GL_TIMESTAMP);
        m_active = true;
    }
    
    void End() {
        glQueryCounter(m_queries[1], GL_TIMESTAMP);
    }
    
    float GetElapsedMs() {
        GLint available = 0;
        while (!available) {
            glGetQueryObjectiv(m_queries[1], GL_QUERY_RESULT_AVAILABLE, &available);
        }
        
        GLuint64 startTime, endTime;
        glGetQueryObjectui64v(m_queries[0], GL_QUERY_RESULT, &startTime);
        glGetQueryObjectui64v(m_queries[1], GL_QUERY_RESULT, &endTime);
        
        return (endTime - startTime) / 1000000.0f;  // ns to ms
    }
};
```

### 9.2 셰이더 복잡도 분석
```cpp
struct ShaderMetrics {
    int instructionCount;
    int textureReads;
    int uniformReads;
    int varyingInterpolations;
    
    void Analyze(const std::string& glslSource) {
        // 명령어 카운트 (근사치)
        instructionCount = std::count(glslSource.begin(), glslSource.end(), ';');
        
        // 텍스처 읽기
        textureReads = CountOccurrences(glslSource, "texture");
        
        // 유니폼 접근
        uniformReads = CountOccurrences(glslSource, "uniform");
        
        // Varying 보간
        varyingInterpolations = CountOccurrences(glslSource, "in ");
    }
};
```

## 10. 메모리 관리

### 10.1 셰이더 리소스 풀
```cpp
class ShaderPool {
private:
    std::unordered_map<size_t, GLuint> m_programs;
    
public:
    GLuint GetOrCreate(const std::string& vertPath, const std::string& fragPath) {
        size_t hash = std::hash<std::string>{}(vertPath + fragPath);
        
        auto it = m_programs.find(hash);
        if (it != m_programs.end()) {
            return it->second;
        }
        
        GLuint program = CreateProgram(vertPath, fragPath);
        m_programs[hash] = program;
        return program;
    }
    
    void Clear() {
        for (auto& pair : m_programs) {
            glDeleteProgram(pair.second);
        }
        m_programs.clear();
    }
};
```

### 10.2 GPU 메모리 추적
```cpp
class GPUMemoryTracker {
private:
    struct Allocation {
        GLenum type;
        size_t size;
        std::string name;
    };
    
    std::unordered_map<GLuint, Allocation> m_allocations;
    size_t m_totalMemory = 0;
    
public:
    void TrackBuffer(GLuint buffer, size_t size, const std::string& name) {
        m_allocations[buffer] = {GL_BUFFER, size, name};
        m_totalMemory += size;
    }
    
    void TrackTexture(GLuint texture, size_t size, const std::string& name) {
        m_allocations[texture] = {GL_TEXTURE, size, name};
        m_totalMemory += size;
    }
    
    void Report() {
        std::cout << "GPU Memory Usage: " << (m_totalMemory / (1024*1024)) << " MB\n";
        for (const auto& [id, alloc] : m_allocations) {
            std::cout << "  " << alloc.name << ": " 
                     << (alloc.size / 1024) << " KB\n";
        }
    }
};
```

## 11. 테스트 및 검증

### 11.1 셰이더 단위 테스트
```cpp
class ShaderTest {
public:
    bool TestCompilation(const char* source, GLenum type) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);
        
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        
        glDeleteShader(shader);
        return success;
    }
    
    bool TestUniformLocation(GLuint program, const char* name) {
        GLint location = glGetUniformLocation(program, name);
        return location != -1;
    }
    
    bool TestAttributeLocation(GLuint program, const char* name) {
        GLint location = glGetAttribLocation(program, name);
        return location != -1;
    }
};
```

### 11.2 렌더링 검증
```cpp
bool ValidateRendering() {
    // 프레임버퍼 완전성 체크
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        return false;
    }
    
    // 픽셀 읽기 테스트
    GLubyte pixel[4];
    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    
    // OpenGL 에러 체크
    GLenum error = glGetError();
    return error == GL_NO_ERROR;
}
```

## 12. 구현 코드 템플릿

### 12.1 Grid Shader 구현
```glsl
// grid.vert
#version 330 core

layout(location = 0) in vec2 aPosition;

uniform mat4 uViewProj;
uniform mat4 uInvViewProj;

out vec2 vWorldPos;

void main() {
    gl_Position = vec4(aPosition * 2.0 - 1.0, 0.0, 1.0);
    vec4 worldPos = uInvViewProj * gl_Position;
    vWorldPos = worldPos.xy / worldPos.w;
}

// grid.frag
#version 330 core

in vec2 vWorldPos;

uniform float uGridSize;
uniform vec4 uGridColor;
uniform vec4 uSubGridColor;
uniform vec2 uCameraPos;
uniform float uZoom;

out vec4 fragColor;

float grid(vec2 pos, float size) {
    vec2 grid = abs(fract(pos / size - 0.5) - 0.5) / fwidth(pos / size);
    return 1.0 - min(min(grid.x, grid.y), 1.0);
}

void main() {
    float majorGrid = grid(vWorldPos, uGridSize * 10.0);
    float minorGrid = grid(vWorldPos, uGridSize);
    
    vec4 color = mix(uSubGridColor, uGridColor, majorGrid);
    float alpha = max(majorGrid, minorGrid * 0.5);
    
    // 거리 페이드
    float dist = length(vWorldPos - uCameraPos);
    float fadeStart = 100.0 / uZoom;
    float fadeEnd = 200.0 / uZoom;
    alpha *= 1.0 - smoothstep(fadeStart, fadeEnd, dist);
    
    fragColor = vec4(color.rgb, color.a * alpha);
}
```

### 12.2 ShaderProgram 클래스 구현
```cpp
class ShaderProgram {
private:
    GLuint m_program = 0;
    UniformCache m_uniformCache;
    
public:
    bool Load(const char* vertPath, const char* fragPath) {
        // 소스 로드
        ShaderSource vertSource, fragSource;
        if (!vertSource.LoadFromFile(vertPath) || 
            !fragSource.LoadFromFile(fragPath)) {
            return false;
        }
        
        // 컴파일
        GLuint vertShader = CompileShader(GL_VERTEX_SHADER, 
                                         vertSource.GetSource());
        GLuint fragShader = CompileShader(GL_FRAGMENT_SHADER, 
                                         fragSource.GetSource());
        
        if (!vertShader || !fragShader) {
            glDeleteShader(vertShader);
            glDeleteShader(fragShader);
            return false;
        }
        
        // 링킹
        m_program = LinkProgram(vertShader, fragShader);
        
        // 셰이더 정리
        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
        
        if (m_program) {
            m_uniformCache.CacheAllUniforms(m_program);
            return true;
        }
        
        return false;
    }
    
    void Use() {
        glUseProgram(m_program);
    }
    
    void SetUniform(const char* name, float value) {
        GLint loc = m_uniformCache.GetLocation(name);
        if (loc != -1) {
            glUniform1f(loc, value);
        }
    }
    
    void SetUniform(const char* name, const glm::mat4& value) {
        GLint loc = m_uniformCache.GetLocation(name);
        if (loc != -1) {
            glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(value));
        }
    }
};
```