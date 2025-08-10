#pragma once

#include "CircuitSimulator.h"
#include "ISimulationObserver.h"
#include <string>
#include <deque>

namespace simulation {

    // Dear ImGui를 사용한 시뮬레이션 제어 UI
    class SimulationUI : public ISimulationObserver {
    public:
        explicit SimulationUI(CircuitSimulator* simulator);
        ~SimulationUI() = default;

        // UI 렌더링
        void render();
        
        // 패널 표시/숨기기 제어
        void showControlPanel(bool show = true) { showControlPanelFlag = show; }
        void showPerformancePanel(bool show = true) { showPerformancePanelFlag = show; }
        void showDebugPanel(bool show = true) { showDebugPanelFlag = show; }
        void showStatusBar(bool show = true) { showStatusBarFlag = show; }

        // ISimulationObserver 인터페이스 구현
        void onSignalChanged(uint32_t signalId, bool newValue) override;
        void onGateStateChanged(uint32_t gateId, GateState newState) override;
        void onLoopDetected(const std::vector<uint32_t>& loopGates) override;
        void onSimulationStateChanged(SimulationState newState) override;
        void onPerformanceWarning(const std::string& message) override;

    private:
        CircuitSimulator* simulator;
        
        // UI 상태
        bool showControlPanelFlag = true;
        bool showPerformancePanelFlag = true;
        bool showDebugPanelFlag = false;
        bool showStatusBarFlag = true;
        
        // 시뮬레이션 상태
        SimulationState currentState = SimulationState::STOPPED;
        float currentSpeed = 1.0f;
        
        // 성능 히스토리 (그래프용)
        std::deque<float> frameTimeHistory;
        std::deque<float> simulationTimeHistory;
        static constexpr size_t MAX_HISTORY_SIZE = 100;
        
        // 메시지 시스템
        struct Message {
            std::string text;
            float timeRemaining;
            int type; // 0=info, 1=warning, 2=error
        };
        std::deque<Message> messages;
        
        // 신호 추적
        uint32_t traceSignalId = 0;
        bool isTracing = false;
        
        // 루프 정보
        std::vector<std::vector<uint32_t>> detectedLoops;
        bool showLoopWarning = false;

        // UI 렌더링 메서드
        void renderControlPanel();
        void renderPerformancePanel();
        void renderDebugPanel();
        void renderStatusBar();
        void renderMessages();
        
        // 유틸리티 메서드
        void addMessage(const std::string& text, int type = 0, float duration = 3.0f);
        void updateMessages(float deltaTime);
        void updateHistories();
        
        // 컨트롤 유틸리티
        const char* getSimulationStateText() const;
        const char* getSpeedText(float speed) const;
        
        // ImGui 스타일링
        void pushButtonStyle(bool enabled);
        void popButtonStyle();
    };

} // namespace simulation