#include "PerformanceManager.h"
#include <algorithm>
#include <numeric>

namespace simulation {

    PerformanceManager::PerformanceManager() 
        : currentLevel(OptimizationLevel::HIGH)
        , targetFrameTime(16.67f)  // 60 FPS
        , frameTime(0.0f)
        , simulationTime(0.0f)
        , lastOptimizationCheck(0.0f)
    {
        initializeDefaults();
    }

    void PerformanceManager::beginFrame() {
        frameStart = std::chrono::high_resolution_clock::now();
    }

    void PerformanceManager::endFrame() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - frameStart);
        frameTime = duration.count() / 1000.0f; // ms 단위로 변환

        // 프레임 히스토리 업데이트
        FrameStats stats;
        stats.frameTime = frameTime;
        stats.simulationTime = simulationTime;
        stats.activeGates = 0;  // 실제 값은 외부에서 설정
        stats.signalChanges = 0;  // 실제 값은 외부에서 설정
        stats.timestamp = now;

        frameHistory.push_back(stats);

        // 오래된 프레임 데이터 정리
        cleanupOldFrames();

        // 주기적으로 최적화 레벨 조정 (0.5초마다)
        float currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count() / 1000.0f;
        
        if (currentTime - lastOptimizationCheck >= 0.5f) {
            updateOptimization();
            lastOptimizationCheck = currentTime;
        }

        // 다음 프레임을 위해 시뮬레이션 시간 초기화
        simulationTime = 0.0f;
    }

    void PerformanceManager::recordSimulationTime(float time) {
        simulationTime += time;
    }

    void PerformanceManager::updateOptimization() {
        OptimizationLevel previousLevel = currentLevel;
        adjustOptimizationLevel();

        if (currentLevel != previousLevel) {
            applyOptimizations();
        }
    }

    PerformanceStats PerformanceManager::getStats() const {
        PerformanceStats stats;
        stats.frameTime = frameTime;
        stats.simulationTime = simulationTime;
        stats.activeGates = frameHistory.empty() ? 0 : frameHistory.back().activeGates;
        stats.signalChanges = frameHistory.empty() ? 0 : frameHistory.back().signalChanges;
        stats.memoryUsage = 0; // 실제 메모리 사용량은 외부에서 설정
        stats.optimizationLevel = static_cast<int>(currentLevel);

        return stats;
    }

    void PerformanceManager::resetStats() {
        frameHistory.clear();
        frameTime = 0.0f;
        simulationTime = 0.0f;
    }

    void PerformanceManager::enableOptimization(const std::string& name, bool enabled) {
        optimizationFlags[name] = enabled;
    }

    bool PerformanceManager::isOptimizationEnabled(const std::string& name) const {
        auto it = optimizationFlags.find(name);
        return (it != optimizationFlags.end()) ? it->second : false;
    }

    void PerformanceManager::setFrameTimeThreshold(OptimizationLevel level, float threshold) {
        frameTimeThresholds[level] = threshold;
    }

    void PerformanceManager::adjustOptimizationLevel() {
        float avgFrameTime = getAverageFrameTime();

        // 성능 저하 감지 - 최적화 레벨 증가
        if (shouldIncreaseOptimization()) {
            if (currentLevel < OptimizationLevel::EMERGENCY) {
                currentLevel = static_cast<OptimizationLevel>(static_cast<int>(currentLevel) + 1);
            }
        }
        // 성능 여유 감지 - 최적화 레벨 감소
        else if (shouldDecreaseOptimization()) {
            if (currentLevel > OptimizationLevel::ULTRA_HIGH) {
                currentLevel = static_cast<OptimizationLevel>(static_cast<int>(currentLevel) - 1);
            }
        }
    }

    void PerformanceManager::applyOptimizations() {
        switch (currentLevel) {
            case OptimizationLevel::ULTRA_HIGH:
                applyUltraHighSettings();
                break;
            case OptimizationLevel::HIGH:
                applyHighSettings();
                break;
            case OptimizationLevel::MEDIUM:
                applyMediumSettings();
                break;
            case OptimizationLevel::LOW:
                applyLowSettings();
                break;
            case OptimizationLevel::EMERGENCY:
                applyEmergencySettings();
                break;
        }
    }

    void PerformanceManager::restoreFeatures() {
        // 기능 복원은 최적화 레벨이 낮아질 때 자동으로 적용됨
        applyOptimizations();
    }

    float PerformanceManager::getAverageFrameTime() const {
        if (frameHistory.empty()) return 0.0f;

        float sum = std::accumulate(frameHistory.begin(), frameHistory.end(), 0.0f,
            [](float acc, const FrameStats& stats) {
                return acc + stats.frameTime;
            });

        return sum / frameHistory.size();
    }

    float PerformanceManager::getAverageSimulationTime() const {
        if (frameHistory.empty()) return 0.0f;

        float sum = std::accumulate(frameHistory.begin(), frameHistory.end(), 0.0f,
            [](float acc, const FrameStats& stats) {
                return acc + stats.simulationTime;
            });

        return sum / frameHistory.size();
    }

    bool PerformanceManager::shouldIncreaseOptimization() const {
        float avgFrameTime = getAverageFrameTime();
        
        // 목표 프레임 시간의 1.5배를 초과하면 최적화 증가
        return avgFrameTime > targetFrameTime * 1.5f;
    }

    bool PerformanceManager::shouldDecreaseOptimization() const {
        float avgFrameTime = getAverageFrameTime();
        
        // 목표 프레임 시간의 0.8배 미만이면 최적화 감소
        return avgFrameTime < targetFrameTime * 0.8f && 
               frameHistory.size() >= MAX_FRAME_HISTORY; // 충분한 샘플이 있을 때만
    }

    void PerformanceManager::initializeDefaults() {
        // 기본 최적화 플래그 설정
        optimizationFlags["particle_effects"] = true;
        optimizationFlags["signal_animations"] = true;
        optimizationFlags["wire_glow"] = true;
        optimizationFlags["gate_shadows"] = true;
        optimizationFlags["antialiasing"] = true;
        optimizationFlags["vsync"] = true;

        // 기본 임계값 설정 (ms)
        frameTimeThresholds[OptimizationLevel::ULTRA_HIGH] = 13.33f;  // 75 FPS
        frameTimeThresholds[OptimizationLevel::HIGH] = 16.67f;        // 60 FPS
        frameTimeThresholds[OptimizationLevel::MEDIUM] = 25.0f;       // 40 FPS
        frameTimeThresholds[OptimizationLevel::LOW] = 33.33f;         // 30 FPS
        frameTimeThresholds[OptimizationLevel::EMERGENCY] = 50.0f;    // 20 FPS
    }

    void PerformanceManager::cleanupOldFrames() {
        auto now = std::chrono::high_resolution_clock::now();
        
        // 1초보다 오래된 프레임 데이터 제거
        while (!frameHistory.empty()) {
            auto age = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - frameHistory.front().timestamp).count();
            
            if (age > 1000) { // 1초
                frameHistory.pop_front();
            } else {
                break;
            }
        }

        // 최대 크기 제한
        while (frameHistory.size() > MAX_FRAME_HISTORY) {
            frameHistory.pop_front();
        }
    }

    void PerformanceManager::applyUltraHighSettings() {
        optimizationFlags["particle_effects"] = true;
        optimizationFlags["signal_animations"] = true;
        optimizationFlags["wire_glow"] = true;
        optimizationFlags["gate_shadows"] = true;
        optimizationFlags["antialiasing"] = true;
        optimizationFlags["vsync"] = true;
        optimizationFlags["high_quality_rendering"] = true;
    }

    void PerformanceManager::applyHighSettings() {
        optimizationFlags["particle_effects"] = true;
        optimizationFlags["signal_animations"] = true;
        optimizationFlags["wire_glow"] = true;
        optimizationFlags["gate_shadows"] = true;
        optimizationFlags["antialiasing"] = true;
        optimizationFlags["vsync"] = true;
        optimizationFlags["high_quality_rendering"] = false;
    }

    void PerformanceManager::applyMediumSettings() {
        optimizationFlags["particle_effects"] = false;
        optimizationFlags["signal_animations"] = true;
        optimizationFlags["wire_glow"] = true;
        optimizationFlags["gate_shadows"] = false;
        optimizationFlags["antialiasing"] = true;
        optimizationFlags["vsync"] = true;
        optimizationFlags["high_quality_rendering"] = false;
    }

    void PerformanceManager::applyLowSettings() {
        optimizationFlags["particle_effects"] = false;
        optimizationFlags["signal_animations"] = false;
        optimizationFlags["wire_glow"] = false;
        optimizationFlags["gate_shadows"] = false;
        optimizationFlags["antialiasing"] = false;
        optimizationFlags["vsync"] = true;
        optimizationFlags["high_quality_rendering"] = false;
        optimizationFlags["static_colors"] = true;
    }

    void PerformanceManager::applyEmergencySettings() {
        optimizationFlags["particle_effects"] = false;
        optimizationFlags["signal_animations"] = false;
        optimizationFlags["wire_glow"] = false;
        optimizationFlags["gate_shadows"] = false;
        optimizationFlags["antialiasing"] = false;
        optimizationFlags["vsync"] = false;
        optimizationFlags["high_quality_rendering"] = false;
        optimizationFlags["static_colors"] = true;
        optimizationFlags["minimal_rendering"] = true;
    }

} // namespace simulation