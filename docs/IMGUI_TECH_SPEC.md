# Dear ImGui Integration Technical Specification

## 1. 기술 스택

### 1.1 핵심 의존성
| 컴포넌트 | 버전 | 용도 |
|---------|------|------|
| Dear ImGui | 1.90.0+ | 즉시 모드 GUI 프레임워크 |
| SDL2 | 2.28.0+ | 윈도우 관리 및 이벤트 처리 |
| OpenGL | 3.3+ | 그래픽스 렌더링 |
| GLEW/GLAD | Latest | OpenGL 확장 로더 |

### 1.2 선택적 의존성
| 컴포넌트 | 버전 | 용도 |
|---------|------|------|
| stb_image | 2.28+ | 이미지 로딩 (아이콘, 텍스처) |
| IconFontCppHeaders | Latest | 아이콘 폰트 정의 |
| ImPlot | 0.16+ | 그래프 및 플롯 (성능 모니터링) |

## 2. 메모리 아키텍처

### 2.1 메모리 레이아웃
```cpp
// ImGui 메모리 할당 정렬
#define IMGUI_MALLOC_ALIGNMENT 16

// 메모리 풀 크기
constexpr size_t IMGUI_VERTEX_BUFFER_SIZE = 512 * 1024;    // 512KB
constexpr size_t IMGUI_INDEX_BUFFER_SIZE = 128 * 1024;     // 128KB
constexpr size_t IMGUI_UNIFORM_BUFFER_SIZE = 16 * 1024;    // 16KB

// 텍스처 아틀라스
constexpr int FONT_ATLAS_WIDTH = 2048;
constexpr int FONT_ATLAS_HEIGHT = 2048;
```

### 2.2 메모리 할당자
```cpp
class ImGuiMemoryAllocator {
private:
    struct MemoryPool {
        alignas(64) uint8_t buffer[1024 * 1024];  // 1MB 풀
        size_t offset;
        std::mutex mutex;
    };
    
    static MemoryPool s_pools[4];  // 4MB 총 메모리
    static std::atomic<int> s_currentPool;
    
public:
    static void* Alloc(size_t size, void* user_data) {
        // Round-robin 풀 선택
        int poolIndex = s_currentPool.fetch_add(1) % 4;
        MemoryPool& pool = s_pools[poolIndex];
        
        std::lock_guard<std::mutex> lock(pool.mutex);
        
        // 정렬
        size_t aligned_size = (size + 15) & ~15;
        
        if (pool.offset + aligned_size > sizeof(pool.buffer)) {
            // 풀 리셋 (프레임 경계에서)
            pool.offset = 0;
        }
        
        void* ptr = pool.buffer + pool.offset;
        pool.offset += aligned_size;
        
        return ptr;
    }
    
    static void Free(void* ptr, void* user_data) {
        // No-op: 프레임 단위로 전체 리셋
    }
};
```

## 3. 렌더링 파이프라인

### 3.1 OpenGL 상태 관리
```cpp
class ImGuiGLState {
private:
    // 백업할 OpenGL 상태
    struct GLStateBackup {
        GLenum last_active_texture;
        GLuint last_program;
        GLuint last_texture;
        GLuint last_sampler;
        GLuint last_array_buffer;
        GLuint last_vertex_array_object;
        GLint last_polygon_mode[2];
        GLint last_viewport[4];
        GLint last_scissor_box[4];
        GLenum last_blend_src_rgb;
        GLenum last_blend_dst_rgb;
        GLenum last_blend_src_alpha;
        GLenum last_blend_dst_alpha;
        GLenum last_blend_equation_rgb;
        GLenum last_blend_equation_alpha;
        GLboolean last_enable_blend;
        GLboolean last_enable_cull_face;
        GLboolean last_enable_depth_test;
        GLboolean last_enable_stencil_test;
        GLboolean last_enable_scissor_test;
    };
    
public:
    static void BackupGLState(GLStateBackup& backup) {
        glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&backup.last_active_texture);
        glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*)&backup.last_program);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&backup.last_texture);
        glGetIntegerv(GL_SAMPLER_BINDING, (GLint*)&backup.last_sampler);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint*)&backup.last_array_buffer);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint*)&backup.last_vertex_array_object);
        glGetIntegerv(GL_POLYGON_MODE, backup.last_polygon_mode);
        glGetIntegerv(GL_VIEWPORT, backup.last_viewport);
        glGetIntegerv(GL_SCISSOR_BOX, backup.last_scissor_box);
        glGetIntegerv(GL_BLEND_SRC_RGB, (GLint*)&backup.last_blend_src_rgb);
        glGetIntegerv(GL_BLEND_DST_RGB, (GLint*)&backup.last_blend_dst_rgb);
        glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint*)&backup.last_blend_src_alpha);
        glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint*)&backup.last_blend_dst_alpha);
        glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint*)&backup.last_blend_equation_rgb);
        glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint*)&backup.last_blend_equation_alpha);
        backup.last_enable_blend = glIsEnabled(GL_BLEND);
        backup.last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
        backup.last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
        backup.last_enable_stencil_test = glIsEnabled(GL_STENCIL_TEST);
        backup.last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);
    }
    
    static void RestoreGLState(const GLStateBackup& backup) {
        glUseProgram(backup.last_program);
        glBindTexture(GL_TEXTURE_2D, backup.last_texture);
        glBindSampler(0, backup.last_sampler);
        glActiveTexture(backup.last_active_texture);
        glBindVertexArray(backup.last_vertex_array_object);
        glBindBuffer(GL_ARRAY_BUFFER, backup.last_array_buffer);
        glBlendEquationSeparate(backup.last_blend_equation_rgb, backup.last_blend_equation_alpha);
        glBlendFuncSeparate(backup.last_blend_src_rgb, backup.last_blend_dst_rgb, 
                           backup.last_blend_src_alpha, backup.last_blend_dst_alpha);
        if (backup.last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
        if (backup.last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
        if (backup.last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        if (backup.last_enable_stencil_test) glEnable(GL_STENCIL_TEST); else glDisable(GL_STENCIL_TEST);
        if (backup.last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, backup.last_polygon_mode[0]);
        glViewport(backup.last_viewport[0], backup.last_viewport[1], 
                  backup.last_viewport[2], backup.last_viewport[3]);
        glScissor(backup.last_scissor_box[0], backup.last_scissor_box[1], 
                 backup.last_scissor_box[2], backup.last_scissor_box[3]);
    }
};
```

### 3.2 셰이더 프로그램
```cpp
class ImGuiShaderProgram {
private:
    GLuint m_shaderProgram;
    GLint m_attribLocationTex;
    GLint m_attribLocationProjMtx;
    GLint m_attribLocationVtxPos;
    GLint m_attribLocationVtxUV;
    GLint m_attribLocationVtxColor;
    
    static constexpr const char* VERTEX_SHADER = R"(
        #version 330 core
        layout (location = 0) in vec2 Position;
        layout (location = 1) in vec2 UV;
        layout (location = 2) in vec4 Color;
        
        uniform mat4 ProjMtx;
        
        out vec2 Frag_UV;
        out vec4 Frag_Color;
        
        void main() {
            Frag_UV = UV;
            Frag_Color = Color;
            gl_Position = ProjMtx * vec4(Position.xy, 0, 1);
        }
    )";
    
    static constexpr const char* FRAGMENT_SHADER = R"(
        #version 330 core
        in vec2 Frag_UV;
        in vec4 Frag_Color;
        
        uniform sampler2D Texture;
        
        layout (location = 0) out vec4 Out_Color;
        
        void main() {
            Out_Color = Frag_Color * texture(Texture, Frag_UV.st);
        }
    )";
    
public:
    bool Initialize() {
        // 셰이더 컴파일
        GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, VERTEX_SHADER);
        GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER);
        
        // 프로그램 링크
        m_shaderProgram = glCreateProgram();
        glAttachShader(m_shaderProgram, vertexShader);
        glAttachShader(m_shaderProgram, fragmentShader);
        glLinkProgram(m_shaderProgram);
        
        // 속성 위치 가져오기
        m_attribLocationTex = glGetUniformLocation(m_shaderProgram, "Texture");
        m_attribLocationProjMtx = glGetUniformLocation(m_shaderProgram, "ProjMtx");
        m_attribLocationVtxPos = glGetAttribLocation(m_shaderProgram, "Position");
        m_attribLocationVtxUV = glGetAttribLocation(m_shaderProgram, "UV");
        m_attribLocationVtxColor = glGetAttribLocation(m_shaderProgram, "Color");
        
        // 정리
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        
        return true;
    }
    
private:
    GLuint CompileShader(GLenum type, const char* source) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);
        
        // 에러 체크
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            throw std::runtime_error(std::string("Shader compilation failed: ") + infoLog);
        }
        
        return shader;
    }
};
```

### 3.3 버텍스 버퍼 관리
```cpp
class ImGuiVertexBuffer {
private:
    GLuint m_vboHandle;
    GLuint m_elementsHandle;
    GLuint m_vaoHandle;
    
    size_t m_vboSize;
    size_t m_elementsSize;
    
public:
    bool Initialize() {
        // VAO 생성
        glGenVertexArrays(1, &m_vaoHandle);
        
        // VBO 생성
        glGenBuffers(1, &m_vboHandle);
        glGenBuffers(1, &m_elementsHandle);
        
        // 초기 크기 할당
        m_vboSize = IMGUI_VERTEX_BUFFER_SIZE;
        m_elementsSize = IMGUI_INDEX_BUFFER_SIZE;
        
        glBindBuffer(GL_ARRAY_BUFFER, m_vboHandle);
        glBufferData(GL_ARRAY_BUFFER, m_vboSize, nullptr, GL_STREAM_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementsHandle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_elementsSize, nullptr, GL_STREAM_DRAW);
        
        return true;
    }
    
    void UploadData(const ImDrawData* draw_data) {
        // 버텍스 버퍼 크기 계산
        size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
        size_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
        
        // 버퍼 리사이즈 필요시
        if (vertex_size > m_vboSize) {
            m_vboSize = vertex_size + 5000 * sizeof(ImDrawVert);  // 여유분 추가
            glBindBuffer(GL_ARRAY_BUFFER, m_vboHandle);
            glBufferData(GL_ARRAY_BUFFER, m_vboSize, nullptr, GL_STREAM_DRAW);
        }
        
        if (index_size > m_elementsSize) {
            m_elementsSize = index_size + 10000 * sizeof(ImDrawIdx);  // 여유분 추가
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementsHandle);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_elementsSize, nullptr, GL_STREAM_DRAW);
        }
        
        // 데이터 업로드 (orphaning 기법)
        glBindBuffer(GL_ARRAY_BUFFER, m_vboHandle);
        glBufferData(GL_ARRAY_BUFFER, m_vboSize, nullptr, GL_STREAM_DRAW);
        ImDrawVert* vtx_dst = (ImDrawVert*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementsHandle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_elementsSize, nullptr, GL_STREAM_DRAW);
        ImDrawIdx* idx_dst = (ImDrawIdx*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        
        // 커맨드 리스트 복사
        for (int n = 0; n < draw_data->CmdListsCount; n++) {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            memcpy(vtx_dst, cmd_list->VtxBuffer.Data, 
                   cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(idx_dst, cmd_list->IdxBuffer.Data, 
                   cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            vtx_dst += cmd_list->VtxBuffer.Size;
            idx_dst += cmd_list->IdxBuffer.Size;
        }
        
        glUnmapBuffer(GL_ARRAY_BUFFER);
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
    }
    
    void SetupRenderState(ImDrawData* draw_data, int fb_width, int fb_height) {
        // OpenGL 상태 설정
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glEnable(GL_SCISSOR_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        
        // 뷰포트 설정
        glViewport(0, 0, fb_width, fb_height);
        
        // 투영 행렬 설정
        float L = draw_data->DisplayPos.x;
        float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
        float T = draw_data->DisplayPos.y;
        float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
        
        const float ortho_projection[4][4] = {
            { 2.0f/(R-L),   0.0f,         0.0f,   0.0f },
            { 0.0f,         2.0f/(T-B),   0.0f,   0.0f },
            { 0.0f,         0.0f,        -1.0f,   0.0f },
            { (R+L)/(L-R),  (T+B)/(B-T),  0.0f,   1.0f },
        };
        
        glUseProgram(m_shaderProgram);
        glUniform1i(m_attribLocationTex, 0);
        glUniformMatrix4fv(m_attribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);
        
        // VAO 바인딩
        glBindVertexArray(m_vaoHandle);
        
        // 버텍스 속성 설정
        glBindBuffer(GL_ARRAY_BUFFER, m_vboHandle);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementsHandle);
        
        glEnableVertexAttribArray(m_attribLocationVtxPos);
        glEnableVertexAttribArray(m_attribLocationVtxUV);
        glEnableVertexAttribArray(m_attribLocationVtxColor);
        
        glVertexAttribPointer(m_attribLocationVtxPos, 2, GL_FLOAT, GL_FALSE, 
                             sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, pos));
        glVertexAttribPointer(m_attribLocationVtxUV, 2, GL_FLOAT, GL_FALSE, 
                             sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, uv));
        glVertexAttribPointer(m_attribLocationVtxColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, 
                             sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, col));
    }
};
```

## 4. 이벤트 처리 시스템

### 4.1 이벤트 디스패처
```cpp
class ImGuiEventDispatcher {
private:
    struct EventMetrics {
        uint32_t mouseEvents = 0;
        uint32_t keyboardEvents = 0;
        uint32_t windowEvents = 0;
        std::chrono::microseconds totalProcessTime{0};
    };
    
    EventMetrics m_metrics;
    bool m_mousePressed[5] = {false};
    SDL_Cursor* m_mouseCursors[ImGuiMouseCursor_COUNT] = {nullptr};
    
public:
    bool ProcessEvent(const SDL_Event& event) {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        ImGuiIO& io = ImGui::GetIO();
        
        switch (event.type) {
            case SDL_MOUSEWHEEL:
                return ProcessMouseWheel(event.wheel);
                
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                return ProcessMouseButton(event.button);
                
            case SDL_MOUSEMOTION:
                return ProcessMouseMotion(event.motion);
                
            case SDL_TEXTINPUT:
                return ProcessTextInput(event.text);
                
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                return ProcessKeyboard(event.key);
                
            case SDL_WINDOWEVENT:
                return ProcessWindowEvent(event.window);
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        m_metrics.totalProcessTime += 
            std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        
        return false;
    }
    
private:
    bool ProcessMouseWheel(const SDL_MouseWheelEvent& event) {
        ImGuiIO& io = ImGui::GetIO();
        
        if (event.x > 0) io.MouseWheelH += 1;
        if (event.x < 0) io.MouseWheelH -= 1;
        if (event.y > 0) io.MouseWheel += 1;
        if (event.y < 0) io.MouseWheel -= 1;
        
        m_metrics.mouseEvents++;
        return io.WantCaptureMouse;
    }
    
    bool ProcessMouseButton(const SDL_MouseButtonEvent& event) {
        ImGuiIO& io = ImGui::GetIO();
        
        int button = -1;
        if (event.button == SDL_BUTTON_LEFT) button = 0;
        if (event.button == SDL_BUTTON_RIGHT) button = 1;
        if (event.button == SDL_BUTTON_MIDDLE) button = 2;
        if (event.button == SDL_BUTTON_X1) button = 3;
        if (event.button == SDL_BUTTON_X2) button = 4;
        
        if (button >= 0 && button < 5) {
            m_mousePressed[button] = (event.type == SDL_MOUSEBUTTONDOWN);
            io.MouseDown[button] = m_mousePressed[button];
        }
        
        m_metrics.mouseEvents++;
        return io.WantCaptureMouse;
    }
    
    bool ProcessKeyboard(const SDL_KeyboardEvent& event) {
        ImGuiIO& io = ImGui::GetIO();
        
        int key = event.keysym.scancode;
        io.KeysDown[key] = (event.type == SDL_KEYDOWN);
        
        io.KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
        io.KeyCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
        io.KeyAlt = ((SDL_GetModState() & KMOD_ALT) != 0);
        io.KeySuper = ((SDL_GetModState() & KMOD_GUI) != 0);
        
        m_metrics.keyboardEvents++;
        return io.WantCaptureKeyboard;
    }
};
```

### 4.2 입력 큐 시스템
```cpp
template<typename T, size_t SIZE>
class CircularBuffer {
private:
    alignas(64) T m_buffer[SIZE];
    std::atomic<size_t> m_head{0};
    std::atomic<size_t> m_tail{0};
    
public:
    bool push(const T& item) {
        size_t head = m_head.load(std::memory_order_relaxed);
        size_t next_head = (head + 1) % SIZE;
        
        if (next_head == m_tail.load(std::memory_order_acquire)) {
            return false;  // 버퍼 가득 참
        }
        
        m_buffer[head] = item;
        m_head.store(next_head, std::memory_order_release);
        return true;
    }
    
    bool pop(T& item) {
        size_t tail = m_tail.load(std::memory_order_relaxed);
        
        if (tail == m_head.load(std::memory_order_acquire)) {
            return false;  // 버퍼 비어있음
        }
        
        item = m_buffer[tail];
        m_tail.store((tail + 1) % SIZE, std::memory_order_release);
        return true;
    }
};

class ImGuiInputQueue {
private:
    CircularBuffer<SDL_Event, 256> m_eventQueue;
    
public:
    void QueueEvent(const SDL_Event& event) {
        if (!m_eventQueue.push(event)) {
            // 버퍼 오버플로우 처리
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, 
                       "ImGui event queue overflow");
        }
    }
    
    void ProcessQueue() {
        SDL_Event event;
        while (m_eventQueue.pop(event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
        }
    }
};
```

## 5. 폰트 시스템

### 5.1 폰트 아틀라스 관리
```cpp
class ImGuiFontAtlas {
private:
    struct FontRange {
        const ImWchar* ranges;
        float size;
        ImFontConfig config;
    };
    
    std::vector<FontRange> m_fontRanges;
    GLuint m_fontTexture;
    
public:
    bool BuildAtlas() {
        ImGuiIO& io = ImGui::GetIO();
        
        // 폰트 아틀라스 구성
        ImFontConfig config;
        config.OversampleH = 3;
        config.OversampleV = 1;
        config.PixelSnapH = true;
        
        // 기본 폰트
        const float fontSize = 16.0f;
        io.Fonts->AddFontDefault(&config);
        
        // 한글 범위 정의
        static const ImWchar korean_ranges[] = {
            0x0020, 0x00FF,    // Basic Latin + Latin Supplement
            0x3131, 0x3163,    // Korean Jamo
            0xAC00, 0xD7A3,    // Korean Syllables
            0x1100, 0x11FF,    // Hangul Jamo
            0x3130, 0x318F,    // Hangul Compatibility Jamo
            0xA960, 0xA97F,    // Hangul Jamo Extended-A
            0xD7B0, 0xD7FF,    // Hangul Jamo Extended-B
            0,
        };
        
        // 한글 폰트 추가
        config.MergeMode = true;
        io.Fonts->AddFontFromFileTTF("assets/fonts/NotoSansCJKkr-Regular.otf", 
                                     fontSize, &config, korean_ranges);
        
        // 아이콘 폰트
        static const ImWchar icon_ranges[] = { 0xe000, 0xf8ff, 0 };
        config.MergeMode = true;
        config.GlyphMinAdvanceX = fontSize;
        io.Fonts->AddFontFromFileTTF("assets/fonts/fontawesome-webfont.ttf", 
                                     fontSize, &config, icon_ranges);
        
        // 아틀라스 빌드
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        
        // OpenGL 텍스처 생성
        glGenTextures(1, &m_fontTexture);
        glBindTexture(GL_TEXTURE_2D, m_fontTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, 
                    GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        
        // 텍스처 ID 저장
        io.Fonts->SetTexID((ImTextureID)(intptr_t)m_fontTexture);
        
        return true;
    }
    
    void OptimizeAtlas() {
        ImGuiIO& io = ImGui::GetIO();
        
        // 사용되지 않는 글리프 제거
        io.Fonts->ClearInputData();
        io.Fonts->ClearTexData();
        
        // 메모리 압축
        io.Fonts->ClearFonts();
    }
};
```

### 5.2 동적 폰트 로딩
```cpp
class ImGuiDynamicFontLoader {
private:
    struct FontRequest {
        std::string path;
        float size;
        std::vector<ImWchar> ranges;
        std::promise<ImFont*> promise;
    };
    
    std::queue<FontRequest> m_requests;
    std::mutex m_requestMutex;
    std::thread m_loaderThread;
    std::atomic<bool> m_running{true};
    
public:
    std::future<ImFont*> LoadFontAsync(const std::string& path, float size) {
        FontRequest request;
        request.path = path;
        request.size = size;
        
        auto future = request.promise.get_future();
        
        {
            std::lock_guard<std::mutex> lock(m_requestMutex);
            m_requests.push(std::move(request));
        }
        
        return future;
    }
    
private:
    void LoaderThread() {
        while (m_running) {
            FontRequest request;
            
            {
                std::lock_guard<std::mutex> lock(m_requestMutex);
                if (m_requests.empty()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                request = std::move(m_requests.front());
                m_requests.pop();
            }
            
            // 메인 스레드에서 실행되어야 함
            // 실제 구현에서는 메인 스레드 큐에 추가
            ImFont* font = LoadFontInternal(request.path, request.size);
            request.promise.set_value(font);
        }
    }
    
    ImFont* LoadFontInternal(const std::string& path, float size) {
        ImGuiIO& io = ImGui::GetIO();
        ImFont* font = io.Fonts->AddFontFromFileTTF(path.c_str(), size);
        
        // 아틀라스 재빌드
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        
        // 텍스처 업데이트...
        
        return font;
    }
};
```

## 6. 성능 최적화

### 6.1 컬링 시스템
```cpp
class ImGuiCullingSystem {
private:
    struct ViewportInfo {
        ImVec2 pos;
        ImVec2 size;
    };
    
public:
    bool IsVisible(const ImVec2& min, const ImVec2& max, const ViewportInfo& viewport) {
        // AABB 컬링
        if (max.x < viewport.pos.x || min.x > viewport.pos.x + viewport.size.x)
            return false;
        if (max.y < viewport.pos.y || min.y > viewport.pos.y + viewport.size.y)
            return false;
        return true;
    }
    
    void OptimizeDrawList(ImDrawList* draw_list) {
        const ImVec2 clip_min = draw_list->GetClipRectMin();
        const ImVec2 clip_max = draw_list->GetClipRectMax();
        
        ViewportInfo viewport;
        viewport.pos = clip_min;
        viewport.size = ImVec2(clip_max.x - clip_min.x, clip_max.y - clip_min.y);
        
        // 보이지 않는 요소 제거
        auto& vtx_buffer = draw_list->VtxBuffer;
        auto& idx_buffer = draw_list->IdxBuffer;
        auto& cmd_buffer = draw_list->CmdBuffer;
        
        // 명령별 처리
        for (auto& cmd : cmd_buffer) {
            if (cmd.UserCallback) continue;
            
            // 클립 영역 확인
            ImVec2 cmd_min(cmd.ClipRect.x, cmd.ClipRect.y);
            ImVec2 cmd_max(cmd.ClipRect.z, cmd.ClipRect.w);
            
            if (!IsVisible(cmd_min, cmd_max, viewport)) {
                cmd.ElemCount = 0;  // 렌더링 스킵
            }
        }
    }
};
```

### 6.2 배치 렌더러
```cpp
class ImGuiBatchRenderer {
private:
    struct DrawBatch {
        GLuint textureId;
        std::vector<ImDrawVert> vertices;
        std::vector<ImDrawIdx> indices;
        ImVec4 clipRect;
    };
    
    std::unordered_map<GLuint, DrawBatch> m_batches;
    
public:
    void AddDrawCall(const ImDrawCmd& cmd, const ImDrawVert* vtx, const ImDrawIdx* idx) {
        GLuint texId = (GLuint)(intptr_t)cmd.GetTexID();
        
        auto& batch = m_batches[texId];
        batch.textureId = texId;
        batch.clipRect = cmd.ClipRect;
        
        // 버텍스 복사
        size_t vtx_start = batch.vertices.size();
        batch.vertices.resize(vtx_start + cmd.VtxOffset);
        memcpy(&batch.vertices[vtx_start], vtx, cmd.VtxOffset * sizeof(ImDrawVert));
        
        // 인덱스 복사 및 오프셋 조정
        size_t idx_start = batch.indices.size();
        batch.indices.resize(idx_start + cmd.ElemCount);
        for (unsigned int i = 0; i < cmd.ElemCount; i++) {
            batch.indices[idx_start + i] = idx[cmd.IdxOffset + i] + vtx_start;
        }
    }
    
    void Render() {
        for (auto& [texId, batch] : m_batches) {
            // 텍스처 바인딩
            glBindTexture(GL_TEXTURE_2D, batch.textureId);
            
            // 클립 영역 설정
            glScissor((int)batch.clipRect.x, (int)batch.clipRect.y,
                     (int)(batch.clipRect.z - batch.clipRect.x),
                     (int)(batch.clipRect.w - batch.clipRect.y));
            
            // 드로우 콜
            glDrawElements(GL_TRIANGLES, (GLsizei)batch.indices.size(),
                          sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                          batch.indices.data());
        }
        
        m_batches.clear();
    }
};
```

## 7. 멀티스레딩 지원

### 7.1 스레드 안전 컨텍스트
```cpp
class ImGuiThreadSafeContext {
private:
    std::mutex m_contextMutex;
    ImGuiContext* m_mainContext;
    thread_local ImGuiContext* t_localContext = nullptr;
    
public:
    void MakeContextCurrent() {
        std::lock_guard<std::mutex> lock(m_contextMutex);
        
        if (!t_localContext) {
            // 스레드별 컨텍스트 생성
            t_localContext = ImGui::CreateContext(m_mainContext->FontAtlas);
        }
        
        ImGui::SetCurrentContext(t_localContext);
    }
    
    void MergeContexts() {
        std::lock_guard<std::mutex> lock(m_contextMutex);
        
        // 모든 스레드 컨텍스트를 메인 컨텍스트로 병합
        // 실제 구현에서는 draw list 병합 등 필요
    }
};
```

## 8. 디버깅 및 프로파일링

### 8.1 성능 모니터
```cpp
class ImGuiPerformanceMonitor {
private:
    struct FrameMetrics {
        float frameTime;
        float renderTime;
        float updateTime;
        int drawCalls;
        int triangles;
        size_t memoryUsage;
    };
    
    CircularBuffer<FrameMetrics, 120> m_frameHistory;
    
public:
    void RecordFrame(const FrameMetrics& metrics) {
        m_frameHistory.push(metrics);
    }
    
    void ShowMetricsWindow() {
        if (ImGui::Begin("Performance Metrics")) {
            // FPS 그래프
            float frameTimes[120];
            size_t count = 0;
            FrameMetrics metrics;
            while (m_frameHistory.pop(metrics) && count < 120) {
                frameTimes[count++] = metrics.frameTime;
            }
            
            ImGui::PlotLines("Frame Time", frameTimes, count, 0, nullptr, 
                           0.0f, 33.33f, ImVec2(0, 80));
            
            // 통계
            ImGui::Text("Average FPS: %.1f", 1000.0f / GetAverageFrameTime());
            ImGui::Text("Draw Calls: %d", GetLastDrawCalls());
            ImGui::Text("Triangles: %d", GetLastTriangles());
            ImGui::Text("Memory: %.2f MB", GetMemoryUsage() / 1024.0f / 1024.0f);
        }
        ImGui::End();
    }
};
```

## 9. 플랫폼 특화 최적화

### 9.1 Windows 최적화
```cpp
#ifdef _WIN32
class ImGuiWindowsPlatform {
public:
    void EnableDPIAwareness() {
        SetProcessDPIAware();
        
        // Windows 10 1703+
        typedef BOOL(WINAPI *SetProcessDpiAwarenessContext_t)(HANDLE);
        SetProcessDpiAwarenessContext_t SetProcessDpiAwarenessContextFunc = 
            (SetProcessDpiAwarenessContext_t)GetProcAddress(
                GetModuleHandleA("user32.dll"), 
                "SetProcessDpiAwarenessContext");
        
        if (SetProcessDpiAwarenessContextFunc) {
            SetProcessDpiAwarenessContextFunc(
                DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        }
    }
    
    float GetDPIScale(SDL_Window* window) {
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(window, &wmInfo);
        
        HDC hdc = GetDC(wmInfo.info.win.window);
        float dpi = (float)GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(wmInfo.info.win.window, hdc);
        
        return dpi / 96.0f;
    }
};
#endif
```

### 9.2 Linux 최적화
```cpp
#ifdef __linux__
class ImGuiLinuxPlatform {
public:
    void SetupX11Backend(SDL_Window* window) {
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(window, &wmInfo);
        
        Display* display = wmInfo.info.x11.display;
        Window x11_window = wmInfo.info.x11.window;
        
        // X11 특화 설정
        XSynchronize(display, False);
        
        // Compositing 최적화
        Atom bypass = XInternAtom(display, "_NET_WM_BYPASS_COMPOSITOR", False);
        long value = 1;
        XChangeProperty(display, x11_window, bypass, XA_CARDINAL, 32,
                       PropModeReplace, (unsigned char*)&value, 1);
    }
};
#endif
```

## 10. 에러 복구 시스템

### 10.1 컨텍스트 복구
```cpp
class ImGuiContextRecovery {
private:
    struct ContextBackup {
        ImGuiStyle style;
        ImGuiIO io_config;
        std::vector<ImFont*> fonts;
    };
    
    ContextBackup m_backup;
    
public:
    void BackupContext() {
        ImGuiContext* ctx = ImGui::GetCurrentContext();
        if (!ctx) return;
        
        m_backup.style = ImGui::GetStyle();
        m_backup.io_config = ImGui::GetIO();
        
        // 폰트 백업
        ImGuiIO& io = ImGui::GetIO();
        for (int i = 0; i < io.Fonts->Fonts.Size; i++) {
            m_backup.fonts.push_back(io.Fonts->Fonts[i]);
        }
    }
    
    bool RecoverContext() {
        try {
            // 기존 컨텍스트 삭제
            if (ImGui::GetCurrentContext()) {
                ImGui::DestroyContext();
            }
            
            // 새 컨텍스트 생성
            ImGui::CreateContext();
            
            // 설정 복원
            ImGui::GetStyle() = m_backup.style;
            
            // IO 설정 복원 (일부만)
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags = m_backup.io_config.ConfigFlags;
            io.IniFilename = m_backup.io_config.IniFilename;
            
            // 백엔드 재초기화 필요
            return true;
            
        } catch (const std::exception& e) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                        "Failed to recover ImGui context: %s", e.what());
            return false;
        }
    }
};
```

## 11. 테스트 스위트

### 11.1 자동화 테스트
```cpp
class ImGuiTestSuite {
public:
    bool RunAllTests() {
        bool success = true;
        
        success &= TestInitialization();
        success &= TestFontLoading();
        success &= TestEventProcessing();
        success &= TestRendering();
        success &= TestMemoryLeaks();
        
        return success;
    }
    
private:
    bool TestInitialization() {
        // SDL 윈도우 생성
        SDL_Window* window = SDL_CreateWindow("Test", 
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
        
        if (!window) return false;
        
        SDL_GLContext gl_context = SDL_GL_CreateContext(window);
        if (!gl_context) {
            SDL_DestroyWindow(window);
            return false;
        }
        
        // ImGui 초기화
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        
        bool result = ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
        result &= ImGui_ImplOpenGL3_Init("#version 330");
        
        // 정리
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        
        SDL_GL_DeleteContext(gl_context);
        SDL_DestroyWindow(window);
        
        return result;
    }
    
    bool TestMemoryLeaks() {
        // Valgrind 또는 AddressSanitizer와 함께 실행
        size_t initial_memory = GetCurrentMemoryUsage();
        
        // 100회 반복 생성/삭제
        for (int i = 0; i < 100; i++) {
            ImGui::CreateContext();
            ImGui::DestroyContext();
        }
        
        size_t final_memory = GetCurrentMemoryUsage();
        
        // 메모리 증가량 확인 (허용 오차: 1MB)
        return (final_memory - initial_memory) < 1024 * 1024;
    }
};
```

## 12. 빌드 설정

### 12.1 CMake 구성
```cmake
# ImGui 설정
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui)

# 컴파일 옵션
if(MSVC)
    add_compile_options(/W4 /WX)
    add_compile_options(/arch:AVX2)  # SIMD 최적화
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
    add_compile_options(-march=native)  # CPU 최적화
endif()

# ImGui 라이브러리
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(imgui PUBLIC
    SDL2::SDL2
    ${OPENGL_LIBRARIES}
)

# 프리컴파일 헤더
target_precompile_headers(imgui PRIVATE
    <imgui.h>
    <imgui_internal.h>
)

# Unity 빌드
set_target_properties(imgui PROPERTIES UNITY_BUILD ON)
```

## 13. 배포 체크리스트

### 13.1 릴리즈 준비
- [ ] 모든 디버그 로그 제거
- [ ] 최적화 플래그 활성화 (-O3, /O2)
- [ ] 심볼 스트리핑
- [ ] 폰트 파일 압축
- [ ] 불필요한 데모 코드 제거
- [ ] 메모리 누수 검사 통과
- [ ] 성능 벤치마크 실행
- [ ] 플랫폼별 테스트 완료