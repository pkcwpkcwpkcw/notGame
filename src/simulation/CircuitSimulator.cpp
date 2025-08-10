#include "CircuitSimulator.h"
#include "../core/CellWireManager.h"
#include <algorithm>
#include <cassert>
#include <cmath>

namespace simulation {

    CircuitSimulator::CircuitSimulator(Circuit* circuit, const SimulationConfig& config)
        : circuit(circuit)
        , state(SimulationState::STOPPED)
        , config(config)
        , nextSignalId(0)
        , accumulatedTime(0.0f)
        , needsSignalPropagation(false)
    {
        if (circuit) {
            // 하위 시스템 초기화
            signalManager = std::make_unique<SignalManager>(config.maxSignals);
            timerManager = std::make_unique<TimerManager>();
            loopDetector = std::make_unique<LoopDetector>(circuit);
            perfManager = std::make_unique<PerformanceManager>();

            // 신호 매핑 초기화
            initializeSignalMapping();
        }
    }

    CircuitSimulator::~CircuitSimulator() = default;

    void CircuitSimulator::initialize() {
        if (!circuit) return;

        // 모든 시스템 초기화
        signalManager->clearAllSignals();
        timerManager->reset();
        loopDetector->invalidateCache();
        perfManager->resetStats();

        // 신호 매핑 재구성
        initializeSignalMapping();
        
        // 초기 상태 설정
        updateGateSignals();
        
        state = SimulationState::STOPPED;
        accumulatedTime = 0.0f;
        needsSignalPropagation = false;
        dirtyGates.clear();

        notifySimulationStateChanged(state);
    }

    void CircuitSimulator::start() {
        if (state == SimulationState::STOPPED || state == SimulationState::PAUSED) {
            state = SimulationState::RUNNING;
            notifySimulationStateChanged(state);
        }
    }

    void CircuitSimulator::pause() {
        if (state == SimulationState::RUNNING) {
            state = SimulationState::PAUSED;
            notifySimulationStateChanged(state);
        }
    }

    void CircuitSimulator::stop() {
        if (state != SimulationState::STOPPED) {
            state = SimulationState::STOPPED;
            accumulatedTime = 0.0f;
            
            // 모든 타이머 취소
            timerManager->reset();
            
            // 신호 상태 초기화
            signalManager->clearAllSignals();
            updateGateSignals();
            
            notifySimulationStateChanged(state);
        }
    }

    void CircuitSimulator::reset() {
        stop();
        initialize();
    }

    void CircuitSimulator::update(float deltaTime) {
        if (!circuit || state != SimulationState::RUNNING) return;

        // 회로 변경 감지 (새 게이트 추가 등)
        onCircuitChanged();

        perfManager->beginFrame();

        // 시뮬레이션 속도 적용
        float adjustedDeltaTime = deltaTime * config.simulationSpeed;
        accumulatedTime += adjustedDeltaTime;

        auto simStart = std::chrono::high_resolution_clock::now();

        // 고정 시간 스텝 시뮬레이션
        const float fixedTimeStep = 1.0f / 60.0f; // 60Hz 고정
        while (accumulatedTime >= fixedTimeStep) {
            // 타이머 업데이트
            updateTimers(fixedTimeStep);
            
            // 만료된 타이머 처리
            processExpiredTimers();
            
            // 신호 전파
            if (needsSignalPropagation) {
                propagateSignals();
                needsSignalPropagation = false;
            }
            
            // 입력 변경 감지
            detectInputChanges();
            
            accumulatedTime -= fixedTimeStep;
        }

        auto simEnd = std::chrono::high_resolution_clock::now();
        auto simDuration = std::chrono::duration_cast<std::chrono::microseconds>(simEnd - simStart);
        perfManager->recordSimulationTime(simDuration.count() / 1000.0f);

        // 성능 최적화
        optimizePerformance();
        
        perfManager->endFrame();
    }

    bool CircuitSimulator::getSignalState(uint32_t signalId) const {
        return signalManager->getSignal(signalId);
    }

    GateState CircuitSimulator::getGateState(GateId gateId) const {
        if (!circuit) return GateState::ERROR;

        const Gate* gate = circuit->getGate(gateId);
        if (!gate) return GateState::ERROR;

        // 타이머가 활성화되어 있으면 PROCESSING
        if (timerManager->hasActiveTimer(gateId)) {
            return GateState::PROCESSING;
        }

        // 출력 신호가 활성화되어 있으면 ACTIVE
        auto outputIt = gateOutputSignals.find(gateId);
        if (outputIt != gateOutputSignals.end()) {
            if (signalManager->getSignal(outputIt->second)) {
                return GateState::ACTIVE;
            }
        }

        return GateState::IDLE;
    }

    void CircuitSimulator::setExternalSignal(uint32_t signalId, bool value) {
        if (signalManager) {
            signalManager->setSignal(signalId, value);
            needsSignalPropagation = true;
        }
    }

    void CircuitSimulator::setSpeed(float speed) {
        config.simulationSpeed = std::max(0.1f, std::min(10.0f, speed));
    }

    bool CircuitSimulator::detectLoops() {
        if (config.enableLoopDetection && loopDetector) {
            bool hasLoops = loopDetector->detectLoops();
            
            if (hasLoops) {
                std::vector<LoopInfo> loops = loopDetector->getAllLoops();
                for (const auto& loop : loops) {
                    notifyLoopDetected(loop.gateIds);
                }
            }
            
            return hasLoops;
        }
        
        return false;
    }

    std::vector<LoopInfo> CircuitSimulator::getDetectedLoops() const {
        if (loopDetector) {
            return loopDetector->getAllLoops();
        }
        return {};
    }

    std::vector<GateId> CircuitSimulator::getActiveGates() const {
        std::vector<GateId> activeGates;
        
        if (circuit) {
            for (auto it = circuit->gatesBegin(); it != circuit->gatesEnd(); ++it) {
                GateId gateId = it->first;
                if (timerManager->hasActiveTimer(gateId)) {
                    activeGates.push_back(gateId);
                }
            }
        }
        
        return activeGates;
    }

    PerformanceStats CircuitSimulator::getPerformanceStats() const {
        if (perfManager) {
            return perfManager->getStats();
        }
        return {};
    }

    void CircuitSimulator::addObserver(ISimulationObserver* observer) {
        if (observer) {
            auto it = std::find(observers.begin(), observers.end(), observer);
            if (it == observers.end()) {
                observers.push_back(observer);
            }
        }
    }

    void CircuitSimulator::removeObserver(ISimulationObserver* observer) {
        auto it = std::find(observers.begin(), observers.end(), observer);
        if (it != observers.end()) {
            observers.erase(it);
        }
    }

    void CircuitSimulator::onCircuitChanged() {
        // 회로가 변경되었을 때 캐시 무효화
        if (loopDetector) {
            loopDetector->invalidateCache();
        }
        
        // 신호 매핑 재구성
        initializeSignalMapping();
        
        // 새로 추가된 게이트들 초기 처리
        if (circuit) {
            for (auto it = circuit->gatesBegin(); it != circuit->gatesEnd(); ++it) {
                GateId gateId = it->first;
                Gate* gate = &it->second;
                
                // 게이트 출력 신호가 없으면 생성하고 초기화
                auto outputIt = gateOutputSignals.find(gateId);
                if (outputIt == gateOutputSignals.end()) {
                    uint32_t signalId = getOrCreateSignal(gateId, -1);
                    if (signalId != INVALID_SIGNAL) {
                        // NOT 게이트는 기본적으로 HIGH 출력
                        bool initialOutput = (gate->currentOutput == SignalState::HIGH);
                        signalManager->setSignal(signalId, initialOutput);
                        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, 
                                    "[CircuitSimulator] New gate %u initialized with output %s", 
                                    gateId, initialOutput ? "HIGH" : "LOW");
                    }
                }
            }
        }
        
        // 신호 전파 필요 표시
        needsSignalPropagation = true;
    }

    void CircuitSimulator::updateTimers(float deltaTime) {
        if (timerManager) {
            timerManager->updateTimers(deltaTime);
        }
    }

    void CircuitSimulator::processExpiredTimers() {
        if (!timerManager) return;

        auto expiredTimers = timerManager->getExpiredTimers();
        
        for (const auto& [gateId, pendingOutput] : expiredTimers) {
            // Gate 객체의 currentOutput 업데이트
            if (circuit) {
                Gate* gate = circuit->getGate(gateId);
                if (gate) {
                    gate->currentOutput = pendingOutput ? SignalState::HIGH : SignalState::LOW;
                    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, 
                                "[CircuitSimulator] Gate %u output changed to %s", 
                                gateId, pendingOutput ? "HIGH" : "LOW");
                }
            }
            
            // 게이트의 출력 신호 업데이트
            auto outputIt = gateOutputSignals.find(gateId);
            if (outputIt != gateOutputSignals.end()) {
                uint32_t signalId = outputIt->second;
                bool currentValue = signalManager->getSignal(signalId);
                
                if (currentValue != pendingOutput) {
                    signalManager->setSignal(signalId, pendingOutput);
                    notifySignalChanged(signalId, pendingOutput);
                    notifyGateStateChanged(gateId, pendingOutput ? GateState::ACTIVE : GateState::IDLE);
                    needsSignalPropagation = true;
                }
            }
        }
        
        timerManager->clearExpiredTimers();
    }

    void CircuitSimulator::propagateSignals() {
        if (!signalManager) return;

        // 변경된 신호들을 전파
        signalManager->propagateSignalsSIMD();
        
        auto changedSignals = signalManager->getChangedSignals();
        
        // 변경된 신호에 연결된 게이트들 처리
        for (uint32_t signalId : changedSignals) {
            // 이 신호를 입력으로 받는 게이트들 찾기
            for (const auto& pair : gateInputSignals) {
                const auto& gatePortPair = pair.first;
                uint32_t inputSignalId = pair.second;
                if (inputSignalId == signalId) {
                    GateId gateId = gatePortPair.first;
                    if (std::find(dirtyGates.begin(), dirtyGates.end(), gateId) == dirtyGates.end()) {
                        dirtyGates.push_back(gateId);
                    }
                }
            }
        }
        
        // 더티 게이트들 처리
        for (GateId gateId : dirtyGates) {
            processGate(gateId);
        }
        
        dirtyGates.clear();
        signalManager->clearChangedSignals();
    }

    void CircuitSimulator::detectInputChanges() {
        // 현재는 신호 전파에서 처리되므로 별도 구현 불필요
    }

    void CircuitSimulator::optimizePerformance() {
        if (perfManager) {
            perfManager->updateOptimization();
            
            // 성능 경고 확인
            PerformanceStats stats = perfManager->getStats();
            
            if (stats.frameTime > 33.33f) { // 30 FPS 이하
                notifyPerformanceWarning("Performance degraded: " + 
                    std::to_string(1000.0f / stats.frameTime) + " FPS");
            }
        }
    }

    void CircuitSimulator::initializeSignalMapping() {
        if (!circuit) return;

        gateOutputSignals.clear();
        gateInputSignals.clear();
        nextSignalId = 0;

        // 모든 게이트에 대해 신호 ID 할당
        for (auto it = circuit->gatesBegin(); it != circuit->gatesEnd(); ++it) {
            GateId gateId = it->first;
            Gate* gate = &it->second;
            
            // 출력 신호 할당
            uint32_t outputSignalId = nextSignalId++;
            gateOutputSignals[gateId] = outputSignalId;
            
            // 게이트의 현재 출력 상태를 신호 매니저에 반영
            if (signalManager) {
                bool initialOutput = (gate->currentOutput == SignalState::HIGH);
                signalManager->setSignal(outputSignalId, initialOutput);
                SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                           "[CircuitSimulator] Gate %u mapped with initial output %s",
                           gateId, initialOutput ? "HIGH" : "LOW");
            }
            
            // 입력 신호 할당 (3개의 입력 포트)
            for (PortIndex port = 0; port < 3; ++port) {
                gateInputSignals[{gateId, port}] = nextSignalId++;
            }
        }
    }

    uint32_t CircuitSimulator::getOrCreateSignal(GateId gateId, PortIndex portIndex) {
        if (portIndex == -1) {
            // 출력 신호
            auto it = gateOutputSignals.find(gateId);
            if (it != gateOutputSignals.end()) {
                return it->second;
            }
        } else {
            // 입력 신호
            auto it = gateInputSignals.find({gateId, portIndex});
            if (it != gateInputSignals.end()) {
                return it->second;
            }
        }
        
        return INVALID_SIGNAL;
    }

    void CircuitSimulator::updateGateSignals() {
        if (!circuit) return;

        // 모든 게이트의 초기 신호 상태 설정
        for (auto it = circuit->gatesBegin(); it != circuit->gatesEnd(); ++it) {
            GateId gateId = it->first;
            const Gate* gate = &it->second;
            
            // 출력 신호 설정 (초기값은 LOW)
            uint32_t outputSignalId = getOrCreateSignal(gateId, -1);
            if (outputSignalId != INVALID_SIGNAL) {
                signalManager->setSignal(outputSignalId, gate->currentOutput == SignalState::HIGH);
            }
            
            // 입력 신호는 와이어 연결에 따라 설정됨
        }
    }

    bool CircuitSimulator::calculateNOTGateOutput(const Gate* gate) {
        if (!gate || gate->type != GateType::NOT) return false;

        // CellWireManager에서 게이트 입력 포트의 신호 확인
        if (cellWireManager) {
            glm::ivec2 gatePos(std::floor(gate->position.x), std::floor(gate->position.y));
            
            // 3개 입력 포트 확인 (왼쪽에 위치)
            for (int port = 0; port < 3; ++port) {
                glm::ivec2 inputPos = gatePos;
                inputPos.x -= 1;  // 왼쪽
                inputPos.y += port - 1;  // 위(-1), 중간(0), 아래(+1)
                
                const CellWire* wire = cellWireManager->getWireAt(inputPos);
                if (wire && wire->hasSignal) {
                    // 하나라도 HIGH 신호가 있으면 출력은 LOW
                    return false;
                }
            }
        }
        
        // 모든 입력이 LOW이면 출력은 HIGH
        return true;
    }

    void CircuitSimulator::processGate(GateId gateId) {
        if (!circuit) return;

        Gate* gate = circuit->getGate(gateId);
        if (!gate) return;

        // 새로운 출력 계산
        bool newOutput = calculateNOTGateOutput(gate);
        
        // 현재 출력과 비교
        uint32_t outputSignalId = getOrCreateSignal(gateId, -1);
        if (outputSignalId != INVALID_SIGNAL) {
            bool currentOutput = signalManager->getSignal(outputSignalId);
            
            if (currentOutput != newOutput) {
                // 출력이 변경되면 딜레이 타이머 시작
                timerManager->scheduleTimer(gateId, config.gateDelay, newOutput);
                notifyGateStateChanged(gateId, GateState::PROCESSING);
                
                SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                           "[CircuitSimulator] Gate %u processing: %s -> %s",
                           gateId, currentOutput ? "HIGH" : "LOW", newOutput ? "HIGH" : "LOW");
            }
        }
    }

    // Observer 알림 메서드들
    void CircuitSimulator::notifySignalChanged(uint32_t signalId, bool newValue) {
        for (auto* observer : observers) {
            observer->onSignalChanged(signalId, newValue);
        }
    }

    void CircuitSimulator::notifyGateStateChanged(GateId gateId, GateState newState) {
        for (auto* observer : observers) {
            observer->onGateStateChanged(gateId, newState);
        }
    }

    void CircuitSimulator::notifyLoopDetected(const std::vector<GateId>& loopGates) {
        for (auto* observer : observers) {
            observer->onLoopDetected(loopGates);
        }
    }

    void CircuitSimulator::notifySimulationStateChanged(SimulationState newState) {
        for (auto* observer : observers) {
            observer->onSimulationStateChanged(newState);
        }
    }

    void CircuitSimulator::notifyPerformanceWarning(const std::string& message) {
        for (auto* observer : observers) {
            observer->onPerformanceWarning(message);
        }
    }

} // namespace simulation