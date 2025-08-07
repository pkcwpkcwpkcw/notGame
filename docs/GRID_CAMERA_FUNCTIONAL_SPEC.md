# Grid Rendering & Camera System Functional Specification

## 1. 시스템 개요

### 1.1 목적
NOT Gate 게임의 그리드 시각화 및 카메라 제어 시스템을 구현하여 사용자가 회로를 효과적으로 구축하고 탐색할 수 있도록 한다.

### 1.2 범위
- **그리드 렌더링**: 게임 공간의 시각적 표현
- **카메라 시스템**: 뷰포트 제어 및 좌표 변환
- **상호작용**: 마우스/키보드 입력 처리

## 2. 기능 명세

### 2.1 GridRenderer 클래스

#### 2.1.1 클래스 정의
```cpp
class GridRenderer {
public:
    // 생성자/소멸자
    GridRenderer();
    ~GridRenderer();
    
    // 초기화
    bool Initialize(int screenWidth, int screenHeight);
    void Shutdown();
    
    // 렌더링
    void Render(const Camera& camera);
    void RenderGrid();
    void RenderHighlights();
    
    // 상태 관리
    void SetGridVisible(bool visible);
    void SetGridOpacity(float opacity);
    void SetCellSize(float size);
    
    // 셀 상호작용
    void SetHoveredCell(const glm::ivec2& cell);
    void SetSelectedCells(const std::vector<glm::ivec2>& cells);
    void ClearSelection();
    
    // 화면 크기 변경
    void OnResize(int width, int height);
    
private:
    // OpenGL 리소스
    GLuint m_gridVAO;
    GLuint m_gridVBO;
    GLuint m_highlightVAO;
    GLuint m_highlightVBO;
    GLuint m_shaderProgram;
    
    // 유니폼 위치
    GLint m_viewProjMatrixLoc;
    GLint m_gridColorLoc;
    GLint m_gridOpacityLoc;
    
    // 렌더링 상태
    bool m_isGridVisible;
    float m_gridOpacity;
    float m_cellSize;
    
    // 선택 상태
    glm::ivec2 m_hoveredCell;
    std::vector<glm::ivec2> m_selectedCells;
    
    // 내부 함수
    void CreateGridMesh();
    void CreateHighlightMesh();
    bool CompileShaders();
    void UpdateGridBuffer(const Camera& camera);
    void UpdateHighlightBuffer();
};
```

#### 2.1.2 주요 메서드 상세

##### Initialize()
```cpp
bool Initialize(int screenWidth, int screenHeight)
{
    // 1. 셰이더 컴파일 및 링크
    if (!CompileShaders()) {
        return false;
    }
    
    // 2. VAO/VBO 생성
    CreateGridMesh();
    CreateHighlightMesh();
    
    // 3. 초기 상태 설정
    m_isGridVisible = true;
    m_gridOpacity = 0.5f;
    m_cellSize = 32.0f;
    
    return true;
}
```

##### Render()
```cpp
void Render(const Camera& camera)
{
    if (!m_isGridVisible) return;
    
    // 1. 셰이더 활성화
    glUseProgram(m_shaderProgram);
    
    // 2. 뷰-프로젝션 매트릭스 설정
    glm::mat4 viewProj = camera.GetProjectionMatrix() * camera.GetViewMatrix();
    glUniformMatrix4fv(m_viewProjMatrixLoc, 1, GL_FALSE, &viewProj[0][0]);
    
    // 3. 그리드 렌더링
    RenderGrid();
    
    // 4. 하이라이트 렌더링
    RenderHighlights();
}
```

### 2.2 Camera 클래스

#### 2.2.1 클래스 정의
```cpp
class Camera {
public:
    // 생성자
    Camera(int screenWidth, int screenHeight);
    
    // 뷰/프로젝션 매트릭스
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;
    glm::mat4 GetViewProjectionMatrix() const;
    
    // 좌표 변환
    glm::vec2 ScreenToWorld(const glm::vec2& screenPos) const;
    glm::vec2 WorldToScreen(const glm::vec2& worldPos) const;
    glm::ivec2 ScreenToGrid(const glm::vec2& screenPos) const;
    glm::vec2 GridToScreen(const glm::ivec2& gridPos) const;
    
    // 카메라 조작
    void Pan(const glm::vec2& delta);
    void Zoom(float factor, const glm::vec2& screenPos);
    void Reset();
    
    // 속성 설정/조회
    void SetPosition(const glm::vec2& position);
    glm::vec2 GetPosition() const { return m_position; }
    
    void SetZoom(float zoom);
    float GetZoom() const { return m_zoom; }
    
    void SetScreenSize(int width, int height);
    glm::vec2 GetScreenSize() const { return m_screenSize; }
    
    // 뷰포트 정보
    glm::vec4 GetVisibleBounds() const;
    bool IsGridCellVisible(const glm::ivec2& gridPos) const;
    
private:
    // 카메라 상태
    glm::vec2 m_position;      // 월드 좌표 (그리드 단위)
    float m_zoom;               // 줌 레벨
    glm::vec2 m_screenSize;     // 화면 크기
    
    // 제한값
    static constexpr float MIN_ZOOM = 0.1f;
    static constexpr float MAX_ZOOM = 10.0f;
    static constexpr float CELL_SIZE = 32.0f;
    
    // 내부 계산
    float GetPixelsPerGrid() const { return CELL_SIZE * m_zoom; }
};
```

#### 2.2.2 주요 메서드 상세

##### 좌표 변환 메서드
```cpp
glm::vec2 ScreenToWorld(const glm::vec2& screenPos) const
{
    // 화면 중심 기준 오프셋 계산
    glm::vec2 offset = screenPos - m_screenSize * 0.5f;
    
    // 줌 레벨 적용 및 월드 좌표 변환
    return m_position + offset / (CELL_SIZE * m_zoom);
}

glm::ivec2 ScreenToGrid(const glm::vec2& screenPos) const
{
    glm::vec2 worldPos = ScreenToWorld(screenPos);
    return glm::ivec2(floor(worldPos.x), floor(worldPos.y));
}
```

##### 카메라 조작 메서드
```cpp
void Pan(const glm::vec2& delta)
{
    // 스크린 델타를 월드 델타로 변환
    glm::vec2 worldDelta = delta / (CELL_SIZE * m_zoom);
    m_position -= worldDelta;  // 반대 방향으로 이동
}

void Zoom(float factor, const glm::vec2& screenPos)
{
    // 이전 마우스 위치의 월드 좌표
    glm::vec2 worldBeforeZoom = ScreenToWorld(screenPos);
    
    // 줌 적용
    m_zoom = glm::clamp(m_zoom * factor, MIN_ZOOM, MAX_ZOOM);
    
    // 줌 후 마우스 위치의 월드 좌표
    glm::vec2 worldAfterZoom = ScreenToWorld(screenPos);
    
    // 마우스 위치가 같은 월드 좌표를 가리키도록 카메라 위치 조정
    m_position += worldBeforeZoom - worldAfterZoom;
}
```

### 2.3 입력 처리 시스템

#### 2.3.1 InputHandler 클래스
```cpp
class InputHandler {
public:
    InputHandler(Camera& camera, GridRenderer& gridRenderer);
    
    // SDL 이벤트 처리
    void HandleEvent(const SDL_Event& event);
    
    // 마우스 이벤트
    void OnMouseMove(int x, int y);
    void OnMouseDown(int button, int x, int y);
    void OnMouseUp(int button, int x, int y);
    void OnMouseWheel(float delta);
    
    // 키보드 이벤트
    void OnKeyDown(SDL_Keycode key);
    void OnKeyUp(SDL_Keycode key);
    
private:
    Camera& m_camera;
    GridRenderer& m_gridRenderer;
    
    // 마우스 상태
    bool m_isPanning;
    glm::vec2 m_lastMousePos;
    glm::vec2 m_panStartPos;
    
    // 선택 상태
    bool m_isSelecting;
    glm::ivec2 m_selectionStart;
    std::vector<glm::ivec2> m_selectedCells;
    
    // 키보드 상태
    bool m_ctrlPressed;
    bool m_shiftPressed;
};
```

#### 2.3.2 이벤트 처리 구현
```cpp
void OnMouseMove(int x, int y)
{
    glm::vec2 mousePos(x, y);
    
    // 호버 셀 업데이트
    glm::ivec2 gridPos = m_camera.ScreenToGrid(mousePos);
    m_gridRenderer.SetHoveredCell(gridPos);
    
    // 팬 처리
    if (m_isPanning) {
        glm::vec2 delta = mousePos - m_lastMousePos;
        m_camera.Pan(delta);
    }
    
    // 선택 영역 업데이트
    if (m_isSelecting) {
        UpdateSelection(gridPos);
    }
    
    m_lastMousePos = mousePos;
}

void OnMouseWheel(float delta)
{
    // 마우스 위치 중심으로 줌
    float zoomFactor = delta > 0 ? 1.1f : 0.9f;
    m_camera.Zoom(zoomFactor, m_lastMousePos);
}
```

### 2.4 셰이더 프로그램

#### 2.4.1 Vertex Shader (grid.vert)
```glsl
#version 330 core

layout(location = 0) in vec2 aPosition;
layout(location = 1) in float aIntensity;

uniform mat4 uViewProjMatrix;
uniform vec2 uGridOffset;
uniform float uGridScale;

out float vIntensity;

void main()
{
    vec2 worldPos = aPosition * uGridScale + uGridOffset;
    gl_Position = uViewProjMatrix * vec4(worldPos, 0.0, 1.0);
    vIntensity = aIntensity;
}
```

#### 2.4.2 Fragment Shader (grid.frag)
```glsl
#version 330 core

in float vIntensity;

uniform vec4 uGridColor;
uniform float uGridOpacity;

out vec4 FragColor;

void main()
{
    vec4 color = uGridColor;
    color.a *= uGridOpacity * vIntensity;
    
    // 안티앨리어싱
    float dist = length(gl_PointCoord - vec2(0.5));
    color.a *= smoothstep(0.5, 0.4, dist);
    
    FragColor = color;
}
```

## 3. 데이터 흐름

### 3.1 렌더링 파이프라인
```
Frame Start
    ↓
Update Camera Position/Zoom
    ↓
Calculate Visible Grid Range
    ↓
Update Grid Vertex Buffer (if needed)
    ↓
Set Shader Uniforms
    ↓
Draw Grid Lines
    ↓
Draw Cell Highlights
    ↓
Draw Hover Effect
    ↓
Frame End
```

### 3.2 입력 처리 흐름
```
SDL Event
    ↓
InputHandler::HandleEvent()
    ↓
Event Type Dispatch
    ├─ Mouse Move → Update Hover/Pan
    ├─ Mouse Click → Select Cell
    ├─ Mouse Wheel → Zoom
    └─ Keyboard → Camera Control
    ↓
Update Camera/Grid State
    ↓
Request Redraw
```

## 4. 성능 최적화 전략

### 4.1 Frustum Culling
```cpp
void GridRenderer::UpdateGridBuffer(const Camera& camera)
{
    glm::vec4 bounds = camera.GetVisibleBounds();
    
    // 가시 영역 내 그리드만 버퍼에 추가
    int startX = floor(bounds.x);
    int endX = ceil(bounds.z);
    int startY = floor(bounds.y);
    int endY = ceil(bounds.w);
    
    // 최대 렌더링 그리드 수 제한
    const int MAX_GRID_LINES = 1000;
    if ((endX - startX) * (endY - startY) > MAX_GRID_LINES) {
        // LOD 적용: 일부 그리드선 생략
        ApplyLOD(startX, endX, startY, endY);
    }
}
```

### 4.2 배치 렌더링
```cpp
void GridRenderer::RenderGrid()
{
    // 모든 그리드선을 단일 드로우콜로 렌더링
    glBindVertexArray(m_gridVAO);
    glDrawArrays(GL_LINES, 0, m_gridVertexCount);
}
```

### 4.3 동적 LOD
```cpp
void GridRenderer::ApplyLOD(int& startX, int& endX, int& startY, int& endY)
{
    float zoom = m_camera.GetZoom();
    
    if (zoom < 0.25f) {
        // 매우 축소: 10칸마다 표시
        m_gridStep = 10;
    } else if (zoom < 0.5f) {
        // 축소: 5칸마다 표시
        m_gridStep = 5;
    } else {
        // 기본: 모든 칸 표시
        m_gridStep = 1;
    }
}
```

## 5. 에러 처리

### 5.1 초기화 실패
```cpp
if (!GridRenderer::Initialize()) {
    SDL_LogError(SDL_LOG_CATEGORY_RENDER, 
                 "Failed to initialize grid renderer");
    // 폴백: 기본 렌더링 모드
    UseBasicRenderingMode();
}
```

### 5.2 셰이더 컴파일 에러
```cpp
bool CompileShader(GLuint shader, const char* source)
{
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, 
                     "Shader compilation failed: %s", infoLog);
        return false;
    }
    
    return true;
}
```

## 6. 테스트 시나리오

### 6.1 기능 테스트
1. **그리드 렌더링**
   - 그리드가 화면에 표시되는가
   - 그리드선 색상과 투명도가 올바른가
   - 축 라인이 구분되어 표시되는가

2. **카메라 이동**
   - 마우스 드래그로 팬이 동작하는가
   - 키보드 화살표로 이동이 가능한가
   - 이동이 부드러운가

3. **줌 기능**
   - 마우스 휠로 줌이 동작하는가
   - 마우스 위치 중심으로 줌이 되는가
   - 줌 한계값이 적용되는가

4. **좌표 변환**
   - 마우스 위치가 올바른 그리드 셀로 변환되는가
   - 선택한 셀이 정확히 하이라이트되는가

### 6.2 성능 테스트
1. **프레임레이트**
   - 기본 줌에서 60 FPS 유지
   - 최대 축소에서 30 FPS 이상

2. **메모리 사용**
   - 그리드 버퍼 < 10MB
   - 메모리 누수 없음

3. **대규모 그리드**
   - 1000x1000 그리드 표시 가능
   - Frustum culling 동작 확인

## 7. 구현 일정

### Phase 1: 기본 구현 (2일)
- [ ] GridRenderer 클래스 구현
- [ ] 기본 그리드 렌더링
- [ ] 고정 카메라 좌표 변환

### Phase 2: 카메라 시스템 (2일)
- [ ] Camera 클래스 구현
- [ ] 팬/줌 기능
- [ ] 입력 처리 시스템

### Phase 3: 최적화 (1일)
- [ ] Frustum culling
- [ ] LOD 시스템
- [ ] 성능 프로파일링

### Phase 4: 통합 테스트 (1일)
- [ ] 전체 시스템 테스트
- [ ] 버그 수정
- [ ] 문서 업데이트