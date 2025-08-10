#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace simulation {

    enum class GateState {
        IDLE,        // 대기 상태
        PROCESSING,  // 딜레이 처리 중
        ACTIVE,      // 활성 (출력 신호 1)
        ERROR        // 오류 상태
    };

    enum class SimulationState {
        STOPPED,
        RUNNING,
        PAUSED
    };

    class ISimulationObserver {
    public:
        virtual ~ISimulationObserver() = default;
        
        virtual void onSignalChanged(uint32_t signalId, bool newValue) = 0;
        virtual void onGateStateChanged(uint32_t gateId, GateState newState) = 0;
        virtual void onLoopDetected(const std::vector<uint32_t>& loopGates) = 0;
        virtual void onSimulationStateChanged(SimulationState newState) = 0;
        virtual void onPerformanceWarning(const std::string& message) = 0;
    };

} // namespace simulation