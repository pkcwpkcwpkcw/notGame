#pragma once

#include "ISimulationObserver.h"
#include "../render/Renderer.h"
#include "../core/Types.h"
#include <unordered_map>

namespace simulation {

    // 렌더링 시스템과 시뮬레이션의 연동을 담당하는 클래스
    class SimulationRenderer : public ISimulationObserver {
    public:
        explicit SimulationRenderer(Renderer* renderer);
        ~SimulationRenderer() = default;

        // ISimulationObserver 인터페이스 구현
        void onSignalChanged(uint32_t signalId, bool newValue) override;
        void onGateStateChanged(uint32_t gateId, GateState newState) override;
        void onLoopDetected(const std::vector<uint32_t>& loopGates) override;
        void onSimulationStateChanged(SimulationState newState) override;
        void onPerformanceWarning(const std::string& message) override;

        // 신호 ID와 게이트 ID 매핑 설정
        void setSignalToGateMapping(const std::unordered_map<uint32_t, GateId>& mapping);
        void setGateToSignalMapping(const std::unordered_map<GateId, uint32_t>& mapping);

        // 시각적 효과 제어
        void setAnimationsEnabled(bool enabled) { animationsEnabled = enabled; }
        void setSignalGlowEnabled(bool enabled) { signalGlowEnabled = enabled; }

    private:
        Renderer* renderer;
        
        // 매핑 테이블
        std::unordered_map<uint32_t, GateId> signalToGate;
        std::unordered_map<GateId, uint32_t> gateToSignal;
        
        // 시각적 효과 설정
        bool animationsEnabled = true;
        bool signalGlowEnabled = true;
        
        // 내부 메서드
        GateId getGateFromSignal(uint32_t signalId) const;
        uint32_t getSignalFromGate(GateId gateId) const;
        
        // 렌더링 상태 업데이트
        void updateGateVisualState(GateId gateId, GateState state);
        void updateSignalVisualState(uint32_t signalId, bool active);
        void triggerSignalAnimation(uint32_t signalId);
        void highlightLoopGates(const std::vector<uint32_t>& gateIds);
    };

} // namespace simulation