#include "SimulationRenderer.h"
#include "SimulationTypes.h"

namespace simulation {

    SimulationRenderer::SimulationRenderer(Renderer* renderer)
        : renderer(renderer) {
    }

    void SimulationRenderer::onSignalChanged(uint32_t signalId, bool newValue) {
        if (!renderer) return;

        // 신호 상태 시각적 업데이트
        updateSignalVisualState(signalId, newValue);

        // 신호 전파 애니메이션 트리거 (활성화된 경우)
        if (newValue && animationsEnabled) {
            triggerSignalAnimation(signalId);
        }

        // 연결된 게이트의 시각적 상태도 업데이트
        GateId gateId = getGateFromSignal(signalId);
        if (gateId != Constants::INVALID_GATE_ID) {
            // 게이트 상태는 별도 이벤트에서 처리되므로 여기서는 신호만 처리
        }
    }

    void SimulationRenderer::onGateStateChanged(uint32_t gateId, GateState newState) {
        if (!renderer) return;

        // 게이트 시각적 상태 업데이트
        updateGateVisualState(gateId, newState);

        // 딜레이 타이머 진행률 표시 (PROCESSING 상태인 경우)
        if (newState == GateState::PROCESSING) {
            // TODO: 타이머 진행률을 렌더러에 전달
            // renderer->updateGateTimer(gateId, progress);
        }
    }

    void SimulationRenderer::onLoopDetected(const std::vector<uint32_t>& loopGates) {
        if (!renderer) return;

        // 루프에 포함된 게이트들을 시각적으로 강조
        highlightLoopGates(loopGates);
    }

    void SimulationRenderer::onSimulationStateChanged(SimulationState newState) {
        if (!renderer) return;

        // 시뮬레이션 상태에 따른 시각적 피드백
        switch (newState) {
            case SimulationState::RUNNING:
                // 실행 중 표시
                break;
            case SimulationState::PAUSED:
                // 일시정지 표시
                break;
            case SimulationState::STOPPED:
                // 정지 상태 표시
                break;
        }
    }

    void SimulationRenderer::onPerformanceWarning(const std::string& message) {
        if (!renderer) return;

        // 성능 경고 메시지 표시
        // TODO: UI 시스템을 통한 경고 메시지 표시
        // renderer->showPerformanceWarning(message);
    }

    void SimulationRenderer::setSignalToGateMapping(const std::unordered_map<uint32_t, GateId>& mapping) {
        signalToGate = mapping;
    }

    void SimulationRenderer::setGateToSignalMapping(const std::unordered_map<GateId, uint32_t>& mapping) {
        gateToSignal = mapping;
    }

    GateId SimulationRenderer::getGateFromSignal(uint32_t signalId) const {
        auto it = signalToGate.find(signalId);
        return (it != signalToGate.end()) ? it->second : Constants::INVALID_GATE_ID;
    }

    uint32_t SimulationRenderer::getSignalFromGate(GateId gateId) const {
        auto it = gateToSignal.find(gateId);
        return (it != gateToSignal.end()) ? it->second : INVALID_SIGNAL;
    }

    void SimulationRenderer::updateGateVisualState(GateId gateId, GateState state) {
        if (!renderer) return;

        // 게이트 상태에 따른 색상 및 효과 변경
        switch (state) {
            case GateState::IDLE:
                // 기본 상태 - 회색 테두리
                break;
            case GateState::ACTIVE:
                // 활성 상태 - 녹색 테두리 + 출력 신호 표시
                break;
            case GateState::PROCESSING:
                // 딜레이 중 - 주황색 테두리 + 진행률 바
                break;
            case GateState::ERROR:
                // 오류 상태 - 빨간색 테두리 + X 마크
                break;
        }

        // TODO: 실제 렌더링 API 호출
        // renderer->updateGateVisual(gateId, state);
    }

    void SimulationRenderer::updateSignalVisualState(uint32_t signalId, bool active) {
        if (!renderer || !signalGlowEnabled) return;

        // 신호 상태에 따른 와이어 색상 변경
        if (active) {
            // 활성 신호: 와이어가 녹색으로 표시
            // TODO: 신호 ID를 와이어 ID로 변환하여 렌더링 업데이트
            // renderer->setWireColor(wireId, Color::GREEN);
        } else {
            // 비활성 신호: 와이어가 회색으로 표시
            // TODO: renderer->setWireColor(wireId, Color::GRAY);
        }
    }

    void SimulationRenderer::triggerSignalAnimation(uint32_t signalId) {
        if (!renderer || !animationsEnabled) return;

        // 신호 전파 애니메이션 트리거
        // 와이어를 따라 흐르는 점 애니메이션 시작
        // TODO: 실제 애니메이션 시스템과 연동
        // renderer->triggerSignalFlowAnimation(signalId);
    }

    void SimulationRenderer::highlightLoopGates(const std::vector<uint32_t>& gateIds) {
        if (!renderer) return;

        // 루프에 포함된 게이트들을 점선 테두리로 강조 표시
        for (uint32_t gateId : gateIds) {
            // TODO: 실제 하이라이트 렌더링
            // renderer->highlightGate(gateId, HighlightType::LOOP);
        }
    }

} // namespace simulation