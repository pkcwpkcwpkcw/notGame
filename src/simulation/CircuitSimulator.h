#pragma once

#include "ISimulationObserver.h"
#include "SimulationTypes.h"
#include "SignalManager.h"
#include "TimerManager.h"
#include "LoopDetector.h"
#include "PerformanceManager.h"
#include "../core/Circuit.h"
#include "../core/Types.h"
#include <memory>
#include <vector>
#include <unordered_map>

class CellWireManager;

namespace simulation {

    class CircuitSimulator {
    public:
        explicit CircuitSimulator(Circuit* circuit, const SimulationConfig& config = {});
        ~CircuitSimulator();
        
        // CellWireManager 설정
        void setCellWireManager(CellWireManager* manager) { cellWireManager = manager; }

        // 시뮬레이션 제어
        void initialize();
        void start();
        void pause();
        void stop();
        void reset();
        void update(float deltaTime);

        // 상태 조회
        bool isRunning() const { return state == SimulationState::RUNNING; }
        bool isPaused() const { return state == SimulationState::PAUSED; }
        bool isStopped() const { return state == SimulationState::STOPPED; }
        
        bool getSignalState(uint32_t signalId) const;
        GateState getGateState(GateId gateId) const;
        
        // 외부 신호 제어 (퍼즐 모드용)
        void setExternalSignal(uint32_t signalId, bool value);
        
        // 시뮬레이션 설정
        void setSpeed(float speed);
        float getSpeed() const { return config.simulationSpeed; }
        
        // 디버깅
        bool detectLoops();
        std::vector<LoopInfo> getDetectedLoops() const;
        std::vector<GateId> getActiveGates() const;
        PerformanceStats getPerformanceStats() const;

        // Observer 패턴
        void addObserver(ISimulationObserver* observer);
        void removeObserver(ISimulationObserver* observer);

        // 회로 변경 알림
        void onCircuitChanged();

    private:
        // 멤버 변수
        Circuit* circuit;
        SimulationState state;
        SimulationConfig config;

        // 하위 시스템들
        std::unique_ptr<SignalManager> signalManager;
        std::unique_ptr<TimerManager> timerManager;
        std::unique_ptr<LoopDetector> loopDetector;
        std::unique_ptr<PerformanceManager> perfManager;

        // Observer 목록
        std::vector<ISimulationObserver*> observers;

        // 유틸리티
        struct PairHash {
            std::size_t operator()(const std::pair<GateId, PortIndex>& p) const {
                return std::hash<GateId>()(p.first) ^ (std::hash<PortIndex>()(p.second) << 1);
            }
        };

        // 신호 매핑 (GateId -> SignalId)
        std::unordered_map<GateId, uint32_t> gateOutputSignals;
        std::unordered_map<std::pair<GateId, PortIndex>, uint32_t, PairHash> gateInputSignals;
        uint32_t nextSignalId;

        // 시뮬레이션 상태
        float accumulatedTime;
        bool needsSignalPropagation;
        std::vector<GateId> dirtyGates;

        // 내부 메서드
        void updateTimers(float deltaTime);
        void processExpiredTimers();
        void propagateSignals();
        void detectInputChanges();
        void optimizePerformance();

        // 신호 관리
        void initializeSignalMapping();
        uint32_t getOrCreateSignal(GateId gateId, PortIndex portIndex = -1);
        void updateGateSignals();

        // NOT 게이트 로직
        bool calculateNOTGateOutput(const Gate* gate);
        void processGate(GateId gateId);
        
        // CellWireManager 참조
        CellWireManager* cellWireManager = nullptr;

        // Observer 알림
        void notifySignalChanged(uint32_t signalId, bool newValue);
        void notifyGateStateChanged(GateId gateId, GateState newState);
        void notifyLoopDetected(const std::vector<GateId>& loopGates);
        void notifySimulationStateChanged(SimulationState newState);
        void notifyPerformanceWarning(const std::string& message);
    };

} // namespace simulation