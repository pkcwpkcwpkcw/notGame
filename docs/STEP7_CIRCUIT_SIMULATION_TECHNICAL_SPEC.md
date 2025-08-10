# Step 7: 회로 시뮬레이션 엔진 기술 명세서

## 1. 개요

### 1.1 문서 목적
본 문서는 NOT Gate 게임의 Step 7 구현을 위한 상세한 기술 설계를 정의합니다. 시스템 아키텍처부터 구체적인 API, 데이터 구조, 알고리즘까지 실제 구현에 필요한 모든 기술적 세부사항을 포함합니다.

### 1.2 기술 스택
- **언어**: C++20
- **그래픽스**: SDL2 + OpenGL 3.3+
- **UI**: Dear ImGui
- **빌드**: CMake 3.20+
- **컴파일러**: MSVC 2022, GCC 11+, Clang 13+

### 1.3 성능 목표
- 100,000개 게이트에서 60 FPS 유지
- 메모리 사용량: 게이트당 64바이트 이하
- 시뮬레이션 업데이트 시간: 16ms 이하

## 2. 시스템 아키텍처

### 2.1 전체 아키텍처 다이어그램

```
┌─────────────────────────────────────────────────────────┐
│                    Game Engine                          │
├─────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐   │
│  │   Renderer   │  │  InputMgr    │  │   UIMgr      │   │
│  │   (Step 3)   │  │  (Step 4)    │  │  (ImGui)     │   │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘   │
│         │                 │                 │           │
├─────────┼─────────────────┼─────────────────┼───────────┤
│         │                 │                 │           │
│  ┌──────▼──────────────────▼─────────────────▼─────┐     │
│  │           CircuitSimulator (Main)             │     │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────────┐   │     │
│  │  │ Signal   │ │  Timer   │ │ Performance  │   │     │
│  │  │ Manager  │ │ Manager  │ │   Manager    │   │     │
│  │  └──────────┘ └──────────┘ └──────────────┘   │     │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────────┐   │     │
│  │  │  Loop    │ │ Memory   │ │   Spatial    │   │     │
│  │  │ Detector │ │ Manager  │ │  Optimizer   │   │     │
│  │  └──────────┘ └──────────┘ └──────────────┘   │     │
│  └───────────────────────────────────────────────┘     │
├─────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐   │
│  │   Circuit    │  │    Gate      │  │    Wire      │   │
│  │   (Step 2)   │  │  (Step 5)    │  │  (Step 6)    │   │
│  └──────────────┘  └──────────────┘  └──────────────┘   │
└─────────────────────────────────────────────────────────┘
```

### 2.2 디자인 패턴

#### 2.2.1 Observer Pattern (시각적 피드백)
```cpp
class ISimulationObserver {
public:
    virtual ~ISimulationObserver() = default;
    virtual void onSignalChanged(uint32_t signalId, bool newValue) = 0;
    virtual void onGateStateChanged(uint32_t gateId, GateState newState) = 0;
    virtual void onLoopDetected(const std::vector<uint32_t>& loopGates) = 0;
};

class CircuitSimulator {
    std::vector<ISimulationObserver*> observers;
    
public:
    void addObserver(ISimulationObserver* observer) {
        observers.push_back(observer);
    }
    
private:
    void notifySignalChanged(uint32_t signalId, bool newValue) {
        for (auto* observer : observers) {
            observer->onSignalChanged(signalId, newValue);
        }
    }
};
```

#### 2.2.2 Command Pattern (시뮬레이션 제어)
```cpp
enum class SimulationCommand {
    START,
    PAUSE,
    STOP,
    STEP,
    SET_SPEED
};

class SimulationCommandProcessor {
public:
    void executeCommand(SimulationCommand cmd, float param = 0.0f) {
        switch (cmd) {
            case SimulationCommand::START:
                simulator->start();
                break;
            case SimulationCommand::SET_SPEED:
                simulator->setSpeed(param);
                break;
        }
    }
};
```

#### 2.2.3 Strategy Pattern (게이트 처리)
```cpp
class IGateProcessor {
public:
    virtual ~IGateProcessor() = default;
    virtual bool processGate(const Gate& gate, float deltaTime) = 0;
    virtual float getDelay() const = 0;
};

class NotGateProcessor : public IGateProcessor {
public:
    bool processGate(const Gate& gate, float deltaTime) override {
        // NOT 게이트 전용 로직
        bool input1 = getSignal(gate.inputs[0]);
        bool input2 = getSignal(gate.inputs[1]);
        bool input3 = getSignal(gate.inputs[2]);
        
        bool newOutput = !(input1 || input2 || input3);
        
        if (newOutput != getSignal(gate.output)) {
            scheduleOutputChange(gate.id, newOutput, getDelay());
            return true;
        }
        return false;
    }
    
    float getDelay() const override { return 0.1f; }
};
```

## 3. 핵심 클래스 설계

### 3.1 CircuitSimulator (메인 클래스)
```cpp
class CircuitSimulator {
public:
    // 생성자/소멸자
    CircuitSimulator(Circuit* circuit);
    ~CircuitSimulator();
    
    // 시뮬레이션 제어
    void initialize();
    void start();
    void pause();
    void stop();
    void reset();
    void update(float deltaTime);
    
    // 상태 조회
    bool isRunning() const { return state == SimulationState::RUNNING; }
    bool getSignalState(uint32_t signalId) const;
    GateState getGateState(uint32_t gateId) const;
    
    // 외부 신호 제어 (퍼즐 모드용)
    void setExternalSignal(uint32_t signalId, bool value);
    
    // 디버깅
    bool detectLoops();
    std::vector<uint32_t> getActiveGates() const;
    PerformanceStats getPerformanceStats() const;
    
    // Observer 패턴
    void addObserver(ISimulationObserver* observer);
    void removeObserver(ISimulationObserver* observer);

private:
    enum class SimulationState {
        STOPPED,
        RUNNING,
        PAUSED
    };
    
    // 멤버 변수
    Circuit* circuit;
    SimulationState state;
    float simulationSpeed;
    
    // 하위 시스템들
    std::unique_ptr<SignalManager> signalManager;
    std::unique_ptr<TimerManager> timerManager;
    std::unique_ptr<LoopDetector> loopDetector;
    std::unique_ptr<PerformanceManager> perfManager;
    std::unique_ptr<MemoryManager> memoryManager;
    std::unique_ptr<SpatialOptimizer> spatialOptimizer;
    
    // Observer 목록
    std::vector<ISimulationObserver*> observers;
    
    // 내부 메서드
    void updateTimers(float deltaTime);
    void processExpiredTimers();
    void propagateSignals();
    void detectInputChanges();
    void optimizePerformance();
    
    void notifyObservers();
};
```

### 3.2 SignalManager (신호 상태 관리)
```cpp
class SignalManager {
public:
    static constexpr size_t SIGNALS_PER_WORD = 32;
    static constexpr size_t MAX_SIGNALS = 1000000;
    static constexpr size_t SIGNAL_WORDS = MAX_SIGNALS / SIGNALS_PER_WORD;
    
    SignalManager();
    ~SignalManager();
    
    // 신호 상태 조회/설정
    bool getSignal(uint32_t signalId) const;
    void setSignal(uint32_t signalId, bool value);
    
    // 배치 처리
    void setMultipleSignals(const std::vector<std::pair<uint32_t, bool>>& signals);
    std::vector<uint32_t> getChangedSignals() const;
    
    // 최적화된 연산
    void clearAllSignals();
    void applyBatchChanges();
    
    // SIMD 최적화된 연산
    void propagateSignalsSIMD();

private:
    // Structure of Arrays 패턴으로 캐시 효율성 극대화
    alignas(64) uint32_t signalBits[SIGNAL_WORDS];
    alignas(64) uint32_t previousBits[SIGNAL_WORDS];  // 변경 감지용
    alignas(64) uint32_t dirtyMask[SIGNAL_WORDS];     // 더티 플래그
    
    // 변경된 신호 추적
    std::vector<uint32_t> changedSignals;
    std::vector<std::pair<uint32_t, bool>> pendingChanges;
    
    // 스레드 안전성
    mutable std::shared_mutex signalMutex;
    
    // 내부 메서드
    void markDirty(uint32_t signalId);
    void updateChangedList();
    void applyPendingChanges();
};

// SIMD 최적화 구현 예시
#ifdef __AVX2__
void SignalManager::propagateSignalsSIMD() {
    const size_t simdWords = SIGNAL_WORDS / 8; // 256비트씩 처리
    
    for (size_t i = 0; i < simdWords; ++i) {
        // 8개의 uint32_t를 한 번에 로드
        __m256i current = _mm256_load_si256(
            reinterpret_cast<const __m256i*>(&signalBits[i * 8]));
        __m256i previous = _mm256_load_si256(
            reinterpret_cast<const __m256i*>(&previousBits[i * 8]));
        
        // XOR로 변경된 비트 감지
        __m256i changed = _mm256_xor_si256(current, previous);
        
        // 변경된 비트가 있으면 처리
        if (!_mm256_testz_si256(changed, changed)) {
            processChangedBatch(i * 8, changed);
        }
        
        // 이전 상태 업데이트
        _mm256_store_si256(
            reinterpret_cast<__m256i*>(&previousBits[i * 8]), current);
    }
}
#endif
```

### 3.3 TimerManager (타이머 시스템)
```cpp
class TimerManager {
public:
    struct GateTimer {
        uint32_t gateId;
        float remainingTime;
        bool pendingOutput;
        uint8_t priority;  // 동시 만료 시 처리 순서
        
        bool operator<(const GateTimer& other) const {
            // 시간이 빠른 순서, 우선순위가 높은 순서
            if (remainingTime != other.remainingTime) {
                return remainingTime > other.remainingTime; // min-heap
            }
            return priority > other.priority;
        }
    };
    
    TimerManager();
    
    // 타이머 관리
    void scheduleTimer(uint32_t gateId, float delay, bool pendingOutput);
    void cancelTimer(uint32_t gateId);
    bool hasActiveTimer(uint32_t gateId) const;
    
    // 업데이트
    void updateTimers(float deltaTime);
    std::vector<std::pair<uint32_t, bool>> getExpiredTimers();
    
    // 상태 조회
    size_t getActiveTimerCount() const;
    float getRemainingTime(uint32_t gateId) const;

private:
    // 우선순위 큐로 효율적인 타이머 관리
    std::priority_queue<GateTimer> timerQueue;
    std::unordered_map<uint32_t, GateTimer*> gateToTimer;
    
    // 만료된 타이머 임시 저장
    std::vector<std::pair<uint32_t, bool>> expiredTimers;
    
    // 스레드 안전성
    std::mutex timerMutex;
    
    void cleanupExpiredTimers();
};
```

### 3.4 LoopDetector (루프 감지)
```cpp
class LoopDetector {
public:
    struct LoopInfo {
        std::vector<uint32_t> gateIds;
        float oscillationPeriod;
        bool isStable;
    };
    
    LoopDetector(const Circuit* circuit);
    
    // 루프 감지
    bool detectLoops();
    std::vector<LoopInfo> getAllLoops() const;
    bool isGateInLoop(uint32_t gateId) const;
    
    // 발진 분석
    float calculateOscillationPeriod(const LoopInfo& loop) const;
    bool isPotentiallyStable(const LoopInfo& loop) const;

private:
    const Circuit* circuit;
    std::vector<LoopInfo> detectedLoops;
    std::unordered_set<uint32_t> loopGates;
    
    // DFS 상태
    enum class DFSState {
        WHITE,  // 미방문
        GRAY,   // 방문 중
        BLACK   // 방문 완료
    };
    
    std::vector<DFSState> dfsState;
    std::vector<uint32_t> dfsStack;
    
    // 내부 메서드
    bool dfsVisit(uint32_t gateId, std::vector<uint32_t>& currentPath);
    void extractLoop(const std::vector<uint32_t>& path, size_t loopStart);
    void buildAdjacencyList();
    
    // 인접 리스트 (캐싱용)
    std::vector<std::vector<uint32_t>> adjacencyList;
    bool adjacencyListValid;
};
```

### 3.5 PerformanceManager (성능 관리)
```cpp
class PerformanceManager {
public:
    enum class OptimizationLevel {
        ULTRA_HIGH = 0,
        HIGH = 1,
        MEDIUM = 2,
        LOW = 3,
        EMERGENCY = 4
    };
    
    struct PerformanceStats {
        float frameTime;
        float simulationTime;
        size_t activeGates;
        size_t signalChanges;
        size_t memoryUsage;
        OptimizationLevel currentLevel;
    };
    
    PerformanceManager();
    
    // 성능 모니터링
    void beginFrame();
    void endFrame();
    void recordSimulationTime(float time);
    
    // 적응형 최적화
    void updateOptimization();
    OptimizationLevel getCurrentLevel() const { return currentLevel; }
    
    // 통계
    PerformanceStats getStats() const;
    void resetStats();
    
    // 최적화 설정
    void enableOptimization(const std::string& name, bool enabled);
    bool isOptimizationEnabled(const std::string& name) const;

private:
    OptimizationLevel currentLevel;
    
    // 성능 측정
    std::chrono::high_resolution_clock::time_point frameStart;
    float frameTime;
    float simulationTime;
    
    // 통계 수집
    struct FrameStats {
        float frameTime;
        float simulationTime;
        size_t activeGates;
        size_t signalChanges;
    };
    
    std::deque<FrameStats> frameHistory;
    static constexpr size_t MAX_FRAME_HISTORY = 60; // 1초간
    
    // 최적화 플래그
    std::unordered_map<std::string, bool> optimizationFlags;
    
    // 적응형 조절
    void adjustOptimizationLevel();
    void applyOptimizations();
    void restoreFeatures();
    
    // 내부 메서드
    float getAverageFrameTime() const;
    bool shouldIncreaseOptimization() const;
    bool shouldDecreaseOptimization() const;
};
```

## 4. 데이터 구조 설계

### 4.1 메모리 레이아웃 최적화

#### 4.1.1 Gate 구조체 (캐시 효율적)
```cpp
// Structure of Arrays 패턴
struct GateArrays {
    // 기본 정보 (자주 접근)
    alignas(64) std::vector<Vec2> positions;
    alignas(64) std::vector<GateType> types;
    alignas(64) std::vector<GateState> states;
    
    // 연결 정보
    alignas(64) std::vector<std::array<uint32_t, 3>> inputs;
    alignas(64) std::vector<uint32_t> outputs;
    
    // 시뮬레이션 상태
    alignas(64) std::vector<bool> currentOutputs;
    alignas(64) std::vector<bool> pendingOutputs;
    alignas(64) std::vector<float> timerValues;
    
    // 최적화 정보
    alignas(64) std::vector<uint8_t> updatePriority;
    alignas(64) std::vector<bool> needsUpdate;
    
    size_t count = 0;
    
    void reserve(size_t capacity) {
        positions.reserve(capacity);
        types.reserve(capacity);
        states.reserve(capacity);
        inputs.reserve(capacity);
        outputs.reserve(capacity);
        currentOutputs.reserve(capacity);
        pendingOutputs.reserve(capacity);
        timerValues.reserve(capacity);
        updatePriority.reserve(capacity);
        needsUpdate.reserve(capacity);
    }
};
```

#### 4.1.2 신호 네트워크 구조
```cpp
struct SignalNetwork {
    // 신호 ID를 비트 인덱스로 사용
    struct SignalNode {
        uint32_t signalId;
        std::vector<uint32_t> connectedGates;  // 이 신호에 연결된 게이트들
        std::vector<uint32_t> connectedWires;  // 이 신호에 연결된 와이어들
        bool isExternal;  // 외부 신호 여부 (퍼즐 모드용)
    };
    
    std::vector<SignalNode> nodes;
    std::unordered_map<uint32_t, size_t> signalToNode;
    
    // 위상학적 정렬 결과 캐싱
    std::vector<uint32_t> topologicalOrder;
    bool topologyValid = false;
    
    void invalidateTopology() { topologyValid = false; }
    void updateTopology();
};
```

### 4.2 메모리 풀 시스템
```cpp
template<typename T, size_t BlockSize = 1024>
class ObjectPool {
public:
    ObjectPool() {
        allocateNewBlock();
    }
    
    ~ObjectPool() {
        for (auto* block : blocks) {
            delete[] block;
        }
    }
    
    T* acquire() {
        if (freeList.empty()) {
            allocateNewBlock();
        }
        
        T* obj = freeList.back();
        freeList.pop_back();
        return new(obj) T();  // placement new
    }
    
    void release(T* obj) {
        obj->~T();  // 명시적 소멸자 호출
        freeList.push_back(obj);
    }
    
    size_t getTotalCapacity() const {
        return blocks.size() * BlockSize;
    }
    
    size_t getUsedCount() const {
        return getTotalCapacity() - freeList.size();
    }

private:
    std::vector<T*> blocks;
    std::vector<T*> freeList;
    
    void allocateNewBlock() {
        T* newBlock = new T[BlockSize];
        blocks.push_back(newBlock);
        
        for (size_t i = 0; i < BlockSize; ++i) {
            freeList.push_back(&newBlock[i]);
        }
    }
};

// 특화된 풀들
using GateTimerPool = ObjectPool<TimerManager::GateTimer, 1024>;
using SignalChangePool = ObjectPool<std::pair<uint32_t, bool>, 2048>;
```

## 5. 핵심 알고리즘

### 5.1 신호 전파 알고리즘
```cpp
class SignalPropagator {
public:
    void propagateChanges(const std::vector<uint32_t>& changedSignals) {
        if (changedSignals.empty()) return;
        
        // 1. 위상학적 정렬 순서대로 처리
        std::vector<uint32_t> affectedGates = 
            findAffectedGates(changedSignals);
        
        // 2. 우선순위에 따라 정렬
        sortByPriority(affectedGates);
        
        // 3. 배치 처리로 효율성 향상
        processBatch(affectedGates);
    }

private:
    std::vector<uint32_t> findAffectedGates(
        const std::vector<uint32_t>& changedSignals) {
        
        std::unordered_set<uint32_t> affectedSet;
        
        for (uint32_t signalId : changedSignals) {
            const auto& node = signalNetwork.getNode(signalId);
            
            // 이 신호에 연결된 모든 게이트 추가
            for (uint32_t gateId : node.connectedGates) {
                affectedSet.insert(gateId);
            }
            
            // 와이어를 통해 연결된 게이트들도 추가
            for (uint32_t wireId : node.connectedWires) {
                const Wire& wire = circuit->getWire(wireId);
                propagateWire(wire, affectedSet);
            }
        }
        
        return std::vector<uint32_t>(affectedSet.begin(), affectedSet.end());
    }
    
    void processBatch(const std::vector<uint32_t>& gates) {
        // SIMD 최적화가 가능한 경우 배치 처리
        constexpr size_t BATCH_SIZE = 8;
        
        for (size_t i = 0; i < gates.size(); i += BATCH_SIZE) {
            size_t batchEnd = std::min(i + BATCH_SIZE, gates.size());
            processGateBatch(gates.data() + i, batchEnd - i);
        }
    }
    
    void processGateBatch(const uint32_t* gateIds, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            uint32_t gateId = gateIds[i];
            processGate(gateId);
        }
    }
};
```

### 5.2 위상학적 정렬 (Kahn's Algorithm)
```cpp
class TopologicalSorter {
public:
    std::vector<uint32_t> sort(const Circuit& circuit) {
        buildGraph(circuit);
        return kahnSort();
    }

private:
    std::vector<std::vector<uint32_t>> adjacencyList;
    std::vector<int> inDegree;
    
    void buildGraph(const Circuit& circuit) {
        size_t gateCount = circuit.getGates().size();
        adjacencyList.resize(gateCount);
        inDegree.resize(gateCount, 0);
        
        // 게이트간 의존성 그래프 구축
        for (const auto& wire : circuit.getWires()) {
            uint32_t fromGate = findGateByOutput(wire.from);
            uint32_t toGate = findGateByInput(wire.to);
            
            if (fromGate != INVALID_GATE && toGate != INVALID_GATE) {
                adjacencyList[fromGate].push_back(toGate);
                inDegree[toGate]++;
            }
        }
    }
    
    std::vector<uint32_t> kahnSort() {
        std::vector<uint32_t> result;
        std::queue<uint32_t> zeroInDegree;
        
        // 진입 차수가 0인 노드들을 큐에 추가
        for (size_t i = 0; i < inDegree.size(); ++i) {
            if (inDegree[i] == 0) {
                zeroInDegree.push(static_cast<uint32_t>(i));
            }
        }
        
        while (!zeroInDegree.empty()) {
            uint32_t current = zeroInDegree.front();
            zeroInDegree.pop();
            result.push_back(current);
            
            // 인접한 노드들의 진입 차수 감소
            for (uint32_t neighbor : adjacencyList[current]) {
                inDegree[neighbor]--;
                if (inDegree[neighbor] == 0) {
                    zeroInDegree.push(neighbor);
                }
            }
        }
        
        return result;
    }
};
```

### 5.3 적응형 시간 스텝 알고리즘
```cpp
class AdaptiveTimeStep {
public:
    struct TimeStepInfo {
        float fixedStep;      // 기본 시간 스텝 (0.1초)
        float currentStep;    // 현재 사용 중인 스텝
        float accumulator;    // 누적된 시간
        float speedMultiplier; // 사용자 설정 속도
    };
    
    void update(float deltaTime, TimeStepInfo& info) {
        float adjustedDelta = deltaTime * info.speedMultiplier;
        info.accumulator += adjustedDelta;
        
        // 적응형 스텝 크기 조정
        adjustStepSize(info, adjustedDelta);
        
        // 시뮬레이션 스텝 실행
        while (info.accumulator >= info.currentStep) {
            performSimulationStep(info.currentStep);
            info.accumulator -= info.currentStep;
        }
    }

private:
    void adjustStepSize(TimeStepInfo& info, float deltaTime) {
        // 성능에 따라 스텝 크기 동적 조정
        float targetFrameTime = 16.67f; // 60 FPS
        float currentFrameTime = getCurrentFrameTime();
        
        if (currentFrameTime > targetFrameTime * 1.5f) {
            // 성능 저하 시 스텝 크기 증가 (정확도 낮춤)
            info.currentStep = std::min(info.currentStep * 1.1f, 
                                      info.fixedStep * 2.0f);
        } else if (currentFrameTime < targetFrameTime * 0.8f) {
            // 성능 여유 시 스텝 크기 감소 (정확도 높임)
            info.currentStep = std::max(info.currentStep * 0.9f, 
                                      info.fixedStep * 0.5f);
        }
    }
    
    void performSimulationStep(float stepSize) {
        // 실제 시뮬레이션 스텝 실행
        timerManager->updateTimers(stepSize);
        processExpiredTimers();
        propagateSignals();
    }
};
```

## 6. API 설계

### 6.1 공개 인터페이스
```cpp
// C-style API (다른 언어 바인딩을 위해)
extern "C" {
    // 시뮬레이터 생명주기
    SimulatorHandle* simulator_create(Circuit* circuit);
    void simulator_destroy(SimulatorHandle* handle);
    void simulator_initialize(SimulatorHandle* handle);
    
    // 시뮬레이션 제어
    void simulator_start(SimulatorHandle* handle);
    void simulator_pause(SimulatorHandle* handle);
    void simulator_stop(SimulatorHandle* handle);
    void simulator_update(SimulatorHandle* handle, float deltaTime);
    
    // 상태 조회
    bool simulator_is_running(SimulatorHandle* handle);
    bool simulator_get_signal(SimulatorHandle* handle, uint32_t signalId);
    int simulator_get_gate_state(SimulatorHandle* handle, uint32_t gateId);
    
    // 성능 통계
    void simulator_get_stats(SimulatorHandle* handle, PerformanceStats* stats);
    
    // 디버깅
    bool simulator_detect_loops(SimulatorHandle* handle);
    uint32_t* simulator_get_loop_gates(SimulatorHandle* handle, size_t* count);
}
```

### 6.2 이벤트 콜백 시스템
```cpp
// 이벤트 타입 정의
enum class SimulationEventType {
    SIGNAL_CHANGED,
    GATE_STATE_CHANGED,
    LOOP_DETECTED,
    PERFORMANCE_WARNING,
    ERROR_OCCURRED
};

// 이벤트 데이터 구조
struct SimulationEvent {
    SimulationEventType type;
    uint64_t timestamp;
    
    union {
        struct {
            uint32_t signalId;
            bool newValue;
        } signalChanged;
        
        struct {
            uint32_t gateId;
            GateState newState;
        } gateStateChanged;
        
        struct {
            uint32_t* gateIds;
            size_t count;
            float period;
        } loopDetected;
        
        struct {
            const char* message;
            int severity;
        } error;
    } data;
};

// 콜백 함수 타입
using SimulationEventCallback = void(*)(const SimulationEvent& event, void* userData);

// 콜백 등록 API
void simulator_register_callback(SimulatorHandle* handle,
                               SimulationEventType eventType,
                               SimulationEventCallback callback,
                               void* userData);
```

## 7. 통합 설계

### 7.1 렌더링 시스템 연동 (Step 3)
```cpp
class SimulationRenderer : public ISimulationObserver {
public:
    SimulationRenderer(Renderer* renderer) : renderer(renderer) {}
    
    // Observer 인터페이스 구현
    void onSignalChanged(uint32_t signalId, bool newValue) override {
        // 신호 상태 변경을 렌더러에 전달
        renderer->updateSignalVisual(signalId, newValue);
        
        // 신호 전파 애니메이션 트리거
        if (newValue) {
            renderer->triggerSignalAnimation(signalId);
        }
    }
    
    void onGateStateChanged(uint32_t gateId, GateState newState) override {
        // 게이트 시각적 상태 업데이트
        renderer->updateGateVisual(gateId, newState);
        
        // 딜레이 타이머 진행률 표시
        if (newState == GateState::PROCESSING) {
            float progress = getTimerProgress(gateId);
            renderer->updateGateTimer(gateId, progress);
        }
    }
    
    void onLoopDetected(const std::vector<uint32_t>& loopGates) override {
        // 루프에 포함된 게이트들을 시각적으로 강조
        for (uint32_t gateId : loopGates) {
            renderer->highlightGate(gateId, HighlightType::LOOP);
        }
    }

private:
    Renderer* renderer;
};
```

### 7.2 입력 처리 시스템 연동 (Step 4)
```cpp
class SimulationInputHandler {
public:
    SimulationInputHandler(CircuitSimulator* simulator) 
        : simulator(simulator) {}
    
    void handleInput(const InputEvent& event) {
        switch (event.type) {
            case InputEventType::SPACE_PRESSED:
                // 스페이스바로 재생/일시정지 토글
                if (simulator->isRunning()) {
                    simulator->pause();
                } else {
                    simulator->start();
                }
                break;
                
            case InputEventType::RIGHT_CLICK:
                // 우클릭으로 신호 추적 시작
                startSignalTrace(event.position);
                break;
                
            case InputEventType::KEY_PRESSED:
                handleKeyboardShortcut(event.keyCode);
                break;
        }
    }

private:
    CircuitSimulator* simulator;
    
    void startSignalTrace(Vec2 position) {
        // 클릭 위치에서 가장 가까운 신호 찾기
        uint32_t signalId = findNearestSignal(position);
        if (signalId != INVALID_SIGNAL) {
            simulator->enableSignalTrace(signalId, true);
        }
    }
    
    void handleKeyboardShortcut(int keyCode) {
        switch (keyCode) {
            case KEY_1: simulator->setSpeed(0.1f); break;
            case KEY_2: simulator->setSpeed(0.5f); break;
            case KEY_3: simulator->setSpeed(1.0f); break;
            case KEY_4: simulator->setSpeed(2.0f); break;
            case KEY_5: simulator->setSpeed(5.0f); break;
        }
    }
};
```

### 7.3 UI 시스템 연동 (Dear ImGui)
```cpp
class SimulationUI {
public:
    SimulationUI(CircuitSimulator* simulator) : simulator(simulator) {}
    
    void render() {
        renderControlPanel();
        renderPerformancePanel();
        renderDebugPanel();
        renderStatusBar();
    }

private:
    CircuitSimulator* simulator;
    bool showPerformancePanel = true;
    bool showDebugPanel = false;
    
    void renderControlPanel() {
        if (ImGui::Begin("Simulation Control")) {
            // 재생 컨트롤 버튼
            if (simulator->isRunning()) {
                if (ImGui::Button("⏸ Pause")) {
                    simulator->pause();
                }
            } else {
                if (ImGui::Button("▶ Play")) {
                    simulator->start();
                }
            }
            
            ImGui::SameLine();
            if (ImGui::Button("⏹ Stop")) {
                simulator->stop();
            }
            
            // 속도 조절 슬라이더
            static float speed = 1.0f;
            if (ImGui::SliderFloat("Speed", &speed, 0.1f, 10.0f, "%.1fx")) {
                simulator->setSpeed(speed);
            }
            
            // 빠른 속도 버튼들
            ImGui::Text("Quick Speed:");
            ImGui::SameLine();
            if (ImGui::SmallButton("0.1x")) { speed = 0.1f; simulator->setSpeed(speed); }
            ImGui::SameLine();
            if (ImGui::SmallButton("1x")) { speed = 1.0f; simulator->setSpeed(speed); }
            ImGui::SameLine();
            if (ImGui::SmallButton("5x")) { speed = 5.0f; simulator->setSpeed(speed); }
            ImGui::SameLine();
            if (ImGui::SmallButton("10x")) { speed = 10.0f; simulator->setSpeed(speed); }
        }
        ImGui::End();
    }
    
    void renderPerformancePanel() {
        if (!showPerformancePanel) return;
        
        if (ImGui::Begin("Performance", &showPerformancePanel)) {
            PerformanceStats stats = simulator->getPerformanceStats();
            
            ImGui::Text("FPS: %.1f", 1000.0f / stats.frameTime);
            ImGui::Text("Frame Time: %.2f ms", stats.frameTime);
            ImGui::Text("Simulation Time: %.2f ms", stats.simulationTime);
            ImGui::Text("Active Gates: %zu", stats.activeGates);
            ImGui::Text("Signal Changes: %zu", stats.signalChanges);
            ImGui::Text("Memory Usage: %.1f MB", stats.memoryUsage / 1024.0f / 1024.0f);
            
            // 최적화 레벨 표시
            const char* levelNames[] = { "Ultra High", "High", "Medium", "Low", "Emergency" };
            ImGui::Text("Optimization: %s", levelNames[static_cast<int>(stats.currentLevel)]);
            
            // 성능 그래프 (간단한 히스토리)
            static float frameTimeHistory[100] = {};
            static int historyIndex = 0;
            frameTimeHistory[historyIndex] = stats.frameTime;
            historyIndex = (historyIndex + 1) % 100;
            
            ImGui::PlotLines("Frame Time", frameTimeHistory, 100, historyIndex, 
                           nullptr, 0.0f, 33.33f, ImVec2(0, 80));
        }
        ImGui::End();
    }
    
    void renderDebugPanel() {
        if (!showDebugPanel) return;
        
        if (ImGui::Begin("Debug", &showDebugPanel)) {
            // 루프 감지
            if (ImGui::Button("Detect Loops")) {
                bool hasLoops = simulator->detectLoops();
                if (hasLoops) {
                    ImGui::OpenPopup("Loop Warning");
                }
            }
            
            if (ImGui::BeginPopupModal("Loop Warning")) {
                ImGui::Text("Signal loops detected in the circuit!");
                ImGui::Text("The circuit may oscillate.");
                if (ImGui::Button("OK")) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            
            // 신호 추적
            ImGui::Separator();
            ImGui::Text("Signal Tracing");
            
            static uint32_t traceSignalId = 0;
            ImGui::InputScalar("Signal ID", ImGuiDataType_U32, &traceSignalId);
            
            if (ImGui::Button("Start Trace")) {
                simulator->enableSignalTrace(traceSignalId, true);
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Stop Trace")) {
                simulator->enableSignalTrace(traceSignalId, false);
            }
        }
        ImGui::End();
    }
    
    void renderStatusBar() {
        ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetIO().DisplaySize.y - 25));
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 25));
        
        if (ImGui::Begin("Status", nullptr, 
                        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
            
            std::string status;
            if (simulator->isRunning()) {
                status = "Simulation Running";
            } else {
                status = "Simulation Stopped";
            }
            
            ImGui::Text("%s", status.c_str());
        }
        ImGui::End();
    }
};
```

## 8. 빌드 및 배포 설정

### 8.1 CMake 설정 추가
```cmake
# Step 7 시뮬레이션 엔진 관련 파일들
set(STEP7_SOURCES
    src/simulation/CircuitSimulator.cpp
    src/simulation/SignalManager.cpp
    src/simulation/TimerManager.cpp
    src/simulation/LoopDetector.cpp
    src/simulation/PerformanceManager.cpp
    src/simulation/MemoryManager.cpp
    src/simulation/SpatialOptimizer.cpp
)

set(STEP7_HEADERS
    src/simulation/CircuitSimulator.h
    src/simulation/SignalManager.h
    src/simulation/TimerManager.h
    src/simulation/LoopDetector.h
    src/simulation/PerformanceManager.h
    src/simulation/MemoryManager.h
    src/simulation/SpatialOptimizer.h
    src/simulation/ISimulationObserver.h
)

# 소스 파일을 기존 타겟에 추가
target_sources(notgate3 PRIVATE 
    ${STEP7_SOURCES}
    ${STEP7_HEADERS}
)

# SIMD 최적화 컴파일러 플래그
if(MSVC)
    target_compile_options(notgate3 PRIVATE /arch:AVX2)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(notgate3 PRIVATE -mavx2 -mfma)
endif()

# 성능 최적화 플래그
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        target_compile_options(notgate3 PRIVATE /O2 /Ot /GL)
        target_link_options(notgate3 PRIVATE /LTCG)
    else()
        target_compile_options(notgate3 PRIVATE -O3 -march=native -flto)
        target_link_options(notgate3 PRIVATE -flto)
    endif()
endif()

# 벤치마킹을 위한 별도 실행파일
add_executable(notgate3_benchmark
    src/benchmark/SimulationBenchmark.cpp
    ${STEP7_SOURCES}
)

target_link_libraries(notgate3_benchmark PRIVATE ${COMMON_LIBRARIES})
```

### 8.2 단위 테스트 설정
```cmake
# Google Test 추가
find_package(GTest REQUIRED)

# 테스트 실행파일
add_executable(step7_tests
    tests/step7/SignalManagerTest.cpp
    tests/step7/TimerManagerTest.cpp
    tests/step7/LoopDetectorTest.cpp
    tests/step7/CircuitSimulatorTest.cpp
    tests/step7/PerformanceTest.cpp
    ${STEP7_SOURCES}
)

target_link_libraries(step7_tests PRIVATE 
    GTest::gtest_main
    ${COMMON_LIBRARIES}
)

# CTest 통합
include(GoogleTest)
gtest_discover_tests(step7_tests)

# 성능 벤치마크 테스트
add_test(NAME performance_benchmark
         COMMAND notgate3_benchmark --gates=100000 --time=60)
```

### 8.3 CI/CD 파이프라인 (.github/workflows/step7.yml)
```yaml
name: Step 7 - Circuit Simulation Engine

on:
  push:
    paths:
      - 'src/simulation/**'
      - 'tests/step7/**'
      - 'docs/STEP7_*.md'

jobs:
  test:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-latest, windows-latest, macos-latest]
        build_type: [Debug, Release]

    steps:
    - uses: actions/checkout@v3

    - name: Setup dependencies
      run: |
        # SDL2, OpenGL, ImGui 설치
        sudo apt-get update && sudo apt-get install -y libsdl2-dev

    - name: Build
      run: |
        cmake -B build -DCMAKE_BUILD_TYPE=${{ matrix.build_type }}
        cmake --build build --config ${{ matrix.build_type }}

    - name: Run Unit Tests
      run: |
        cd build
        ctest --config ${{ matrix.build_type }} --output-on-failure

    - name: Run Performance Benchmark
      if: matrix.build_type == 'Release'
      run: |
        ./build/notgate3_benchmark --gates=50000 --time=30
        
  memory-check:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v3
    
    - name: Install Valgrind
      run: sudo apt-get install -y valgrind
      
    - name: Memory Leak Check
      run: |
        cmake -B build -DCMAKE_BUILD_TYPE=Debug
        cmake --build build
        valgrind --leak-check=full --error-exitcode=1 ./build/step7_tests
```

## 9. 성능 최적화 전략

### 9.1 컴파일 타임 최적화
```cpp
// 템플릿 메타프로그래밍으로 컴파일 타임 상수 최적화
template<size_t MaxGates>
class OptimizedCircuitSimulator {
    static_assert(MaxGates > 0 && MaxGates <= 1000000, 
                 "MaxGates must be between 1 and 1,000,000");
    
    static constexpr size_t SIGNAL_WORDS = (MaxGates * 4 + 31) / 32;
    static constexpr size_t CACHE_LINE_SIZE = 64;
    
    alignas(CACHE_LINE_SIZE) uint32_t signalBits[SIGNAL_WORDS];
    alignas(CACHE_LINE_SIZE) GateData gates[MaxGates];

public:
    void updateSignalsSIMD() {
        // 컴파일 타임에 최적화된 루프 언롤링
        constexpr size_t UNROLL_FACTOR = 4;
        constexpr size_t UNROLLED_COUNT = SIGNAL_WORDS / UNROLL_FACTOR;
        
        for (size_t i = 0; i < UNROLLED_COUNT; ++i) {
            processSignalWord(i * UNROLL_FACTOR + 0);
            processSignalWord(i * UNROLL_FACTOR + 1);
            processSignalWord(i * UNROLL_FACTOR + 2);
            processSignalWord(i * UNROLL_FACTOR + 3);
        }
        
        // 남은 워드들 처리
        for (size_t i = UNROLLED_COUNT * UNROLL_FACTOR; i < SIGNAL_WORDS; ++i) {
            processSignalWord(i);
        }
    }
};
```

### 9.2 런타임 최적화
```cpp
class RuntimeOptimizer {
public:
    struct OptimizationHints {
        bool useMultithreading = false;
        bool enableSIMD = true;
        bool useMemoryPools = true;
        bool enableSpatialPartitioning = true;
        size_t maxUpdateDepth = 100;
    };
    
    void optimizeForCurrentHardware(OptimizationHints& hints) {
        // CPU 기능 감지
        if (supportsSIMD()) {
            hints.enableSIMD = true;
        }
        
        // 코어 수에 따른 멀티스레딩 결정
        size_t coreCount = std::thread::hardware_concurrency();
        if (coreCount >= 4) {
            hints.useMultithreading = true;
        }
        
        // 메모리 크기에 따른 풀 크기 조정
        size_t availableMemory = getAvailableMemory();
        if (availableMemory < 1024 * 1024 * 1024) { // 1GB 미만
            hints.useMemoryPools = true;
        }
    }

private:
    bool supportsSIMD() {
        // CPUID 명령어로 AVX2 지원 확인
        #ifdef _WIN32
        int cpuInfo[4];
        __cpuid(cpuInfo, 7);
        return (cpuInfo[1] & (1 << 5)) != 0; // AVX2 비트
        #else
        return false; // 간단히 false 반환 (실제로는 더 복잡한 검사 필요)
        #endif
    }
};
```

## 10. 검증 및 테스트 계획

### 10.1 단위 테스트 예시
```cpp
#include <gtest/gtest.h>
#include "SignalManager.h"

class SignalManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        signalManager = std::make_unique<SignalManager>();
    }
    
    std::unique_ptr<SignalManager> signalManager;
};

TEST_F(SignalManagerTest, BasicSignalOperations) {
    // 초기 상태 테스트
    EXPECT_FALSE(signalManager->getSignal(0));
    EXPECT_FALSE(signalManager->getSignal(100));
    
    // 신호 설정 테스트
    signalManager->setSignal(0, true);
    EXPECT_TRUE(signalManager->getSignal(0));
    
    signalManager->setSignal(0, false);
    EXPECT_FALSE(signalManager->getSignal(0));
}

TEST_F(SignalManagerTest, BatchOperations) {
    std::vector<std::pair<uint32_t, bool>> changes = {
        {0, true}, {1, true}, {2, false}, {100, true}
    };
    
    signalManager->setMultipleSignals(changes);
    
    EXPECT_TRUE(signalManager->getSignal(0));
    EXPECT_TRUE(signalManager->getSignal(1));
    EXPECT_FALSE(signalManager->getSignal(2));
    EXPECT_TRUE(signalManager->getSignal(100));
}

TEST_F(SignalManagerTest, PerformanceTest) {
    const size_t SIGNAL_COUNT = 100000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 모든 신호를 true로 설정
    for (size_t i = 0; i < SIGNAL_COUNT; ++i) {
        signalManager->setSignal(static_cast<uint32_t>(i), true);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // 100,000개 신호 설정이 1ms 내에 완료되어야 함
    EXPECT_LT(duration.count(), 1000);
}
```

### 10.2 통합 테스트
```cpp
class CircuitSimulatorIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 테스트용 간단한 회로 생성
        circuit = createTestCircuit();
        simulator = std::make_unique<CircuitSimulator>(circuit.get());
        simulator->initialize();
    }
    
    std::unique_ptr<Circuit> createTestCircuit() {
        auto circuit = std::make_unique<Circuit>();
        
        // 3개의 NOT 게이트를 연쇄 연결
        circuit->addGate(GateType::NOT, Vec2{0, 0});
        circuit->addGate(GateType::NOT, Vec2{1, 0});
        circuit->addGate(GateType::NOT, Vec2{2, 0});
        
        circuit->addWire(0, 0, 1, 0); // Gate0.output -> Gate1.input[0]
        circuit->addWire(1, 0, 2, 0); // Gate1.output -> Gate2.input[0]
        
        return circuit;
    }
    
    std::unique_ptr<Circuit> circuit;
    std::unique_ptr<CircuitSimulator> simulator;
};

TEST_F(CircuitSimulatorIntegrationTest, SignalPropagation) {
    // 초기 상태: 모든 신호는 0
    EXPECT_FALSE(simulator->getSignalState(0)); // Gate0 output
    EXPECT_FALSE(simulator->getSignalState(1)); // Gate1 output  
    EXPECT_FALSE(simulator->getSignalState(2)); // Gate2 output
    
    // Gate0에 입력 신호 1 설정
    simulator->setExternalSignal(100, true); // Gate0.input[0]
    simulator->start();
    
    // 0.1초 후: Gate0 출력이 0으로 변경
    simulator->update(0.1f);
    EXPECT_FALSE(simulator->getSignalState(0));
    EXPECT_FALSE(simulator->getSignalState(1)); // 아직 변경 안됨
    
    // 0.2초 후: Gate1 출력이 1로 변경
    simulator->update(0.1f);
    EXPECT_FALSE(simulator->getSignalState(0));
    EXPECT_TRUE(simulator->getSignalState(1));
    EXPECT_FALSE(simulator->getSignalState(2)); // 아직 변경 안됨
    
    // 0.3초 후: Gate2 출력이 0으로 변경
    simulator->update(0.1f);
    EXPECT_FALSE(simulator->getSignalState(0));
    EXPECT_TRUE(simulator->getSignalState(1));
    EXPECT_FALSE(simulator->getSignalState(2));
}
```

### 10.3 스트레스 테스트
```cpp
class StressTest : public ::testing::Test {
protected:
    std::unique_ptr<Circuit> createLargeCircuit(size_t gateCount) {
        auto circuit = std::make_unique<Circuit>();
        
        // 큰 규모의 회로 생성 (체인 형태)
        for (size_t i = 0; i < gateCount; ++i) {
            circuit->addGate(GateType::NOT, Vec2{static_cast<float>(i), 0});
            
            if (i > 0) {
                // 이전 게이트의 출력을 현재 게이트의 입력으로 연결
                circuit->addWire(static_cast<uint32_t>(i-1), 0, 
                               static_cast<uint32_t>(i), 0);
            }
        }
        
        return circuit;
    }
};

TEST_F(StressTest, Performance100KGates) {
    const size_t GATE_COUNT = 100000;
    auto circuit = createLargeCircuit(GATE_COUNT);
    auto simulator = std::make_unique<CircuitSimulator>(circuit.get());
    
    // 초기화 시간 측정
    auto start = std::chrono::high_resolution_clock::now();
    simulator->initialize();
    auto end = std::chrono::high_resolution_clock::now();
    
    auto initTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(initTime.count(), 5000); // 5초 내 초기화
    
    // 시뮬레이션 시작
    simulator->start();
    simulator->setExternalSignal(0, true); // 첫 번째 게이트에 신호 입력
    
    // 60프레임 동안 성능 측정
    std::vector<float> frameTimes;
    frameTimes.reserve(60);
    
    for (int frame = 0; frame < 60; ++frame) {
        start = std::chrono::high_resolution_clock::now();
        simulator->update(1.0f / 60.0f); // 60 FPS
        end = std::chrono::high_resolution_clock::now();
        
        float frameTime = std::chrono::duration_cast<std::chrono::microseconds>
                         (end - start).count() / 1000.0f;
        frameTimes.push_back(frameTime);
    }
    
    // 평균 프레임 시간 계산
    float avgFrameTime = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0f) 
                        / frameTimes.size();
    
    // 16.67ms 이내 (60 FPS 유지)
    EXPECT_LT(avgFrameTime, 16.67f);
    
    std::cout << "Average frame time with " << GATE_COUNT 
              << " gates: " << avgFrameTime << "ms" << std::endl;
}

TEST_F(StressTest, MemoryUsage) {
    const size_t GATE_COUNT = 100000;
    auto circuit = createLargeCircuit(GATE_COUNT);
    auto simulator = std::make_unique<CircuitSimulator>(circuit.get());
    
    // 초기 메모리 사용량
    size_t initialMemory = getCurrentMemoryUsage();
    
    simulator->initialize();
    
    // 시뮬레이션 후 메모리 사용량
    size_t afterSimMemory = getCurrentMemoryUsage();
    
    size_t memoryIncrease = afterSimMemory - initialMemory;
    size_t expectedMaxMemory = GATE_COUNT * 64; // 게이트당 64바이트
    
    EXPECT_LT(memoryIncrease, expectedMaxMemory);
    
    std::cout << "Memory increase for " << GATE_COUNT 
              << " gates: " << memoryIncrease / 1024 / 1024 << "MB" << std::endl;
}
```

이 기술 명세서는 Step 7 구현을 위한 완전한 기술적 가이드라인을 제공합니다. 모든 설계 결정은 성능 요구사항과 기존 시스템과의 통합을 고려하여 이루어졌습니다.