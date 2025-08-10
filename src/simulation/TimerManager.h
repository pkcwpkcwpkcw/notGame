#pragma once

#include "SimulationTypes.h"
#include <queue>
#include <unordered_map>
#include <mutex>
#include <vector>

namespace simulation {

    class TimerManager {
    public:
        struct GateTimer {
            uint32_t gateId;
            float remainingTime;
            bool pendingOutput;
            uint8_t priority;  // 동시 만료 시 처리 순서

            bool operator<(const GateTimer& other) const {
                // 시간이 빠른 순서, 우선순위가 높은 순서 (min-heap)
                if (remainingTime != other.remainingTime) {
                    return remainingTime > other.remainingTime;
                }
                return priority > other.priority;
            }
        };

        TimerManager();
        ~TimerManager() = default;

        // 타이머 관리
        void scheduleTimer(uint32_t gateId, float delay, bool pendingOutput, uint8_t priority = 0);
        void cancelTimer(uint32_t gateId);
        bool hasActiveTimer(uint32_t gateId) const;

        // 업데이트
        void updateTimers(float deltaTime);
        std::vector<std::pair<uint32_t, bool>> getExpiredTimers();
        void clearExpiredTimers();

        // 상태 조회
        size_t getActiveTimerCount() const;
        float getRemainingTime(uint32_t gateId) const;

        // 초기화
        void reset();

    private:
        // 우선순위 큐로 효율적인 타이머 관리
        std::priority_queue<GateTimer> timerQueue;
        std::unordered_map<uint32_t, float> gateToRemainingTime;  // 빠른 조회를 위한 맵

        // 만료된 타이머 임시 저장
        std::vector<std::pair<uint32_t, bool>> expiredTimers;

        // 스레드 안전성
        mutable std::mutex timerMutex;

        void cleanupExpiredTimers();
        void validateTimerQueue();  // 디버그용 검증 함수
    };

} // namespace simulation