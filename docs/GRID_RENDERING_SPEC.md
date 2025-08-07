# Grid Rendering Requirements Specification

## Step 3-2 & 3-5: 그리드 렌더링 및 카메라 시스템

### 목적
- **Step 3-2**: 게임의 핵심인 그리드 시스템을 시각적으로 렌더링
- **Step 3-5**: 카메라 시스템을 구현하여 대규모 회로 탐색 지원

### 기능 요구사항

#### 1. 그리드 구조
- **셀 크기**: 32x32 픽셀 (기본값, 줌 레벨에 따라 조정)
- **그리드 범위**: 무한 그리드 (가상 좌표계)
- **원점**: 화면 중앙을 (0, 0)으로 설정
- **좌표 시스템**: 
  - 월드 좌표: 그리드 셀 단위 (정수)
  - 스크린 좌표: 픽셀 단위

#### 2. 렌더링 요소

##### 2.1 그리드 라인
- **주 그리드선**: 1픽셀 두께, 회색 (rgba: 60, 60, 60, 255)
- **보조 그리드선**: 10칸마다 강조선 (rgba: 80, 80, 80, 255)
- **축 라인**: X/Y 축 표시 (rgba: 100, 100, 100, 255)

##### 2.2 셀 하이라이트
- **호버 효과**: 마우스 위치의 셀 강조 (rgba: 100, 100, 100, 50)
- **선택 표시**: 선택된 셀들 표시 (rgba: 100, 150, 255, 80)

##### 2.3 줌 레벨별 렌더링
- **줌 아웃 (< 0.5x)**: 그리드선 생략, 주요 그리드만 표시
- **기본 줌 (0.5x - 2x)**: 모든 그리드선 표시
- **줌 인 (> 2x)**: 세부 그리드 추가 표시

### 성능 요구사항

#### 1. 렌더링 최적화
- **Frustum Culling**: 화면 밖 그리드 렌더링 제외
- **배치 렌더링**: 그리드선을 단일 드로우콜로 처리
- **VBO 사용**: 정적 버텍스 버퍼로 그리드 메시 관리

#### 2. 성능 목표
- **프레임레이트**: 60 FPS 유지
- **렌더링 시간**: < 2ms (그리드만)
- **메모리 사용**: < 10MB (그리드 버퍼)

### 기술 요구사항

#### 1. OpenGL 구현
```cpp
class GridRenderer {
    // 셰이더 프로그램
    GLuint shaderProgram;
    
    // 버텍스 버퍼
    GLuint vao, vbo;
    
    // 유니폼 변수
    glm::mat4 viewProjectionMatrix;
    glm::vec2 gridOffset;
    float gridScale;
};
```

#### 2. 셰이더 요구사항

##### Vertex Shader
- 그리드 정점 변환
- 뷰-프로젝션 매트릭스 적용
- 줌 레벨 기반 LOD 계산

##### Fragment Shader
- 그리드선 색상 적용
- 안티앨리어싱 처리
- 페이드 효과 (줌 레벨 기반)

#### 3. 카메라 시스템 통합
```cpp
struct Camera {
    glm::vec2 position;     // 월드 좌표 (그리드 단위)
    float zoom;              // 줌 레벨 (0.1 ~ 10.0)
    glm::vec2 screenSize;    // 화면 크기
    
    glm::mat4 GetViewMatrix();
    glm::mat4 GetProjectionMatrix();
    glm::vec2 ScreenToWorld(glm::vec2 screenPos);
    glm::vec2 WorldToScreen(glm::vec2 worldPos);
    glm::ivec2 ScreenToGrid(glm::vec2 screenPos);
    glm::vec2 GridToScreen(glm::ivec2 gridPos);
};
```

### 인터페이스 요구사항

#### 1. 마우스 상호작용
- **좌표 변환**: 마우스 위치 → 그리드 셀 좌표
- **셀 선택**: 클릭한 셀 하이라이트
- **드래그 지원**: 다중 셀 선택
- **호버 효과**: 마우스 위치 셀 실시간 표시

#### 2. 카메라 컨트롤
- **팬(Pan)**: 
  - 마우스 중간 버튼 드래그
  - 우클릭 드래그 (대체 옵션)
  - 화살표 키 이동
- **줌(Zoom)**:
  - 마우스 휠 (마우스 위치 중심)
  - Ctrl + +/- 키
  - 줌 범위: 0.1x ~ 10.0x
- **리셋**: 
  - Home 키로 원점 복귀
  - 줌 레벨 1.0x로 초기화

### 데이터 구조

#### 1. 그리드 셀
```cpp
struct GridCell {
    int x, y;           // 그리드 좌표
    bool occupied;      // 점유 상태
    uint32_t entityId;  // 배치된 개체 ID
};
```

#### 2. 렌더링 상태
```cpp
struct GridRenderState {
    bool showGrid;           // 그리드 표시 여부
    bool showAxis;           // 축 표시 여부
    float gridOpacity;       // 그리드 투명도
    GridCell hoveredCell;    // 현재 호버 중인 셀
    std::vector<GridCell> selectedCells;  // 선택된 셀들
};
```

### 구현 우선순위

#### Step 3-2: 그리드 렌더링
1. **Phase 1: 기본 그리드**
   - 고정 뷰 그리드 렌더링
   - 화면 중앙 기준 그리드
   - 기본 좌표 변환 (고정 카메라)

2. **Phase 2: 그리드 최적화**
   - VBO 사용
   - 배치 렌더링
   - 기본 셀 하이라이트

#### Step 3-5: 카메라 시스템
1. **Phase 1: 카메라 구현**
   - Camera 클래스 구현
   - 뷰/프로젝션 매트릭스
   - 좌표 변환 함수

2. **Phase 2: 카메라 컨트롤**
   - 팬 기능 (드래그, 키보드)
   - 줌 기능 (휠, 키보드)
   - 카메라 리셋

3. **Phase 3: 고급 기능**
   - Frustum culling
   - 줌 레벨별 LOD
   - 부드러운 카메라 이동

### 테스트 요구사항

#### 1. 기능 테스트
- [ ] 그리드가 올바르게 표시되는가
- [ ] 마우스 좌표 변환이 정확한가
- [ ] 줌/팬이 부드럽게 동작하는가
- [ ] 셀 선택이 정확한가

#### 2. 성능 테스트
- [ ] 60 FPS 유지 (기본 줌 레벨)
- [ ] 줌 아웃시 성능 저하 없음
- [ ] 대규모 그리드에서 메모리 사용량 체크

#### 3. 호환성 테스트
- [ ] OpenGL 3.3+ 지원 확인
- [ ] 다양한 해상도에서 동작
- [ ] 다양한 종횡비 지원

### 예상 파일 구조
```
src/render/
├── GridRenderer.h       # 그리드 렌더러 클래스
├── GridRenderer.cpp     # 구현
├── Camera.h            # 카메라 시스템
├── Camera.cpp          # 카메라 구현
└── shaders/
    ├── grid.vert       # 그리드 버텍스 셰이더
    └── grid.frag       # 그리드 프래그먼트 셰이더
```

### 의존성
- OpenGL 3.3+
- GLM (수학 라이브러리)
- SDL2 (이벤트 처리)

### 참고사항
- 그리드는 게임의 기반이므로 확장 가능하게 설계
- 추후 NOT 게이트와 와이어 렌더링이 이 위에 구축됨
- 대규모 회로 지원을 위해 초기부터 최적화 고려