#include "SimulationUI.h"
#include <imgui.h>
#include <algorithm>

namespace simulation {

    SimulationUI::SimulationUI(CircuitSimulator* simulator)
        : simulator(simulator) {
        
        frameTimeHistory.resize(MAX_HISTORY_SIZE, 0.0f);
        simulationTimeHistory.resize(MAX_HISTORY_SIZE, 0.0f);
    }

    void SimulationUI::render() {
        if (!simulator) return;

        // 메시지 업데이트
        updateMessages(ImGui::GetIO().DeltaTime);
        updateHistories();

        // 각 패널 렌더링
        if (showControlPanelFlag) {
            renderControlPanel();
        }
        
        if (showPerformancePanelFlag) {
            renderPerformancePanel();
        }
        
        if (showDebugPanelFlag) {
            renderDebugPanel();
        }
        
        if (showStatusBarFlag) {
            renderStatusBar();
        }
        
        // 메시지 렌더링
        renderMessages();
    }

    void SimulationUI::renderControlPanel() {
        if (ImGui::Begin("Simulation Control", &showControlPanelFlag)) {
            // 재생 컨트롤 버튼들
            ImGui::Text("Controls:");
            
            // 재생/일시정지 버튼
            bool isRunning = simulator->isRunning();
            bool isPaused = simulator->isPaused();
            
            pushButtonStyle(currentState == SimulationState::STOPPED || isPaused);
            if (ImGui::Button(isRunning ? "Pause" : "Play")) {
                if (isRunning) {
                    simulator->pause();
                } else {
                    simulator->start();
                }
            }
            popButtonStyle();
            
            ImGui::SameLine();
            pushButtonStyle(currentState != SimulationState::STOPPED);
            if (ImGui::Button("Stop")) {
                simulator->stop();
            }
            popButtonStyle();
            
            ImGui::SameLine();
            pushButtonStyle(currentState != SimulationState::STOPPED);
            if (ImGui::Button("Reset")) {
                simulator->reset();
            }
            popButtonStyle();

            ImGui::Separator();

            // 속도 조절
            ImGui::Text("Speed Control:");
            float speed = simulator->getSpeed();
            if (ImGui::SliderFloat("Speed", &speed, 0.1f, 10.0f, "%.1fx")) {
                simulator->setSpeed(speed);
                currentSpeed = speed;
            }

            // 빠른 속도 버튼들
            ImGui::Text("Quick Speed:");
            if (ImGui::SmallButton("0.1x")) { simulator->setSpeed(0.1f); currentSpeed = 0.1f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("0.5x")) { simulator->setSpeed(0.5f); currentSpeed = 0.5f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("1x")) { simulator->setSpeed(1.0f); currentSpeed = 1.0f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("2x")) { simulator->setSpeed(2.0f); currentSpeed = 2.0f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("5x")) { simulator->setSpeed(5.0f); currentSpeed = 5.0f; }
            ImGui::SameLine();
            if (ImGui::SmallButton("10x")) { simulator->setSpeed(10.0f); currentSpeed = 10.0f; }

            ImGui::Separator();

            // 시뮬레이션 정보
            ImGui::Text("Status: %s", getSimulationStateText());
            ImGui::Text("Speed: %s", getSpeedText(currentSpeed));
        }
        ImGui::End();
    }

    void SimulationUI::renderPerformancePanel() {
        if (ImGui::Begin("Performance", &showPerformancePanelFlag)) {
            PerformanceStats stats = simulator->getPerformanceStats();

            // 기본 성능 정보
            ImGui::Text("FPS: %.1f", 1000.0f / std::max(0.001f, stats.frameTime));
            ImGui::Text("Frame Time: %.2f ms", stats.frameTime);
            ImGui::Text("Simulation Time: %.2f ms", stats.simulationTime);
            ImGui::Text("Active Gates: %zu", stats.activeGates);
            ImGui::Text("Signal Changes: %zu", stats.signalChanges);
            ImGui::Text("Memory Usage: %.1f MB", stats.memoryUsage / 1024.0f / 1024.0f);

            // 최적화 레벨 표시
            const char* levelNames[] = { "Ultra High", "High", "Medium", "Low", "Emergency" };
            int levelIndex = std::clamp(stats.optimizationLevel, 0, 4);
            ImGui::Text("Optimization: %s", levelNames[levelIndex]);

            ImGui::Separator();

            // 성능 그래프
            if (!frameTimeHistory.empty()) {
                ImGui::Text("Frame Time History:");
                
                // std::deque를 std::vector로 변환 (data() 함수를 위해)
                std::vector<float> frameTimeVec(frameTimeHistory.begin(), frameTimeHistory.end());
                ImGui::PlotLines("##FrameTime", 
                    frameTimeVec.data(), 
                    static_cast<int>(frameTimeVec.size()), 
                    0, 
                    nullptr, 
                    0.0f, 
                    33.33f,  // 30 FPS 라인
                    ImVec2(0, 80));
            }

            if (!simulationTimeHistory.empty()) {
                ImGui::Text("Simulation Time History:");
                
                // std::deque를 std::vector로 변환 (data() 함수를 위해)
                std::vector<float> simTimeVec(simulationTimeHistory.begin(), simulationTimeHistory.end());
                ImGui::PlotLines("##SimTime", 
                    simTimeVec.data(), 
                    static_cast<int>(simTimeVec.size()), 
                    0, 
                    nullptr, 
                    0.0f, 
                    16.67f,  // 60 FPS budget
                    ImVec2(0, 80));
            }
        }
        ImGui::End();
    }

    void SimulationUI::renderDebugPanel() {
        if (ImGui::Begin("Debug", &showDebugPanelFlag)) {
            // 루프 감지
            ImGui::Text("Loop Detection:");
            if (ImGui::Button("Detect Loops")) {
                bool hasLoops = simulator->detectLoops();
                if (hasLoops) {
                    auto loops = simulator->getDetectedLoops();
                    detectedLoops.clear();
                    for (const auto& loop : loops) {
                        detectedLoops.push_back(loop.gateIds);
                    }
                    showLoopWarning = true;
                }
            }

            if (!detectedLoops.empty()) {
                ImGui::Text("Detected %zu loop(s)", detectedLoops.size());
                for (size_t i = 0; i < detectedLoops.size(); ++i) {
                    ImGui::Text("Loop %zu: %zu gates", i + 1, detectedLoops[i].size());
                }
            }

            ImGui::Separator();

            // 신호 추적
            ImGui::Text("Signal Tracing:");
            ImGui::InputScalar("Signal ID", ImGuiDataType_U32, &traceSignalId);

            if (ImGui::Button("Start Trace")) {
                isTracing = true;
                addMessage("Signal tracing started for ID " + std::to_string(traceSignalId), 0);
            }

            ImGui::SameLine();
            if (ImGui::Button("Stop Trace")) {
                isTracing = false;
                addMessage("Signal tracing stopped", 0);
            }

            if (isTracing) {
                ImGui::Text("Tracing signal %u", traceSignalId);
                bool signalState = simulator->getSignalState(traceSignalId);
                ImGui::Text("Current state: %s", signalState ? "HIGH" : "LOW");
            }

            ImGui::Separator();

            // 활성 게이트 정보
            auto activeGates = simulator->getActiveGates();
            ImGui::Text("Active Gates: %zu", activeGates.size());
            
            if (ImGui::TreeNode("Active Gate List")) {
                for (GateId gateId : activeGates) {
                    GateState state = simulator->getGateState(gateId);
                    const char* stateText = "";
                    switch (state) {
                        case GateState::IDLE: stateText = "IDLE"; break;
                        case GateState::PROCESSING: stateText = "PROCESSING"; break;
                        case GateState::ACTIVE: stateText = "ACTIVE"; break;
                        case GateState::ERROR: stateText = "ERROR"; break;
                    }
                    ImGui::Text("Gate %u: %s", gateId, stateText);
                }
                ImGui::TreePop();
            }
        }
        ImGui::End();

        // 루프 경고 팝업
        if (showLoopWarning) {
            ImGui::OpenPopup("Loop Warning");
        }

        if (ImGui::BeginPopupModal("Loop Warning", &showLoopWarning)) {
            ImGui::Text("Signal loops detected in the circuit!");
            ImGui::Text("The circuit may oscillate.");
            ImGui::Text("Detected %zu loop(s).", detectedLoops.size());
            
            if (ImGui::Button("OK")) {
                showLoopWarning = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void SimulationUI::renderStatusBar() {
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        ImGui::SetNextWindowPos(ImVec2(0, displaySize.y - 25));
        ImGui::SetNextWindowSize(ImVec2(displaySize.x, 25));

        if (ImGui::Begin("Status", &showStatusBarFlag,
                        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {

            std::string status = getSimulationStateText();
            ImGui::Text("%s", status.c_str());

            // 성능 정보 (간단하게)
            PerformanceStats stats = simulator->getPerformanceStats();
            ImGui::SameLine(200);
            ImGui::Text("FPS: %.1f", 1000.0f / std::max(0.001f, stats.frameTime));
        }
        ImGui::End();
    }

    void SimulationUI::renderMessages() {
        if (messages.empty()) return;

        const float PADDING = 10.0f;
        const float MESSAGE_HEIGHT = 40.0f;
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;

        for (size_t i = 0; i < messages.size(); ++i) {
            const Message& msg = messages[i];
            
            // 메시지 위치 계산
            float y = PADDING + i * (MESSAGE_HEIGHT + PADDING);
            ImGui::SetNextWindowPos(ImVec2(displaySize.x - 400 - PADDING, y));
            ImGui::SetNextWindowSize(ImVec2(400, MESSAGE_HEIGHT));

            // 메시지 타입에 따른 색상
            ImVec4 bgColor;
            switch (msg.type) {
                case 0: bgColor = ImVec4(0.2f, 0.3f, 0.8f, 0.9f); break; // 정보 - 파란색
                case 1: bgColor = ImVec4(0.8f, 0.5f, 0.2f, 0.9f); break; // 경고 - 주황색
                case 2: bgColor = ImVec4(0.8f, 0.2f, 0.2f, 0.9f); break; // 오류 - 빨간색
                default: bgColor = ImVec4(0.3f, 0.3f, 0.3f, 0.9f); break;
            }

            ImGui::PushStyleColor(ImGuiCol_WindowBg, bgColor);
            
            std::string windowName = "Message##" + std::to_string(i);
            if (ImGui::Begin(windowName.c_str(), nullptr,
                            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoScrollbar)) {
                
                ImGui::TextWrapped("%s", msg.text.c_str());
            }
            ImGui::End();
            ImGui::PopStyleColor();
        }
    }

    // ISimulationObserver 구현
    void SimulationUI::onSignalChanged(uint32_t signalId, bool newValue) {
        if (isTracing && signalId == traceSignalId) {
            std::string msg = "Signal " + std::to_string(signalId) + " changed to " + 
                            (newValue ? "HIGH" : "LOW");
            addMessage(msg, 0, 1.0f);
        }
    }

    void SimulationUI::onGateStateChanged(uint32_t gateId, GateState newState) {
        // 필요시 게이트 상태 변경 로깅
    }

    void SimulationUI::onLoopDetected(const std::vector<uint32_t>& loopGates) {
        detectedLoops.push_back(loopGates);
        showLoopWarning = true;
        addMessage("Loop detected with " + std::to_string(loopGates.size()) + " gates", 1, 5.0f);
    }

    void SimulationUI::onSimulationStateChanged(SimulationState newState) {
        currentState = newState;
        
        std::string stateText = getSimulationStateText();
        addMessage("Simulation " + stateText, 0, 2.0f);
    }

    void SimulationUI::onPerformanceWarning(const std::string& message) {
        addMessage("Performance: " + message, 1, 5.0f);
    }

    // 유틸리티 메서드들
    void SimulationUI::addMessage(const std::string& text, int type, float duration) {
        messages.push_back({text, duration, type});
        
        // 메시지가 너무 많으면 오래된 것 제거
        if (messages.size() > 5) {
            messages.pop_front();
        }
    }

    void SimulationUI::updateMessages(float deltaTime) {
        for (auto it = messages.begin(); it != messages.end();) {
            it->timeRemaining -= deltaTime;
            if (it->timeRemaining <= 0.0f) {
                it = messages.erase(it);
            } else {
                ++it;
            }
        }
    }

    void SimulationUI::updateHistories() {
        PerformanceStats stats = simulator->getPerformanceStats();
        
        frameTimeHistory.push_back(stats.frameTime);
        if (frameTimeHistory.size() > MAX_HISTORY_SIZE) {
            frameTimeHistory.pop_front();
        }
        
        simulationTimeHistory.push_back(stats.simulationTime);
        if (simulationTimeHistory.size() > MAX_HISTORY_SIZE) {
            simulationTimeHistory.pop_front();
        }
    }

    const char* SimulationUI::getSimulationStateText() const {
        switch (currentState) {
            case SimulationState::RUNNING: return "Running";
            case SimulationState::PAUSED: return "Paused";
            case SimulationState::STOPPED: return "Stopped";
            default: return "Unknown";
        }
    }

    const char* SimulationUI::getSpeedText(float speed) const {
        static char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.1fx", speed);
        return buffer;
    }

    void SimulationUI::pushButtonStyle(bool enabled) {
        if (!enabled) {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.6f);
        }
    }

    void SimulationUI::popButtonStyle() {
        ImGui::PopStyleVar();
    }

} // namespace simulation