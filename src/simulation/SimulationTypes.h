#pragma once

#include <cstdint>
#include <chrono>

namespace simulation {

    // 성능 통계 구조체
    struct PerformanceStats {
        float frameTime;        // 프레임 시간 (ms)
        float simulationTime;   // 시뮬레이션 시간 (ms)
        size_t activeGates;     // 활성 게이트 수
        size_t signalChanges;   // 프레임당 신호 변경 수
        size_t memoryUsage;     // 메모리 사용량 (bytes)
        int optimizationLevel;  // 현재 최적화 레벨 (0-4)
    };

    // 시뮬레이션 설정
    struct SimulationConfig {
        float gateDelay = 0.1f;         // 게이트 딜레이 (초)
        float simulationSpeed = 1.0f;   // 시뮬레이션 속도 배율
        size_t maxSignals = 1000000;    // 최대 신호 수
        size_t maxGates = 100000;       // 최대 게이트 수
        bool enableSIMD = true;         // SIMD 최적화 활성화
        bool enableLoopDetection = true; // 루프 감지 활성화
    };

    // 상수 정의
    static constexpr uint32_t INVALID_SIGNAL = UINT32_MAX;
    static constexpr uint32_t INVALID_GATE = UINT32_MAX;
    static constexpr size_t SIGNALS_PER_WORD = 32;
    static constexpr size_t CACHE_LINE_SIZE = 64;
    static constexpr float DEFAULT_GATE_DELAY = 0.1f;
    static constexpr size_t MAX_PROPAGATION_DEPTH = 1000;

    // 고해상도 타이머 타입
    using TimePoint = std::chrono::high_resolution_clock::time_point;
    using Duration = std::chrono::nanoseconds;

} // namespace simulation