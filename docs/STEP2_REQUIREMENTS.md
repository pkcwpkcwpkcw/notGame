# Step 2: 데이터 구조 정의 - 요구사항 명세서

## 개요
NOT Gate 게임의 핵심 데이터 구조를 정의하고 구현합니다. 이 단계에서는 게임 로직과 렌더링에 필요한 모든 기본 데이터 타입을 설계합니다.

## 1. Vec2 구조체

### 목적
2D 공간에서의 위치와 방향을 표현하는 기본 벡터 구조체

### 요구사항
- **필드**
  - `float x`: X 좌표
  - `float y`: Y 좌표

- **메서드**
  - 생성자: 기본, 매개변수
  - 연산자 오버로딩: +, -, *, / (스칼라 연산 포함)
  - 길이 계산: `length()`, `lengthSquared()`
  - 정규화: `normalize()`
  - 내적: `dot(const Vec2&)`
  - 거리: `distance(const Vec2&)`

### 사용 예시
```cpp
Vec2 gatePos(10.0f, 20.0f);
Vec2 mousePos = screenToGrid(mouseX, mouseY);
float dist = gatePos.distance(mousePos);
```

## 2. Gate 구조체

### 목적
NOT 게이트의 모든 속성과 상태를 저장

### 요구사항
- **필드**
  - `uint32_t id`: 고유 식별자
  - `GateType type`: 게이트 타입 (현재는 NOT만)
  - `Vec2 position`: 그리드 상 위치
  - `std::array<uint32_t, 3> inputConnections`: 입력 포트 연결 정보 (와이어 ID 또는 INVALID_ID)
  - `uint32_t outputConnection`: 출력 포트 연결 정보
  - `bool currentOutput`: 현재 출력 신호 상태
  - `bool pendingOutput`: 딜레이 후 출력될 신호
  - `float delayTimer`: 딜레이 카운터 (0.1초)

- **메서드**
  - `void update(float deltaTime)`: 딜레이 타이머 업데이트
  - `bool calculateOutput()`: 입력 기반 출력 계산
  - `bool isInputPort(Vec2 localPos)`: 특정 위치가 입력 포트인지 확인
  - `bool isOutputPort(Vec2 localPos)`: 특정 위치가 출력 포트인지 확인
  - `int getInputPortIndex(Vec2 localPos)`: 입력 포트 인덱스 반환

### 포트 위치 규칙
```
입력 포트 (왼쪽):      출력 포트 (오른쪽):
  [0] ─┐              
  [1] ─┼─ [NOT] ──── [출력]
  [2] ─┘              
```

## 3. Wire 구조체

### 목적
게이트 간 연결을 표현

### 요구사항
- **필드**
  - `uint32_t id`: 고유 식별자
  - `uint32_t fromGateId`: 시작 게이트 ID
  - `uint32_t toGateId`: 끝 게이트 ID
  - `int fromPortIndex`: 시작 포트 인덱스 (-1: 출력, 0-2: 입력)
  - `int toPortIndex`: 끝 포트 인덱스
  - `bool signalState`: 현재 전달 중인 신호
  - `std::vector<Vec2> path`: 와이어 경로 (렌더링용)

- **메서드**
  - `void calculatePath(const Gate& from, const Gate& to)`: 경로 계산
  - `bool isPointOnWire(Vec2 point, float tolerance)`: 점이 와이어 위에 있는지 확인

## 4. Circuit 클래스

### 목적
전체 회로를 관리하고 시뮬레이션을 수행

### 요구사항
- **필드**
  - `std::unordered_map<uint32_t, Gate> gates`: 게이트 컨테이너
  - `std::unordered_map<uint32_t, Wire> wires`: 와이어 컨테이너
  - `uint32_t nextGateId`: 다음 게이트 ID
  - `uint32_t nextWireId`: 다음 와이어 ID
  - `float simulationTime`: 총 시뮬레이션 시간
  - `bool isPaused`: 일시정지 상태

- **메서드**
  - **게이트 관리**
    - `uint32_t addGate(GateType type, Vec2 position)`
    - `void removeGate(uint32_t id)`
    - `Gate* getGate(uint32_t id)`
    - `Gate* getGateAt(Vec2 position)`
  
  - **와이어 관리**
    - `uint32_t connectGates(uint32_t fromId, int fromPort, uint32_t toId, int toPort)`
    - `void removeWire(uint32_t id)`
    - `Wire* getWireAt(Vec2 position)`
  
  - **시뮬레이션**
    - `void update(float deltaTime)`: 회로 시뮬레이션 업데이트
    - `void propagateSignals()`: 신호 전파
    - `void reset()`: 모든 신호 초기화
  
  - **유효성 검사**
    - `bool canPlaceGate(Vec2 position)`: 게이트 배치 가능 여부
    - `bool canConnect(uint32_t fromId, int fromPort, uint32_t toId, int toPort)`: 연결 가능 여부
    - `bool hasLoop()`: 순환 회로 검사

### 시뮬레이션 순서
1. 모든 게이트의 입력 신호 읽기
2. NOT 연산 수행 및 pendingOutput 설정
3. 딜레이 타이머 업데이트
4. 타이머 완료 시 currentOutput 업데이트
5. 와이어를 통해 신호 전파

## 5. Grid 시스템

### 목적
화면 좌표와 그리드 좌표 간 변환 및 그리드 관리

### 요구사항
- **Grid 클래스**
  - **필드**
    - `float cellSize`: 그리드 한 칸 크기 (픽셀)
    - `Vec2 offset`: 카메라 오프셋
    - `float zoom`: 줌 레벨
    - `int width, height`: 그리드 크기 (샌드박스는 무제한)
  
  - **메서드**
    - `Vec2 screenToGrid(Vec2 screenPos)`: 화면 좌표 → 그리드 좌표
    - `Vec2 gridToScreen(Vec2 gridPos)`: 그리드 좌표 → 화면 좌표
    - `Vec2 snapToGrid(Vec2 pos)`: 가장 가까운 그리드 점으로 스냅
    - `bool isValidPosition(Vec2 gridPos)`: 유효한 그리드 위치인지 확인
    - `void pan(Vec2 delta)`: 카메라 이동
    - `void zoomAt(Vec2 screenPos, float factor)`: 특정 지점 기준 줌

### 좌표 시스템
- **화면 좌표**: 픽셀 단위, 좌상단 (0,0)
- **그리드 좌표**: 그리드 단위, 중앙 (0,0)
- **월드 좌표**: 실제 렌더링 좌표 (그리드 × cellSize)

## 6. 공통 상수 및 타입 정의

### Types.h
```cpp
// 기본 타입
using GateId = uint32_t;
using WireId = uint32_t;
using PortIndex = int;

// 상수
constexpr uint32_t INVALID_ID = 0;
constexpr float GATE_DELAY = 0.1f;  // 0.1초
constexpr int MAX_INPUT_PORTS = 3;
constexpr int OUTPUT_PORT = -1;

// 열거형
enum class GateType {
    NOT = 0,
    // 향후 확장 가능
};

enum class SignalState {
    LOW = 0,
    HIGH = 1,
    UNDEFINED = 2
};
```

## 7. 성능 고려사항

### 메모리 최적화
- Gate와 Wire는 POD(Plain Old Data) 타입으로 설계
- 캐시 친화적 메모리 레이아웃
- 불필요한 동적 할당 최소화

### 연산 최적화
- Vec2는 SIMD 연산 가능하도록 정렬
- 빈번한 조회를 위한 해시맵 사용
- 공간 분할을 위한 준비 (향후 QuadTree)

## 8. 테스트 요구사항

### 단위 테스트
- Vec2 연산 정확성
- Gate 신호 처리 로직
- Wire 연결 유효성
- Circuit 시뮬레이션 정확성
- Grid 좌표 변환 정확성

### 통합 테스트
- 간단한 NOT 게이트 체인 시뮬레이션
- 신호 딜레이 검증
- 메모리 누수 검사

## 9. 파일 구조

```
src/
├── core/
│   ├── Types.h         # 공통 타입 정의
│   ├── Vec2.h          # Vec2 구조체
│   ├── Vec2.cpp        
│   ├── Gate.h          # Gate 구조체
│   ├── Gate.cpp
│   ├── Wire.h          # Wire 구조체
│   ├── Wire.cpp
│   ├── Circuit.h       # Circuit 클래스
│   ├── Circuit.cpp
│   ├── Grid.h          # Grid 시스템
│   └── Grid.cpp
```

## 10. 구현 순서

1. Types.h - 공통 타입 정의
2. Vec2 - 기본 수학 구조체
3. Gate - 게이트 구조체
4. Wire - 와이어 구조체
5. Grid - 좌표 변환 시스템
6. Circuit - 회로 관리 클래스

## 검증 기준

- [ ] 모든 구조체/클래스가 컴파일 됨
- [ ] Vec2 기본 연산이 정상 동작
- [ ] Gate가 NOT 연산을 올바르게 수행
- [ ] Wire가 신호를 정확히 전달
- [ ] Circuit이 여러 게이트를 관리 가능
- [ ] Grid 좌표 변환이 정확함
- [ ] 0.1초 딜레이가 정확히 구현됨
- [ ] 메모리 누수가 없음