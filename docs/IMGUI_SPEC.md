# Dear ImGui Integration Functional Specification

## 1. 개요

### 1.1 목적
NOT Gate 게임에 Dear ImGui를 통합하여 즉시 모드 GUI 시스템을 구축한다. 이를 통해 게임 UI, 디버그 도구, 레벨 에디터 등의 인터페이스를 효율적으로 개발한다.

### 1.2 범위
- SDL2 + OpenGL3 백엔드와의 통합
- 기본 UI 렌더링 파이프라인 구축
- 한글 입력 및 다국어 지원
- 도킹 및 멀티 뷰포트 기능

## 2. 시스템 아키텍처

### 2.1 컴포넌트 구조
```
Application
    ├── SDL2 Window
    ├── OpenGL Context
    └── ImGuiManager
        ├── Context Management
        ├── Backend Bindings (SDL2 + OpenGL3)
        ├── Frame Pipeline
        └── UIContext
            ├── Window States
            ├── Theme Settings
            └── Font Management
```

### 2.2 데이터 흐름
```
SDL Event → ImGui Process → Update UI State → Render ImGui → OpenGL Draw
```

## 3. 상세 기능 명세

### 3.1 ImGuiManager 클래스

#### 3.1.1 초기화 (Initialize)
```cpp
class ImGuiManager {
public:
    bool Initialize(SDL_Window* window, SDL_GLContext gl_context);
    void Shutdown();
    
private:
    ImGuiContext* m_context;
    ImGuiIO* m_io;
};
```

**초기화 순서:**
1. `ImGui::CreateContext()` - ImGui 컨텍스트 생성
2. `ImGui::GetIO()` - IO 설정 획득
3. IO 플래그 설정:
   - `io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard` - 키보드 네비게이션
   - `io.ConfigFlags |= ImGuiConfigFlags_DockingEnable` - 도킹 활성화
   - `io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable` - 멀티 뷰포트
4. `ImGui_ImplSDL2_InitForOpenGL(window, gl_context)` - SDL2 백엔드 초기화
5. `ImGui_ImplOpenGL3_Init("#version 330")` - OpenGL3 백엔드 초기화
6. 폰트 로드 및 설정
7. 테마 적용

#### 3.1.2 프레임 처리
```cpp
void BeginFrame();  // 새 프레임 시작
void EndFrame();    // 프레임 렌더링
void ProcessEvent(const SDL_Event& event);  // 이벤트 처리
```

**프레임 파이프라인:**
1. `ImGui_ImplOpenGL3_NewFrame()` - OpenGL 새 프레임
2. `ImGui_ImplSDL2_NewFrame()` - SDL2 새 프레임
3. `ImGui::NewFrame()` - ImGui 새 프레임
4. UI 요소 렌더링 (사용자 코드)
5. `ImGui::Render()` - 렌더 데이터 생성
6. `ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData())` - OpenGL 렌더링

### 3.2 UIContext 클래스

#### 3.2.1 윈도우 관리
```cpp
class UIContext {
public:
    void ShowMainMenu();
    void ShowToolbar();
    void ShowPropertyPanel();
    void ShowDebugWindow();
    
private:
    bool m_showDemo = false;
    bool m_showMetrics = false;
    std::map<std::string, WindowState> m_windows;
};
```

#### 3.2.2 윈도우 상태
```cpp
struct WindowState {
    bool isOpen;
    ImVec2 position;
    ImVec2 size;
    bool isDocked;
    std::string dockId;
};
```

### 3.3 폰트 시스템

#### 3.3.1 폰트 로드
```cpp
void LoadFonts() {
    ImGuiIO& io = ImGui::GetIO();
    
    // 기본 폰트
    io.Fonts->AddFontDefault();
    
    // 한글 폰트 (NotoSansCJK 또는 맑은 고딕)
    ImFontConfig config;
    config.MergeMode = true;
    config.PixelSnapH = true;
    
    static const ImWchar korean_ranges[] = {
        0x0020, 0x00FF,  // Basic Latin + Latin Supplement
        0x3131, 0x3163,  // Korean Jamo
        0xAC00, 0xD7A3,  // Korean Syllables
        0,
    };
    
    io.Fonts->AddFontFromFileTTF("assets/fonts/NotoSansCJK-Regular.ttc", 
                                  16.0f, &config, korean_ranges);
    
    // 아이콘 폰트 (FontAwesome)
    static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    config.MergeMode = true;
    config.GlyphMinAdvanceX = 16.0f;
    io.Fonts->AddFontFromFileTTF("assets/fonts/fontawesome.ttf", 
                                  16.0f, &config, icon_ranges);
}
```

### 3.4 테마 시스템

#### 3.4.1 테마 설정
```cpp
enum class Theme {
    Dark,
    Light,
    Classic
};

void ApplyTheme(Theme theme) {
    switch(theme) {
        case Theme::Dark:
            ImGui::StyleColorsDark();
            CustomizeDarkTheme();
            break;
        case Theme::Light:
            ImGui::StyleColorsLight();
            CustomizeLightTheme();
            break;
        case Theme::Classic:
            ImGui::StyleColorsClassic();
            break;
    }
}

void CustomizeDarkTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    // 색상 커스터마이징
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    
    // 스타일 설정
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.GrabRounding = 3.0f;
}
```

### 3.5 입력 처리

#### 3.5.1 이벤트 전달
```cpp
void ProcessEvent(const SDL_Event& event) {
    // ImGui로 이벤트 전달
    ImGui_ImplSDL2_ProcessEvent(&event);
    
    // ImGui가 입력을 캡처했는지 확인
    ImGuiIO& io = ImGui::GetIO();
    
    // 게임으로 이벤트 전달 여부 결정
    if (!io.WantCaptureMouse && event.type == SDL_MOUSEBUTTONDOWN) {
        // 게임 입력 처리
    }
    
    if (!io.WantCaptureKeyboard && event.type == SDL_KEYDOWN) {
        // 게임 키보드 입력 처리
    }
}
```

#### 3.5.2 한글 입력 (IME)
```cpp
void EnableIME() {
    SDL_StartTextInput();
    
    // IME 위치 설정
    SDL_Rect rect;
    rect.x = ImGui::GetCursorScreenPos().x;
    rect.y = ImGui::GetCursorScreenPos().y;
    rect.w = 100;
    rect.h = ImGui::GetTextLineHeight();
    SDL_SetTextInputRect(&rect);
}
```

### 3.6 도킹 시스템

#### 3.6.1 도킹 스페이스 설정
```cpp
void SetupDockSpace() {
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
    
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    
    ImGuiWindowFlags window_flags = 
        ImGuiWindowFlags_NoDocking | 
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | 
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;
    
    ImGui::Begin("DockSpace", nullptr, window_flags);
    
    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    
    // 초기 레이아웃 설정
    static bool first_time = true;
    if (first_time) {
        first_time = false;
        
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);
        
        auto dock_id_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.2f, nullptr, &dockspace_id);
        auto dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.2f, nullptr, &dockspace_id);
        auto dock_id_bottom = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.2f, nullptr, &dockspace_id);
        
        ImGui::DockBuilderDockWindow("Toolbar", dock_id_left);
        ImGui::DockBuilderDockWindow("Properties", dock_id_right);
        ImGui::DockBuilderDockWindow("Console", dock_id_bottom);
        ImGui::DockBuilderDockWindow("Viewport", dockspace_id);
        
        ImGui::DockBuilderFinish(dockspace_id);
    }
    
    ImGui::End();
}
```

## 4. 게임 특화 UI 컴포넌트

### 4.1 게이트 팔레트
```cpp
void ShowGatePalette() {
    if (ImGui::Begin("Gate Palette")) {
        if (ImGui::Button("NOT Gate", ImVec2(100, 50))) {
            // NOT 게이트 선택
            m_selectedTool = Tool::NotGate;
        }
        
        if (ImGui::Button("Wire", ImVec2(100, 50))) {
            // 와이어 도구 선택
            m_selectedTool = Tool::Wire;
        }
        
        ImGui::Separator();
        
        ImGui::Text("Selected: %s", GetToolName(m_selectedTool));
    }
    ImGui::End();
}
```

### 4.2 속성 패널
```cpp
void ShowPropertyPanel() {
    if (ImGui::Begin("Properties")) {
        if (m_selectedGate) {
            ImGui::Text("Gate ID: %d", m_selectedGate->id);
            ImGui::Text("Position: (%.0f, %.0f)", 
                       m_selectedGate->position.x, 
                       m_selectedGate->position.y);
            
            ImGui::Separator();
            
            ImGui::Text("Inputs:");
            for (int i = 0; i < 3; i++) {
                ImGui::Text("  [%d]: %s", i, 
                           m_selectedGate->inputs[i] ? "Connected" : "Empty");
            }
            
            ImGui::Text("Output: %s", 
                       m_selectedGate->output ? "Connected" : "Empty");
        } else {
            ImGui::Text("No gate selected");
        }
    }
    ImGui::End();
}
```

### 4.3 시뮬레이션 컨트롤
```cpp
void ShowSimulationControl() {
    if (ImGui::Begin("Simulation")) {
        // 재생/일시정지 버튼
        if (m_isSimulating) {
            if (ImGui::Button(ICON_FA_PAUSE " Pause")) {
                PauseSimulation();
            }
        } else {
            if (ImGui::Button(ICON_FA_PLAY " Play")) {
                StartSimulation();
            }
        }
        
        ImGui::SameLine();
        
        // 단계별 실행
        if (ImGui::Button(ICON_FA_STEP_FORWARD " Step")) {
            StepSimulation();
        }
        
        ImGui::SameLine();
        
        // 리셋
        if (ImGui::Button(ICON_FA_UNDO " Reset")) {
            ResetSimulation();
        }
        
        ImGui::Separator();
        
        // 시뮬레이션 속도
        ImGui::SliderFloat("Speed", &m_simulationSpeed, 0.1f, 10.0f);
        
        // 통계
        ImGui::Text("Gates: %d", m_circuit->GetGateCount());
        ImGui::Text("Wires: %d", m_circuit->GetWireCount());
        ImGui::Text("Active Signals: %d", m_circuit->GetActiveSignalCount());
        ImGui::Text("Simulation Time: %.2f s", m_simulationTime);
    }
    ImGui::End();
}
```

## 5. 성능 최적화

### 5.1 렌더링 최적화
- 가시 영역 외부 UI 요소 스킵
- 조건부 렌더링 (ImGui::Begin 반환값 확인)
- 정적 텍스트 캐싱

### 5.2 메모리 관리
```cpp
// 프레임당 메모리 예약
void ReserveMemory() {
    ImGui::GetIO().IniFilename = nullptr;  // INI 파일 비활성화
    ImGui::SetAllocatorFunctions(CustomAlloc, CustomFree, nullptr);
}
```

## 6. 디버그 도구

### 6.1 메트릭 윈도우
```cpp
void ShowMetricsWindow() {
    ImGui::ShowMetricsWindow(&m_showMetrics);
}
```

### 6.2 스타일 에디터
```cpp
void ShowStyleEditor() {
    ImGui::Begin("Style Editor");
    ImGui::ShowStyleEditor();
    ImGui::End();
}
```

## 7. 에러 처리

### 7.1 초기화 실패
```cpp
if (!ImGuiManager::Initialize(window, gl_context)) {
    SDL_ShowSimpleMessageBox(
        SDL_MESSAGEBOX_ERROR,
        "ImGui Error",
        "Failed to initialize ImGui",
        window
    );
    return false;
}
```

### 7.2 렌더링 에러
```cpp
try {
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
} catch (const std::exception& e) {
    LogError("ImGui render failed: %s", e.what());
    // 복구 시도
    RecreateContext();
}
```

## 8. 테스트 계획

### 8.1 단위 테스트
- ImGuiManager 초기화/종료
- 폰트 로드
- 테마 전환
- 이벤트 처리

### 8.2 통합 테스트
- SDL2 + OpenGL 통합
- 멀티 윈도우 렌더링
- 도킹 레이아웃 저장/복원
- 한글 입력

### 8.3 성능 테스트
- 1000개 UI 요소 렌더링
- 60 FPS 유지 확인
- 메모리 사용량 모니터링

## 9. 파일 구조

```
src/ui/
├── ImGuiManager.h       # ImGui 관리자 헤더
├── ImGuiManager.cpp     # ImGui 관리자 구현
├── UIContext.h          # UI 컨텍스트 헤더
├── UIContext.cpp        # UI 컨텍스트 구현
├── GameUI.h            # 게임 UI 헤더
├── GameUI.cpp          # 게임 UI 구현
└── Themes.h            # 테마 정의

extern/imgui/
├── imgui.h
├── imgui.cpp
├── imgui_demo.cpp
├── imgui_draw.cpp
├── imgui_tables.cpp
├── imgui_widgets.cpp
├── backends/
│   ├── imgui_impl_sdl2.h
│   ├── imgui_impl_sdl2.cpp
│   ├── imgui_impl_opengl3.h
│   └── imgui_impl_opengl3.cpp
└── misc/
    └── fonts/
        ├── fontawesome.ttf
        └── NotoSansCJK.ttc
```

## 10. CMake 설정

```cmake
# ImGui 소스 파일
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# ImGui 포함 디렉토리
target_include_directories(notgate3 PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# 소스 파일에 추가
target_sources(notgate3 PRIVATE
    ${IMGUI_SOURCES}
    src/ui/ImGuiManager.cpp
    src/ui/UIContext.cpp
    src/ui/GameUI.cpp
)
```

## 11. 구현 우선순위

1. **필수 (Phase 1)**
   - 기본 초기화/종료
   - SDL2 + OpenGL3 백엔드
   - 데모 윈도우 표시

2. **중요 (Phase 2)**
   - 게임 UI 컴포넌트
   - 한글 폰트 지원
   - 도킹 시스템

3. **선택 (Phase 3)**
   - 커스텀 테마
   - 고급 디버그 도구
   - 멀티 뷰포트