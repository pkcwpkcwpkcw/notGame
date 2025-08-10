#pragma once

#include "SimulationTypes.h"
#include <chrono>
#include <deque>
#include <unordered_map>
#include <string>

namespace simulation {

    class PerformanceManager {
    public:
        enum class OptimizationLevel {
            ULTRA_HIGH = 0,  // 모든 기능 활성
            HIGH = 1,        // 기본 최적화
            MEDIUM = 2,      // 애니메이션 간소화
            LOW = 3,         // 시각 효과 최소화
            EMERGENCY = 4    // 최소 기능만 유지
        };

        PerformanceManager();
        ~PerformanceManager() = default;

        // 성능 모니터링
        void beginFrame();
        void endFrame();
        void recordSimulationTime(float time);

        // 적응형 최적화
        void updateOptimization();
        OptimizationLevel getCurrentLevel() const { return currentLevel; }
        void setTargetFrameRate(float fps) { targetFrameTime = 1000.0f / fps; }

        // 통계
        PerformanceStats getStats() const;
        void resetStats();

        // 최적화 설정
        void enableOptimization(const std::string& name, bool enabled);
        bool isOptimizationEnabled(const std::string& name) const;

        // 성능 임계값 설정
        void setFrameTimeThreshold(OptimizationLevel level, float threshold);

    private:
        OptimizationLevel currentLevel;
        float targetFrameTime;  // 목표 프레임 시간 (ms)

        // 성능 측정
        TimePoint frameStart;
        float frameTime;
        float simulationTime;
        float lastOptimizationCheck;

        // 통계 수집
        struct FrameStats {
            float frameTime;
            float simulationTime;
            size_t activeGates;
            size_t signalChanges;
            TimePoint timestamp;
        };

        std::deque<FrameStats> frameHistory;
        static constexpr size_t MAX_FRAME_HISTORY = 60; // 1초간

        // 최적화 플래그
        std::unordered_map<std::string, bool> optimizationFlags;

        // 성능 임계값 (ms)
        std::unordered_map<OptimizationLevel, float> frameTimeThresholds;

        // 적응형 조절
        void adjustOptimizationLevel();
        void applyOptimizations();
        void restoreFeatures();

        // 내부 메서드
        float getAverageFrameTime() const;
        float getAverageSimulationTime() const;
        bool shouldIncreaseOptimization() const;
        bool shouldDecreaseOptimization() const;

        void initializeDefaults();
        void cleanupOldFrames();

        // 최적화 레벨별 설정
        void applyUltraHighSettings();
        void applyHighSettings();
        void applyMediumSettings();
        void applyLowSettings();
        void applyEmergencySettings();
    };

} // namespace simulation