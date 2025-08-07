# Step 2: 데이터 구조 정의 - 기능 명세서

## 1. 개요

### 1.1 목적
NOT Gate 게임의 핵심 데이터 구조를 구현하여 게임 로직의 기반을 마련합니다. 이 단계에서 구현된 데이터 구조는 이후 모든 기능의 토대가 됩니다.

### 1.2 범위
- 수학 연산을 위한 2D 벡터 구조체
- 게이트와 와이어의 데이터 모델
- 회로 시뮬레이션 엔진의 기초
- 좌표 시스템 변환 로직

### 1.3 성공 기준
- 모든 데이터 구조가 설계대로 동작
- 간단한 NOT 게이트 체인 시뮬레이션 가능
- 메모리 효율적이고 확장 가능한 구조

## 2. Vec2 - 2D 벡터 연산

### 2.1 기본 기능
#### 2.1.1 생성 및 초기화
```cpp
Vec2 v1;           // (0, 0)으로 초기화
Vec2 v2(10, 20);   // (10, 20)으로 초기화
Vec2 v3 = v2;      // 복사 생성자
```

#### 2.1.2 기본 연산
```cpp
Vec2 a(10, 20), b(5, 10);
Vec2 sum = a + b;        // (15, 30)
Vec2 diff = a - b;       // (5, 10)
Vec2 scaled = a * 2.0f;  // (20, 40)
Vec2 divided = a / 2.0f; // (5, 10)
```

### 2.2 고급 기능
#### 2.2.1 벡터 연산
- **길이 계산**: 벡터의 크기 반환
- **정규화**: 단위 벡터로 변환
- **내적**: 두 벡터의 내적 계산
- **거리**: 두 점 사이의 거리

#### 2.2.2 유틸리티 함수
```cpp
bool isZero() const;           // 영벡터 확인
bool equals(const Vec2&) const; // 근사 비교 (부동소수점)
Vec2 perpendicular() const;     // 수직 벡터
float angle() const;            // 각도 (라디안)
```

## 3. Gate - NOT 게이트 모델

### 3.1 게이트 속성
#### 3.1.1 식별 정보
- **고유 ID**: 회로 내 유일한 식별자
- **타입**: 게이트 종류 (현재는 NOT만)
- **위치**: 그리드 상 좌표

#### 3.1.2 연결 정보
```
입력 포트 구성:
┌─────────┐
│ [0]     │ 
│ [1] NOT │──[출력]
│ [2]     │
└─────────┘
```
- 입력 포트 3개 (왼쪽)
- 출력 포트 1개 (오른쪽)
- 각 포트는 하나의 와이어만 연결 가능

### 3.2 게이트 동작
#### 3.2.1 신호 처리
```cpp
// NOT 연산 로직
bool calculateOutput() {
    // 입력 중 하나라도 HIGH면 출력은 LOW
    // 모든 입력이 LOW면 출력은 HIGH
    for (int i = 0; i < 3; i++) {
        if (getInputSignal(i) == HIGH) {
            return LOW;
        }
    }
    return HIGH;
}
```

#### 3.2.2 딜레이 처리
- 입력 변화 감지 → 0.1초 대기 → 출력 변경
- 딜레이 중 새 입력은 무시
- 타이머 관리 필요

### 3.3 포트 관리
#### 3.3.1 포트 위치 계산
```cpp
Vec2 getInputPortPosition(int index) {
    // index: 0, 1, 2
    return Vec2(position.x - 0.5f, 
                position.y - 0.3f + index * 0.3f);
}

Vec2 getOutputPortPosition() {
    return Vec2(position.x + 0.5f, position.y);
}
```

#### 3.3.2 포트 선택
- 마우스 위치로 가장 가까운 포트 찾기
- 포트 하이라이트를 위한 거리 계산

## 4. Wire - 연결선 모델

### 4.1 와이어 속성
#### 4.1.1 연결 정보
- **시작점**: 게이트 ID + 포트 인덱스
- **끝점**: 게이트 ID + 포트 인덱스
- **신호 상태**: HIGH/LOW/UNDEFINED

#### 4.1.2 경로 정보
- 직선 또는 꺾인 경로
- 렌더링을 위한 중간 점들
- 충돌 감지를 위한 바운딩 박스

### 4.2 와이어 기능
#### 4.2.1 경로 생성
```cpp
void calculatePath(const Gate& from, const Gate& to) {
    Vec2 start = from.getOutputPortPosition();
    Vec2 end = to.getInputPortPosition(toPortIndex);
    
    // 직선 경로 또는 L자 경로 생성
    if (canGoStraight(start, end)) {
        path = {start, end};
    } else {
        Vec2 mid = calculateBendPoint(start, end);
        path = {start, mid, end};
    }
}
```

#### 4.2.2 신호 전달
- 출력 게이트의 신호를 입력 게이트로 전달
- 즉시 전달 (와이어는 딜레이 없음)
- 신호 변화 이벤트 발생

### 4.3 와이어 제약
- 자기 자신에게 연결 불가
- 출력→입력 방향만 가능
- 이미 연결된 포트에 중복 연결 불가

## 5. Circuit - 회로 관리자

### 5.1 회로 구성 요소
#### 5.1.1 컨테이너 관리
```cpp
class Circuit {
    std::unordered_map<uint32_t, Gate> gates;
    std::unordered_map<uint32_t, Wire> wires;
    
    // ID 생성기
    uint32_t nextGateId = 1;
    uint32_t nextWireId = 1;
};
```

#### 5.1.2 시뮬레이션 상태
- 전체 시뮬레이션 시간
- 일시정지/재생 상태
- 업데이트 주기 (60 FPS)

### 5.2 게이트 관리 기능
#### 5.2.1 게이트 추가
```cpp
uint32_t addGate(Vec2 position) {
    // 1. 위치 유효성 검사
    if (!canPlaceGate(position)) return INVALID_ID;
    
    // 2. 새 게이트 생성
    Gate gate;
    gate.id = nextGateId++;
    gate.position = position;
    
    // 3. 컨테이너에 추가
    gates[gate.id] = gate;
    return gate.id;
}
```

#### 5.2.2 게이트 삭제
- 연결된 모든 와이어 제거
- 게이트 제거
- 주변 게이트 신호 재계산

### 5.3 와이어 관리 기능
#### 5.3.1 와이어 연결
```cpp
uint32_t connectGates(uint32_t fromId, uint32_t toId, int toPort) {
    // 1. 유효성 검사
    if (!canConnect(fromId, toId, toPort)) return INVALID_ID;
    
    // 2. 와이어 생성
    Wire wire;
    wire.id = nextWireId++;
    wire.fromGateId = fromId;
    wire.toGateId = toId;
    wire.toPortIndex = toPort;
    
    // 3. 경로 계산
    wire.calculatePath(gates[fromId], gates[toId]);
    
    // 4. 연결 정보 업데이트
    gates[fromId].outputConnection = wire.id;
    gates[toId].inputConnections[toPort] = wire.id;
    
    wires[wire.id] = wire;
    return wire.id;
}
```

#### 5.3.2 와이어 삭제
- 연결 정보 제거
- 와이어 제거
- 영향받는 게이트 신호 재계산

### 5.4 시뮬레이션 엔진
#### 5.4.1 업데이트 사이클
```cpp
void update(float deltaTime) {
    if (isPaused) return;
    
    // 1. 게이트 딜레이 타이머 업데이트
    for (auto& [id, gate] : gates) {
        gate.updateTimer(deltaTime);
    }
    
    // 2. 신호 계산 및 전파
    propagateSignals();
    
    // 3. 시뮬레이션 시간 업데이트
    simulationTime += deltaTime;
}
```

#### 5.4.2 신호 전파
```cpp
void propagateSignals() {
    // 1. 모든 게이트의 새 출력 계산
    std::unordered_map<uint32_t, bool> newOutputs;
    for (auto& [id, gate] : gates) {
        if (gate.isTimerComplete()) {
            newOutputs[id] = gate.calculateOutput();
        }
    }
    
    // 2. 출력 업데이트
    for (auto& [id, output] : newOutputs) {
        gates[id].currentOutput = output;
    }
    
    // 3. 와이어를 통해 신호 전달
    for (auto& [id, wire] : wires) {
        Gate& from = gates[wire.fromGateId];
        wire.signalState = from.currentOutput;
    }
}
```

### 5.5 유효성 검사
#### 5.5.1 배치 가능 검사
- 그리드 범위 확인
- 기존 게이트와 겹침 확인
- 유효한 그리드 좌표 확인

#### 5.5.2 연결 가능 검사
- 포트 사용 가능 여부
- 순환 회로 검사
- 유효한 게이트 ID 확인

## 6. Grid - 좌표 시스템

### 6.1 좌표 변환
#### 6.1.1 화면 → 그리드
```cpp
Vec2 screenToGrid(Vec2 screenPos) {
    // 카메라 오프셋과 줌 적용
    Vec2 worldPos = (screenPos - offset) / zoom;
    
    // 그리드 단위로 변환
    return worldPos / cellSize;
}
```

#### 6.1.2 그리드 → 화면
```cpp
Vec2 gridToScreen(Vec2 gridPos) {
    // 월드 좌표로 변환
    Vec2 worldPos = gridPos * cellSize;
    
    // 화면 좌표로 변환
    return worldPos * zoom + offset;
}
```

### 6.2 그리드 스냅
#### 6.2.1 가장 가까운 그리드 점
```cpp
Vec2 snapToGrid(Vec2 pos) {
    return Vec2(
        std::round(pos.x),
        std::round(pos.y)
    );
}
```

#### 6.2.2 그리드 범위 확인
- 퍼즐 모드: 제한된 크기
- 샌드박스 모드: 무제한 (가상)

### 6.3 카메라 제어
#### 6.3.1 이동 (Pan)
```cpp
void pan(Vec2 delta) {
    offset += delta;
    
    // 경계 제한 (옵션)
    if (hasLimits) {
        clampOffset();
    }
}
```

#### 6.3.2 줌
```cpp
void zoomAt(Vec2 screenPos, float factor) {
    // 마우스 위치 기준 줌
    Vec2 gridPos = screenToGrid(screenPos);
    
    zoom *= factor;
    zoom = std::clamp(zoom, minZoom, maxZoom);
    
    // 줌 후 같은 그리드 위치가 마우스 아래 유지
    Vec2 newScreenPos = gridToScreen(gridPos);
    offset += screenPos - newScreenPos;
}
```

## 7. 에러 처리

### 7.1 예외 상황
- 잘못된 게이트/와이어 ID
- 범위 초과 배치
- 메모리 할당 실패
- 순환 회로 감지

### 7.2 에러 코드
```cpp
enum class ErrorCode {
    SUCCESS = 0,
    INVALID_ID,
    POSITION_OCCUPIED,
    PORT_ALREADY_CONNECTED,
    CIRCULAR_DEPENDENCY,
    OUT_OF_MEMORY
};
```

## 8. 성능 최적화 전략

### 8.1 메모리 최적화
- 객체 풀 사용 (게이트, 와이어)
- 캐시 친화적 데이터 배치
- 불필요한 복사 방지 (move semantics)

### 8.2 연산 최적화
- 더티 플래그로 불필요한 계산 방지
- 공간 분할로 충돌 감지 최적화
- SIMD 명령어 활용 준비

## 9. 테스트 시나리오

### 9.1 단위 테스트
1. Vec2 모든 연산 검증
2. Gate NOT 로직 검증
3. Wire 경로 계산 검증
4. Circuit 기본 동작 검증
5. Grid 좌표 변환 정확성

### 9.2 통합 테스트
1. 3개 NOT 게이트 체인
2. 신호 딜레이 타이밍
3. 게이트 추가/삭제
4. 와이어 연결/해제
5. 대규모 회로 (1000개 게이트)

### 9.3 스트레스 테스트
- 10,000개 게이트 생성
- 50,000개 와이어 연결
- 60 FPS 유지 확인

## 10. API 사용 예제

### 10.1 간단한 회로 생성
```cpp
Circuit circuit;

// 게이트 3개 추가
auto g1 = circuit.addGate(Vec2(0, 0));
auto g2 = circuit.addGate(Vec2(2, 0));
auto g3 = circuit.addGate(Vec2(4, 0));

// 연결
circuit.connectGates(g1, g2, 0);  // g1 출력 → g2 입력0
circuit.connectGates(g2, g3, 1);  // g2 출력 → g3 입력1

// 시뮬레이션
float deltaTime = 1.0f / 60.0f;
for (int i = 0; i < 60; i++) {
    circuit.update(deltaTime);
}
```

### 10.2 마우스 상호작용
```cpp
Grid grid(32.0f);  // 32픽셀 그리드

// 마우스 클릭 처리
Vec2 mouseScreen(event.x, event.y);
Vec2 gridPos = grid.screenToGrid(mouseScreen);
Vec2 snapped = grid.snapToGrid(gridPos);

// 게이트 배치
if (circuit.canPlaceGate(snapped)) {
    circuit.addGate(snapped);
}
```

## 11. 확장 가능성

### 11.1 향후 추가 기능
- 다른 게이트 타입 (AND, OR, XOR)
- 커스텀 게이트 (서브 회로)
- 신호 버스 (다중 신호)
- 클록 신호 생성기

### 11.2 성능 개선 여지
- GPU 컴퓨트 셰이더 활용
- 멀티스레드 시뮬레이션
- 증분 업데이트 (변경된 부분만)
- LOD 시스템 (먼 거리 간소화)