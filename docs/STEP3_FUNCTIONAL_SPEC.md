# Step 3-3, 3-4 기능 명세서

## Step 3-3: 게이트 스프라이트 렌더링 기능 명세

### 1. 게이트 렌더링 컴포넌트

#### 1.1 GateRenderer 클래스
```cpp
class GateRenderer {
public:
    void initialize();
    void render(const std::vector<Gate>& gates, const Camera& camera);
    void renderSingle(const Gate& gate, const glm::mat4& mvp);
    void cleanup();
    
private:
    GLuint vao, vbo, ebo;
    GLuint instanceVBO;
    Shader gateShader;
};
```

#### 1.2 게이트 비주얼 데이터 구조
```cpp
struct GateVisual {
    glm::vec2 position;      // 그리드 위치
    glm::vec4 bodyColor;     // 게이트 본체 색상
    glm::vec4 portColors[4]; // 포트 색상 (3 입력, 1 출력)
    float scale;             // 크기
    bool isActive;           // 활성화 상태
};
```

### 2. 렌더링 기능 상세

#### 2.1 게이트 본체 렌더링
- **형태**: 1x1 그리드 크기의 사각형
- **색상**: 
  - 기본: RGB(128, 128, 128)
  - 선택됨: RGB(200, 200, 100)
  - 처리중: RGB(180, 180, 255)
- **테두리**: 2픽셀 두께의 어두운 테두리

#### 2.2 포트 렌더링
- **입력 포트 (3개)**
  - 위치: 왼쪽 면에 균등 배치
  - 크기: 게이트 높이의 1/6
  - 색상:
    - 연결 안됨: RGB(80, 80, 80)
    - 신호 0: RGB(100, 100, 100)
    - 신호 1: RGB(255, 255, 0)
    
- **출력 포트 (1개)**
  - 위치: 오른쪽 면 중앙
  - 크기: 게이트 높이의 1/4
  - 색상:
    - 신호 0: RGB(100, 100, 100)
    - 신호 1: RGB(255, 255, 0)

#### 2.3 게이트 라벨
- NOT 텍스트 또는 ¬ 기호 표시
- 중앙 정렬
- 색상: 흰색 또는 검정색 (배경 대비)

### 3. 렌더링 최적화 기능

#### 3.1 인스턴스 렌더링
- 동일한 타입의 게이트를 한 번의 드로우콜로 렌더링
- 최대 10,000개씩 배치 처리
- 인스턴스별 데이터: 위치, 색상, 신호 상태

#### 3.2 프러스텀 컬링
- 화면 밖 게이트 제외
- 여유 공간 20% 추가 (스크롤 대비)
- 쿼드트리 기반 공간 분할 (대규모 회로)

#### 3.3 LOD (Level of Detail)
- **레벨 0** (줌 > 2.0): 전체 디테일
- **레벨 1** (줌 0.5-2.0): 포트 간소화
- **레벨 2** (줌 < 0.5): 게이트만 표시

---

## Step 3-4: 와이어 라인 렌더링 기능 명세

### 1. 와이어 렌더링 컴포넌트

#### 1.1 WireRenderer 클래스
```cpp
class WireRenderer {
public:
    void initialize();
    void render(const std::vector<Wire>& wires, const Camera& camera);
    void renderDragging(const glm::vec2& start, const glm::vec2& current);
    void cleanup();
    
private:
    GLuint vao, vbo;
    Shader wireShader;
    std::vector<glm::vec2> calculatePath(const Wire& wire);
};
```

#### 1.2 와이어 비주얼 데이터 구조
```cpp
struct WireVisual {
    std::vector<glm::vec2> pathPoints;  // 경로 점들
    glm::vec4 color;                    // 와이어 색상
    float thickness;                     // 선 두께
    bool hasSignal;                      // 신호 전달 여부
    float signalProgress;                // 신호 애니메이션 진행도
};
```

### 2. 와이어 경로 계산 기능

#### 2.1 경로 알고리즘
- **직선 경로**: 수평 또는 수직 일직선인 경우
- **L자 경로**: 한 번 꺾임
- **Z자 경로**: 두 번 꺾임 (필요시)
- **충돌 회피**: A* 알고리즘 사용 (복잡한 경우)

#### 2.2 경로 최적화
- 불필요한 중간점 제거
- 직각 우선 정책
- 최단 거리 선택

### 3. 와이어 렌더링 기능

#### 3.1 기본 렌더링
- **선 두께**: 
  - 기본: 2픽셀
  - 선택됨: 3픽셀
  - 줌 레벨에 따라 조절
- **색상**:
  - 연결 안됨: RGB(100, 100, 100)
  - 신호 0: RGB(120, 120, 120)
  - 신호 1: RGB(255, 200, 0)
  - 드래그 중: RGB(150, 150, 255)

#### 3.2 연결점 렌더링
- 시작점과 끝점에 작은 원
- 크기: 와이어 두께의 2배
- 색상: 와이어와 동일

#### 3.3 신호 애니메이션
- 신호 전파 시 밝은 점이 이동
- 속도: 10 그리드/초
- 페이드 효과 적용

### 4. 와이어 상호작용 기능

#### 4.1 드래그 중 렌더링
- 실시간 경로 업데이트
- 반투명 표시
- 연결 가능 포트 하이라이트

#### 4.2 선택 및 하이라이트
- 마우스 오버 시 밝게 표시
- 선택된 와이어 강조
- 다중 선택 지원

### 5. 렌더링 최적화 기능

#### 5.1 배치 렌더링
- 동일한 상태의 와이어 그룹화
- 버텍스 버퍼 재사용
- 최대 1000개씩 배치 처리

#### 5.2 시각적 최적화
- 줌 아웃 시 와이어 단순화
- 안티앨리어싱 적용
- 겹치는 와이어 오프셋

---

## 통합 렌더링 시스템

### 렌더링 순서
1. 배경 그리드
2. 와이어 (뒤)
3. 게이트
4. 와이어 (앞, 선택된 것)
5. UI 오버레이

### 셰이더 프로그램

#### 게이트 버텍스 셰이더
```glsl
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aInstancePos;
layout (location = 2) in vec4 aColor;

uniform mat4 mvp;
out vec4 vertexColor;

void main() {
    vec2 worldPos = aPos + aInstancePos;
    gl_Position = mvp * vec4(worldPos, 0.0, 1.0);
    vertexColor = aColor;
}
```

#### 와이어 버텍스 셰이더
```glsl
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;

uniform mat4 mvp;
out vec4 vertexColor;

void main() {
    gl_Position = mvp * vec4(aPos, 0.0, 1.0);
    vertexColor = aColor;
}
```

### 메모리 관리
- 정적 버퍼: 게이트 지오메트리
- 동적 버퍼: 인스턴스 데이터, 와이어 경로
- 버퍼 크기: 초기 1MB, 필요시 2배씩 증가

### 성능 목표
- 10,000 게이트 @ 60 FPS
- 50,000 와이어 @ 60 FPS
- 렌더링 시간 < 16ms