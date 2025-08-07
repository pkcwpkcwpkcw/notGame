# Step 4: 입력 처리 시스템 요구사항 명세서

## 개요
사용자의 마우스 입력을 처리하여 게임 내 상호작용을 가능하게 하는 시스템 구현

## 구현 목표
- 마우스 이벤트를 게임 좌표계로 변환
- 게임 오브젝트와의 상호작용 감지
- 드래그 동작 처리

## 상세 요구사항

### 4.1 마우스 좌표를 그리드 좌표로 변환

#### 기능 요구사항
- 스크린 좌표를 월드 좌표로 변환
- 월드 좌표를 그리드 좌표로 변환
- 카메라 변환 행렬 적용 (pan, zoom 고려)

#### 구현 사항
```cpp
// InputManager.h
class InputManager {
    glm::vec2 screenToWorld(const glm::vec2& screenPos);
    glm::ivec2 worldToGrid(const glm::vec2& worldPos);
    glm::ivec2 screenToGrid(const glm::vec2& screenPos);
};
```

#### 변환 공식
1. 스크린 → NDC: `(2.0 * x / width - 1.0, 1.0 - 2.0 * y / height)`
2. NDC → 월드: 역 뷰-프로젝션 행렬 적용
3. 월드 → 그리드: `floor(worldPos / gridSize)`

### 4.2 클릭 감지 (게이트, 와이어, 빈 공간)

#### 기능 요구사항
- 마우스 클릭 위치에서 오브젝트 판별
- 우선순위: 게이트 > 와이어 > 빈 공간
- 클릭 가능 영역 정의

#### 구현 사항
```cpp
enum class ClickTarget {
    None,
    Gate,
    Wire,
    Empty
};

struct ClickInfo {
    ClickTarget type;
    uint32_t objectId;  // 게이트 또는 와이어 ID
    glm::ivec2 gridPos;
};

class InputManager {
    ClickInfo detectClick(const glm::vec2& mousePos);
    bool isPointInGate(const glm::vec2& point, const Gate& gate);
    bool isPointOnWire(const glm::vec2& point, const Wire& wire);
};
```

#### 히트 박스 정의
- 게이트: 그리드 셀 전체 (1.0 x 1.0)
- 와이어: 선분으로부터 0.1 단위 거리 이내
- 포트: 게이트 가장자리 0.2 x 0.2 영역

### 4.3 드래그 시작/종료 감지

#### 기능 요구사항
- 마우스 버튼 press/release 추적
- 드래그 임계값 설정 (5픽셀)
- 드래그 상태 관리

#### 구현 사항
```cpp
enum class DragState {
    None,
    Potential,  // 버튼 눌림, 아직 움직이지 않음
    Active      // 임계값 초과하여 드래그 중
};

struct DragInfo {
    DragState state;
    glm::vec2 startPos;     // 드래그 시작 위치
    glm::vec2 currentPos;   // 현재 마우스 위치
    glm::ivec2 startGrid;   // 시작 그리드 좌표
    glm::ivec2 currentGrid; // 현재 그리드 좌표
    ClickTarget dragTarget; // 드래그 대상 타입
    uint32_t targetId;      // 드래그 대상 ID
};

class InputManager {
    void onMouseDown(int button, const glm::vec2& pos);
    void onMouseMove(const glm::vec2& pos);
    void onMouseUp(int button, const glm::vec2& pos);
    
    bool isDragging() const;
    const DragInfo& getDragInfo() const;
};
```

## 이벤트 처리 흐름

### 마우스 버튼 DOWN
1. 스크린 좌표 → 월드 좌표 변환
2. 클릭 대상 감지
3. 드래그 상태를 Potential로 설정
4. 시작 위치 저장

### 마우스 이동
1. Potential 상태에서 임계값 체크
2. 임계값 초과 시 Active로 전환
3. 현재 위치 업데이트
4. 드래그 콜백 호출

### 마우스 버튼 UP
1. 드래그 상태가 Potential이면 클릭으로 처리
2. 드래그 상태가 Active이면 드래그 종료로 처리
3. 상태 초기화

## 인터페이스 설계

```cpp
// InputManager.h
class InputManager {
public:
    InputManager(Camera* camera, Circuit* circuit);
    
    // SDL 이벤트 처리
    void handleEvent(const SDL_Event& event);
    
    // 상태 조회
    bool isMouseDown(int button) const;
    bool isDragging() const;
    glm::vec2 getMousePos() const;
    glm::ivec2 getMouseGrid() const;
    
    // 콜백 등록
    using ClickCallback = std::function<void(const ClickInfo&)>;
    using DragCallback = std::function<void(const DragInfo&)>;
    
    void setOnClick(ClickCallback callback);
    void setOnDragStart(DragCallback callback);
    void setOnDragMove(DragCallback callback);
    void setOnDragEnd(DragCallback callback);
    
private:
    Camera* m_camera;
    Circuit* m_circuit;
    
    // 마우스 상태
    glm::vec2 m_mousePos;
    bool m_mouseButtons[3];
    
    // 드래그 상태
    DragInfo m_dragInfo;
    static constexpr float DRAG_THRESHOLD = 5.0f;
    
    // 콜백
    ClickCallback m_onClickCallback;
    DragCallback m_onDragStartCallback;
    DragCallback m_onDragMoveCallback;
    DragCallback m_onDragEndCallback;
};
```

## 통합 방법

### main.cpp 수정
```cpp
// 이벤트 루프에서
while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_ProcessEvent(&event);
    
    if (!ImGui::GetIO().WantCaptureMouse) {
        inputManager.handleEvent(event);
    }
}
```

### 사용 예시
```cpp
// 게임 초기화
inputManager.setOnClick([](const ClickInfo& info) {
    if (info.type == ClickTarget::Gate) {
        // 게이트 선택
    } else if (info.type == ClickTarget::Empty) {
        // 새 게이트 배치
    }
});

inputManager.setOnDragEnd([](const DragInfo& info) {
    if (info.dragTarget == ClickTarget::Wire) {
        // 와이어 연결 완료
    }
});
```

## 테스트 요구사항

### 단위 테스트
1. 좌표 변환 정확성
   - 다양한 카메라 위치/줌 레벨에서 변환 검증
   - 경계값 테스트

2. 히트 감지
   - 게이트 중심, 모서리, 외부 클릭
   - 와이어 위, 근처, 멀리 클릭

3. 드래그 상태 전환
   - 임계값 미만/초과 이동
   - 여러 버튼 동시 처리

### 통합 테스트
1. 카메라 이동 중 클릭
2. 줌 레벨 변경 후 드래그
3. 빠른 클릭/드래그 전환

## 성능 고려사항

### 최적화 전략
- 히트 감지 시 공간 분할 사용 (그리드 기반)
- 마우스 이동 시에만 좌표 변환 수행
- 드래그 중이 아닐 때는 드래그 체크 스킵

### 목표 성능
- 마우스 이벤트 처리: < 0.1ms
- 히트 감지: < 0.5ms (1000개 오브젝트 기준)

## 확장 고려사항

### 향후 추가 기능
- 터치 입력 지원
- 키보드 단축키와 조합
- 우클릭 컨텍스트 메뉴
- 멀티 터치 제스처

### 모듈성
- InputManager는 렌더링과 독립적
- 이벤트 시스템으로 확장 가능
- 입력 매핑 커스터마이징 지원

## 구현 체크리스트

- [ ] InputManager 클래스 생성
- [ ] 좌표 변환 함수 구현
  - [ ] screenToWorld
  - [ ] worldToGrid
  - [ ] screenToGrid
- [ ] 히트 감지 구현
  - [ ] isPointInGate
  - [ ] isPointOnWire
  - [ ] detectClick
- [ ] 드래그 처리 구현
  - [ ] onMouseDown
  - [ ] onMouseMove
  - [ ] onMouseUp
- [ ] 콜백 시스템 구현
- [ ] SDL 이벤트 통합
- [ ] 테스트 코드 작성
- [ ] 디버그 시각화 (선택사항)