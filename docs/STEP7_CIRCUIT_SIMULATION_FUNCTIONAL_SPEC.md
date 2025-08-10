# Step 7: 회로 시뮬레이션 엔진 기능 명세서

## 1. 개요

### 1.1 목적
본 문서는 NOT Gate 게임의 회로 시뮬레이션 엔진이 **어떻게** 동작해야 하는지를 사용자와 시스템 관점에서 상세히 정의합니다. 사용자 인터페이스부터 내부 알고리즘까지 전체 시스템의 동작 방식을 설명합니다.

### 1.2 대상 독자
- 게임 개발자 (구현 담당)
- UI/UX 디자이너 (인터페이스 설계)
- QA 테스터 (기능 검증)
- 프로젝트 관리자 (진행도 확인)

### 1.3 관련 문서
- [STEP7_CIRCUIT_SIMULATION_REQUIREMENTS.md](STEP7_CIRCUIT_SIMULATION_REQUIREMENTS.md): 요구사항 명세
- [GAME_SPEC.md](GAME_SPEC.md): 게임 기능 명세
- [CPP_ARCHITECTURE.md](CPP_ARCHITECTURE.md): 기술 아키텍처

## 2. 시스템 동작 워크플로우

### 2.1 시뮬레이션 생명주기

#### 2.1.1 초기화 단계 (Initialization)
```
사용자 동작: 게임 시작 또는 회로 로드
시스템 응답:
1. 회로 데이터 구조 생성 (0.1초)
2. 신호 상태 배열 초기화 (모든 신호 = 0)
3. 게이트 타이머 배열 초기화
4. 연결 그래프 구축 및 위상학적 정렬
5. UI 컨트롤 활성화 (재생 버튼 등)
```

**사용자 피드백**: 로딩 바와 "회로 초기화 중..." 메시지 표시

#### 2.1.2 시뮬레이션 시작 (Start)
```
사용자 동작: ▶ 재생 버튼 클릭
시스템 응답:
1. 시뮬레이션 상태를 RUNNING으로 변경
2. deltaTime 기반 업데이트 루프 시작
3. 신호 전파 계산 시작 (60 FPS 목표)
4. 시각적 피드백 활성화 (신호 애니메이션)
```

**사용자 피드백**: 
- 재생 버튼이 ⏸ 일시정지 버튼으로 변경
- 활성화된 신호가 색상으로 표시 (0=회색, 1=녹색)
- 상태바에 "시뮬레이션 실행 중" 표시

#### 2.1.3 일시정지/재개 (Pause/Resume)
```
사용자 동작: ⏸ 일시정지 버튼 클릭
시스템 응답:
1. 시뮬레이션 상태를 PAUSED로 변경
2. deltaTime 누적 중단
3. 현재 신호 상태 유지
4. 게이트 타이머 상태 보존
```

**사용자 피드백**: 
- 일시정지 버튼이 ▶ 재생 버튼으로 변경
- 모든 신호 애니메이션 정지
- 상태바에 "시뮬레이션 일시정지됨" 표시

#### 2.1.4 정지/리셋 (Stop/Reset)
```
사용자 동작: ⏹ 정지 버튼 클릭
시스템 응답:
1. 시뮬레이션 상태를 STOPPED로 변경
2. 모든 신호를 초기 상태(0)로 리셋
3. 게이트 타이머 초기화
4. UI를 초기 상태로 복원
```

### 2.2 신호 전파 프로세스

#### 2.2.1 프레임별 업데이트 사이클
```
매 프레임 (16.67ms @ 60 FPS):
1. deltaTime 계산
2. 게이트 타이머 업데이트
3. 만료된 타이머의 출력 변경 적용
4. 변경된 신호 전파 (BFS/DFS 방식)
5. 새로운 게이트 입력 변경 감지
6. 해당 게이트의 타이머 시작/재시작
7. UI 업데이트 (변경된 시각 요소만)
```

#### 2.2.2 NOT 게이트 신호 처리
```cpp
// 게이트 입력 변경 감지
bool input1 = getSignalState(gate.input1);
bool input2 = getSignalState(gate.input2);  
bool input3 = getSignalState(gate.input3);

// NOT 연산 (하나라도 1이면 출력 0)
bool newOutput = !(input1 || input2 || input3);

if (newOutput != gate.currentOutput) {
    // 0.1초 딜레이 타이머 시작
    gate.timer = 0.1f;
    gate.pendingOutput = newOutput;
    gate.timerActive = true;
}
```

**시각적 표현**:
- 입력 신호 변경 시 게이트가 깜빡임 (주황색)
- 딜레이 중인 게이트는 타이머 진행 바 표시
- 출력 변경 시 연결된 와이어에 신호 전파 애니메이션

#### 2.2.3 와이어 신호 전달
```cpp
// 와이어는 즉시 신호 전달 (딜레이 없음)
void propagateWireSignal(Wire& wire) {
    bool sourceSignal = getSignalState(wire.source);
    setSignalState(wire.destination, sourceSignal);
    
    // 시각적 업데이트
    wire.visualState = sourceSignal ? ACTIVE : INACTIVE;
    
    // 연결된 다음 요소로 즉시 전파
    propagateToConnectedElements(wire.destination);
}
```

**시각적 표현**:
- 활성 신호: 와이어가 녹색으로 표시, 신호 흐름 애니메이션
- 비활성 신호: 와이어가 회색으로 표시

### 2.3 루프 감지 및 발진 처리

#### 2.3.1 순환 의존성 감지
```cpp
bool CircuitSimulator::detectLoop() {
    // DFS를 사용한 순환 감지
    std::vector<bool> visited(gates.size(), false);
    std::vector<bool> recursionStack(gates.size(), false);
    
    for (int i = 0; i < gates.size(); i++) {
        if (!visited[i] && hasCycleDFS(i, visited, recursionStack)) {
            return true;
        }
    }
    return false;
}
```

**사용자 알림**:
- 루프 감지 시 화면 상단에 주황색 경고 메시지
- "⚠️ 신호 루프가 감지되었습니다. 회로가 발진할 수 있습니다."
- 루프에 포함된 게이트들을 점선 테두리로 강조 표시

#### 2.3.2 발진 동작
```
루프가 있는 회로에서:
1. 신호가 루프를 따라 계속 순환
2. 각 게이트의 0.1초 딜레이로 인해 자연스러운 발진 발생
3. 발진 주기 = 루프 내 게이트 수 × 0.1초
4. 최대 발진 주기 = 1초 (안전장치)
```

**시각적 표현**:
- 발진하는 신호는 교대로 깜빡이는 애니메이션
- 발진 주기를 UI에 표시 ("발진 주기: 0.3초")

## 3. 사용자 인터페이스 동작

### 3.1 시뮬레이션 컨트롤 패널

#### 3.1.1 재생 컨트롤
```
[▶ 재생] [⏸ 일시정지] [⏹ 정지] [⏩ 빠른 재생]

상태별 버튼 동작:
- STOPPED: 재생만 활성화
- RUNNING: 일시정지, 정지 활성화  
- PAUSED: 재생, 정지 활성화
```

**상호작용**:
- 버튼 호버 시 툴팁 표시
- 클릭 시 0.1초 버튼 눌림 애니메이션
- 키보드 단축키 지원 (스페이스바 = 재생/일시정지)

#### 3.1.2 속도 조절
```
[0.1x] [0.5x] [1x] [2x] [5x] [10x]

속도별 동작:
- 0.1x: 디버깅용 초저속 (게이트 딜레이 1초)
- 1x: 기본 속도 (게이트 딜레이 0.1초)
- 10x: 고속 시뮬레이션 (게이트 딜레이 0.01초)
```

**시각적 피드백**:
- 선택된 속도는 녹색 배경으로 강조
- 속도 변경 시 현재 속도를 상태바에 표시
- 고속 모드에서는 애니메이션 간소화

### 3.2 실시간 신호 상태 표시

#### 3.2.1 신호 시각화
```
신호 상태별 색상:
- 0 (OFF): #808080 (회색)
- 1 (ON): #00FF00 (녹색)  
- 딜레이 중: #FFA500 (주황색)
- 오류: #FF0000 (빨간색)
```

**애니메이션 효과**:
- 신호 변경 시 0.2초간 부드러운 색상 전환
- 활성 신호는 미세한 맥동 효과 (1초 주기)
- 신호 전파 시 와이어를 따라 흐르는 점 애니메이션

#### 3.2.2 게이트 상태 표시
```
게이트 시각 상태:
- 대기: 기본 스프라이트 (회색 테두리)
- 활성: 녹색 테두리 + 출력 신호 표시
- 딜레이: 주황색 테두리 + 진행률 바
- 오류: 빨간색 테두리 + X 마크
```

### 3.3 디버그 모드 UI

#### 3.3.1 성능 지표 패널
```
┌─ 성능 정보 ─────────────────┐
│ FPS: 60 / 60               │
│ 시뮬레이션 시간: 15.2ms     │
│ 활성 게이트: 1,234 / 10,000 │
│ 신호 변경: 45 / frame      │
│ 메모리 사용: 12.4 MB       │
└──────────────────────────┘
```

#### 3.3.2 신호 추적 도구
```
사용자 동작: 게이트나 와이어 우클릭
시스템 응답:
1. 해당 신호의 전파 경로를 색상으로 강조
2. 신호 값 변화 히스토리를 타임라인으로 표시
3. 연결된 모든 요소를 하이라이트
```

**시각적 표현**:
- 추적 중인 신호 경로는 노란색 테두리
- 신호 히스토리는 그래프 형태로 하단에 표시
- 현재 위치를 빨간색 마커로 표시

### 3.4 오류 및 경고 메시지

#### 3.4.1 메시지 유형별 표시
```
정보 메시지 (파란색):
"시뮬레이션이 시작되었습니다."

경고 메시지 (주황색):  
"⚠️ 신호 루프 감지: 발진 가능성"

오류 메시지 (빨간색):
"🚨 메모리 부족: 시뮬레이션 중단"
```

**표시 위치 및 지속시간**:
- 화면 상단 중앙에 토스트 메시지 형태
- 정보: 3초 후 자동 사라짐
- 경고: 5초 후 자동 사라짐  
- 오류: 사용자가 닫을 때까지 유지

#### 3.4.2 상황별 대응 가이드
```
메모리 부족 시:
"메모리가 부족합니다. 다음 중 하나를 시도하세요:
• 일부 게이트를 삭제하세요
• 시뮬레이션 영역을 축소하세요  
• 애플리케이션을 재시작하세요"

성능 저하 시:
"시뮬레이션이 느려졌습니다. 권장 사항:
• 시뮬레이션 속도를 낮추세요
• 불필요한 게이트를 제거하세요
• 디버그 모드를 비활성화하세요"
```

## 4. 데이터 처리 로직

### 4.1 신호 상태 관리

#### 4.1.1 비트 배열 구조
```cpp
class SignalManager {
    // 32개 신호를 하나의 uint32_t에 저장
    alignas(64) uint32_t signalBits[MAX_SIGNALS / 32];
    std::vector<uint32_t> dirtySignals;  // 변경된 신호 목록
    
public:
    bool getSignal(uint32_t id) {
        uint32_t wordIndex = id / 32;
        uint32_t bitIndex = id % 32;
        return (signalBits[wordIndex] >> bitIndex) & 1;
    }
    
    void setSignal(uint32_t id, bool value) {
        uint32_t wordIndex = id / 32;
        uint32_t bitIndex = id % 32;
        
        if (value) {
            signalBits[wordIndex] |= (1U << bitIndex);
        } else {
            signalBits[wordIndex] &= ~(1U << bitIndex);
        }
        
        dirtySignals.push_back(id);  // 변경 추적
    }
};
```

#### 4.1.2 변경 추적 및 전파
```cpp
void CircuitSimulator::updateFrame(float deltaTime) {
    // 1. 타이머 업데이트
    updateGateTimers(deltaTime);
    
    // 2. 만료된 타이머 처리
    processExpiredTimers();
    
    // 3. 신호 전파 (변경된 신호만)
    propagateChangedSignals();
    
    // 4. 새로운 게이트 입력 변경 감지
    detectGateInputChanges();
    
    // 5. 시각적 업데이트 준비
    prepareVisualUpdates();
}
```

### 4.2 타이머 시스템

#### 4.2.1 게이트 딜레이 타이머
```cpp
struct GateTimer {
    float remainingTime;    // 남은 시간 (0.1초에서 감소)
    bool newOutput;        // 변경될 출력 값
    bool isActive;         // 타이머 활성 여부
    uint32_t gateId;       // 해당 게이트 ID
    
    void update(float deltaTime) {
        if (!isActive) return;
        
        remainingTime -= deltaTime;
        if (remainingTime <= 0.0f) {
            // 타이머 만료 - 출력 변경 적용
            applyOutputChange();
            isActive = false;
        }
    }
};
```

#### 4.2.2 적응형 업데이트 주기
```cpp
class AdaptiveFrameRate {
    float targetFrameTime = 16.67f;  // 60 FPS 목표
    float currentFrameTime;
    int performanceLevel = 3;        // 1(저성능) ~ 5(고성능)
    
public:
    void adjustPerformance() {
        if (currentFrameTime > targetFrameTime * 1.5f) {
            // 성능 저하 감지 - 최적화 적용
            performanceLevel--;
            applyOptimizations();
        } else if (currentFrameTime < targetFrameTime * 0.8f) {
            // 성능 여유 - 품질 향상
            performanceLevel++;
            improveQuality();
        }
    }
    
private:
    void applyOptimizations() {
        switch(performanceLevel) {
            case 1: // 최소 품질
                disableAnimations();
                reduceUpdateFrequency();
                break;
            case 2: // 저품질
                simplifyVisuals();
                break;
        }
    }
};
```

### 4.3 공간 최적화

#### 4.3.1 관심 영역 기반 업데이트
```cpp
class SpatialOptimizer {
    struct ViewBounds {
        int minX, minY, maxX, maxY;
    };
    
    ViewBounds currentView;
    std::unordered_set<uint32_t> activeGates;
    
public:
    void updateViewBounds(const Camera& camera) {
        // 카메라 시야에 보이는 영역 계산
        currentView = calculateViewBounds(camera);
        
        // 해당 영역의 게이트만 활성화
        updateActiveGates();
    }
    
    bool isGateInView(const Gate& gate) {
        return gate.position.x >= currentView.minX &&
               gate.position.x <= currentView.maxX &&
               gate.position.y >= currentView.minY &&
               gate.position.y <= currentView.maxY;
    }
};
```

## 5. 오류 처리 및 복구

### 5.1 예외 상황별 처리

#### 5.1.1 메모리 부족
```cpp
class MemoryManager {
    size_t maxMemoryUsage = 512 * 1024 * 1024;  // 512MB 제한
    size_t currentUsage = 0;
    
public:
    bool allocateGate() {
        size_t requiredMemory = sizeof(Gate) + sizeof(GateTimer);
        
        if (currentUsage + requiredMemory > maxMemoryUsage) {
            // 메모리 부족 - 정리 시도
            if (!cleanup()) {
                showErrorMessage("메모리 부족으로 게이트를 추가할 수 없습니다.");
                return false;
            }
        }
        
        currentUsage += requiredMemory;
        return true;
    }
    
private:
    bool cleanup() {
        // 1. 삭제된 게이트 정리
        cleanupDeletedGates();
        
        // 2. 사용하지 않는 신호 정리  
        cleanupUnusedSignals();
        
        // 3. 캐시 정리
        clearCaches();
        
        return currentUsage < maxMemoryUsage * 0.8f;
    }
};
```

#### 5.1.2 무한 루프 방지
```cpp
class LoopProtection {
    static const int MAX_PROPAGATION_DEPTH = 1000;
    int currentDepth = 0;
    
public:
    bool propagateSignal(uint32_t signalId) {
        if (currentDepth >= MAX_PROPAGATION_DEPTH) {
            showWarningMessage("신호 전파 깊이 한계 도달 - 무한 루프 방지");
            return false;
        }
        
        currentDepth++;
        bool result = performPropagation(signalId);
        currentDepth--;
        
        return result;
    }
};
```

#### 5.1.3 연결 무결성 검증
```cpp
class ConnectionValidator {
public:
    bool validateCircuit(const Circuit& circuit) {
        std::vector<ValidationError> errors;
        
        // 1. 끊어진 연결 검사
        for (const auto& wire : circuit.wires) {
            if (!isValidConnection(wire)) {
                errors.push_back({ERROR_BROKEN_WIRE, wire.id});
            }
        }
        
        // 2. 순환 의존성 검사
        if (hasCircularDependency(circuit)) {
            errors.push_back({WARNING_CIRCULAR_DEPENDENCY, 0});
        }
        
        // 3. 오류 보고 및 자동 수정
        return handleValidationErrors(errors);
    }
    
private:
    bool handleValidationErrors(const std::vector<ValidationError>& errors) {
        for (const auto& error : errors) {
            switch (error.type) {
                case ERROR_BROKEN_WIRE:
                    // 자동 수정: 끊어진 와이어 제거
                    removeWire(error.elementId);
                    showInfoMessage("끊어진 와이어가 자동으로 제거되었습니다.");
                    break;
                    
                case WARNING_CIRCULAR_DEPENDENCY:
                    // 경고만 표시 (발진 허용)
                    showWarningMessage("순환 의존성이 감지되었습니다.");
                    break;
            }
        }
        
        return errors.empty() || allErrorsFixed(errors);
    }
};
```

### 5.2 성능 저하 대응

#### 5.2.1 단계별 최적화
```cpp
class PerformanceManager {
    enum OptimizationLevel {
        FULL_QUALITY = 0,    // 모든 기능 활성
        REDUCED_EFFECTS = 1, // 애니메이션 간소화
        MINIMAL_VISUAL = 2,  // 시각 효과 최소화
        EMERGENCY_MODE = 3   // 최소 기능만 유지
    };
    
    OptimizationLevel currentLevel = FULL_QUALITY;
    
public:
    void checkPerformance(float frameTime) {
        if (frameTime > 33.33f) {  // 30 FPS 이하
            if (currentLevel < EMERGENCY_MODE) {
                currentLevel = (OptimizationLevel)(currentLevel + 1);
                applyOptimization();
                showWarningMessage("성능 향상을 위해 일부 기능이 제한됩니다.");
            }
        } else if (frameTime < 16.67f) {  // 60 FPS 이상
            if (currentLevel > FULL_QUALITY) {
                currentLevel = (OptimizationLevel)(currentLevel - 1);
                restoreFeatures();
                showInfoMessage("성능이 개선되어 기능을 복원합니다.");
            }
        }
    }
    
private:
    void applyOptimization() {
        switch (currentLevel) {
            case REDUCED_EFFECTS:
                disableParticleEffects();
                reduceAnimationQuality();
                break;
            case MINIMAL_VISUAL:
                disableAnimations();
                useStaticColors();
                break;
            case EMERGENCY_MODE:
                disableVisualFeedback();
                increaseUpdateInterval();
                break;
        }
    }
};
```

## 6. 테스트 시나리오

### 6.1 기본 기능 테스트

#### 6.1.1 단일 게이트 테스트
```
테스트 케이스 1: NOT 게이트 기본 동작
전제조건: 빈 그리드에 NOT 게이트 1개 배치
테스트 단계:
1. 입력 포트에 신호 0 연결
2. 시뮬레이션 시작
3. 0.1초 후 출력 포트 신호 확인 (예상: 1)
4. 입력 포트에 신호 1 연결  
5. 0.1초 후 출력 포트 신호 확인 (예상: 0)

예상 결과: 
- 정확한 NOT 연산 수행
- 0.1초 딜레이 정확히 적용
- 시각적 피드백 정상 표시
```

#### 6.1.2 연쇄 게이트 테스트
```
테스트 케이스 2: 3개 게이트 연쇄 연결
구성: Gate1 → Gate2 → Gate3
테스트 단계:
1. Gate1 입력에 신호 1 연결
2. 시뮬레이션 시작
3. 시간대별 출력 확인:
   - 0.0초: Gate1=1, Gate2=1, Gate3=1 (초기값)
   - 0.1초: Gate1=0, Gate2=1, Gate3=1
   - 0.2초: Gate1=0, Gate2=1, Gate3=1  
   - 0.3초: Gate1=0, Gate2=0, Gate3=0

예상 결과: 신호가 0.1초 간격으로 순차 전파
```

### 6.2 성능 테스트

#### 6.2.1 대규모 회로 테스트
```
테스트 케이스 3: 100,000 게이트 성능
구성: 100x1000 그리드에 게이트 배치
측정 항목:
- 초기화 시간 < 5초
- 평균 프레임 시간 < 16.67ms
- 메모리 사용량 < 512MB
- CPU 사용률 < 80%

스트레스 조건:
- 모든 게이트 동시 입력 변경
- 10분간 연속 실행
- 메모리 누수 확인
```

#### 6.2.2 발진 회로 성능
```
테스트 케이스 4: 다중 발진기
구성: 100개의 독립적인 3-게이트 발진기
측정 항목:
- 발진 주기 정확도 (0.3초 ±1%)
- 발진기간 성능 저하 < 5%
- 동기화 없이 독립적 발진

부하 테스트:
- 발진기 수를 점진적으로 증가
- 성능 한계점 확인
- 자동 최적화 동작 검증
```

### 6.3 사용자 경험 테스트

#### 6.3.1 반응성 테스트
```
테스트 케이스 5: 사용자 입력 반응
시나리오:
1. 재생 버튼 클릭 → 0.1초 내 시뮬레이션 시작
2. 일시정지 버튼 클릭 → 즉시 정지  
3. 속도 변경 → 0.2초 내 반영
4. 게이트 추가/삭제 → 실시간 반영

측정 기준:
- 입력 지연 < 100ms
- 시각적 피드백 < 200ms  
- UI 응답성 > 30 FPS
```

#### 6.3.2 직관성 테스트
```
테스트 케이스 6: 사용자 이해도
평가 항목:
- 신호 상태를 색상으로 구분 가능한가?
- 딜레이 중인 게이트를 인식 가능한가?
- 루프 경고 메시지가 명확한가?
- 성능 저하 시 원인 파악 가능한가?

성공 기준:
- 신호 상태 인식률 > 95%
- 오류 메시지 이해도 > 90%
- 기능 발견률 > 80%
```

## 7. 확장성 고려사항

### 7.1 추가 게이트 타입 지원
```cpp
// 확장 가능한 게이트 시스템
class GateProcessor {
    std::unordered_map<GateType, std::unique_ptr<GateLogic>> processors;
    
public:
    void registerGateType(GateType type, std::unique_ptr<GateLogic> logic) {
        processors[type] = std::move(logic);
    }
    
    bool processGate(Gate& gate, float deltaTime) {
        auto it = processors.find(gate.type);
        if (it != processors.end()) {
            return it->second->process(gate, deltaTime);
        }
        return false;
    }
};

// NOT 게이트 전용 로직
class NotGateLogic : public GateLogic {
public:
    bool process(Gate& gate, float deltaTime) override {
        bool input1 = getSignal(gate.inputs[0]);
        bool input2 = getSignal(gate.inputs[1]); 
        bool input3 = getSignal(gate.inputs[2]);
        
        bool newOutput = !(input1 || input2 || input3);
        
        return scheduleOutputChange(gate, newOutput, 0.1f);
    }
};
```

### 7.2 네트워크 멀티플레이어 대비
```cpp
// 동기화 가능한 시뮬레이션 상태
struct SimulationSnapshot {
    uint64_t timestamp;
    std::vector<uint8_t> signalStates;
    std::vector<GateTimer> activeTimers;
    uint32_t checksum;
    
    void serialize(BinaryWriter& writer) const;
    void deserialize(BinaryReader& reader);
    bool validate() const;
};

class NetworkSync {
    std::queue<SimulationSnapshot> snapshotHistory;
    
public:
    void saveSnapshot(const SimulationSnapshot& snapshot) {
        snapshotHistory.push(snapshot);
        if (snapshotHistory.size() > 60) {  // 1초간 히스토리 유지
            snapshotHistory.pop();
        }
    }
    
    bool rollbackToSnapshot(uint64_t timestamp) {
        // 특정 시점으로 롤백 (네트워크 동기화용)
        return findAndApplySnapshot(timestamp);
    }
};
```

### 7.3 GPU 가속 준비
```cpp
// GPU 컴퓨트 셰이더 인터페이스
class GPUSimulator {
    struct GPUGateData {
        uint32_t inputs[3];
        uint32_t output;
        float timer;
        uint32_t type;
    };
    
    std::vector<GPUGateData> gpuGates;
    ComputeShader signalPropagationShader;
    
public:
    void uploadGateData() {
        // CPU 데이터를 GPU 버퍼로 업로드
        signalPropagationShader.setBuffer("gateData", gpuGates);
    }
    
    void executeSimulationStep(float deltaTime) {
        signalPropagationShader.setFloat("deltaTime", deltaTime);
        signalPropagationShader.dispatch(gpuGates.size() / 64, 1, 1);
    }
    
    void downloadResults() {
        // GPU 결과를 CPU로 다운로드
        signalPropagationShader.getBuffer("gateData", gpuGates);
    }
};
```

## 8. 구현 검증 체크리스트

### 8.1 핵심 기능 검증
- [ ] NOT 게이트 진리표 정확성 (모든 입력 조합)
- [ ] 0.1초 딜레이 정확도 (±1ms 오차 범위)
- [ ] 와이어 즉시 전파 (지연 없음)
- [ ] 순환 의존성 감지 및 발진 동작
- [ ] 대규모 회로에서 신호 전파 정확성

### 8.2 성능 기준 검증  
- [ ] 100,000 게이트에서 60 FPS 유지
- [ ] 메모리 사용량 예상 범위 내 (게이트당 64바이트)
- [ ] CPU 사용률 합리적 수준 (80% 미만)
- [ ] 메모리 누수 없음 (10분 연속 실행)

### 8.3 사용자 경험 검증
- [ ] 직관적인 시각적 피드백 (색상, 애니메이션)  
- [ ] 빠른 입력 반응성 (100ms 미만)
- [ ] 명확한 오류 메시지 및 가이드
- [ ] 성능 저하 시 자동 최적화 동작

### 8.4 시스템 통합 검증
- [ ] 렌더링 시스템과 정상 연동
- [ ] 입력 처리 시스템과 실시간 상호작용
- [ ] 게이트/와이어 편집 시 안정성
- [ ] 기존 기능과의 호환성 유지

이 기능 명세서는 Step 7 구현의 상세한 가이드라인을 제공하며, 사용자 경험부터 내부 알고리즘까지 모든 측면을 포괄적으로 다룹니다.