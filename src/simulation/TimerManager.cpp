#include "TimerManager.h"
#include <algorithm>

namespace simulation {

    TimerManager::TimerManager() {
        expiredTimers.reserve(256);  // 일반적으로 프레임당 많은 타이머가 만료되지 않음
    }

    void TimerManager::scheduleTimer(uint32_t gateId, float delay, bool pendingOutput, uint8_t priority) {
        std::lock_guard<std::mutex> lock(timerMutex);

        // 기존 타이머가 있으면 취소 (내부적으로 처리)
        gateToRemainingTime[gateId] = delay;

        // 새 타이머 생성 및 큐에 추가
        GateTimer timer;
        timer.gateId = gateId;
        timer.remainingTime = delay;
        timer.pendingOutput = pendingOutput;
        timer.priority = priority;

        timerQueue.push(timer);
    }

    void TimerManager::cancelTimer(uint32_t gateId) {
        std::lock_guard<std::mutex> lock(timerMutex);

        // 맵에서 제거 (큐에서는 updateTimers에서 자연스럽게 제거됨)
        gateToRemainingTime.erase(gateId);
    }

    bool TimerManager::hasActiveTimer(uint32_t gateId) const {
        std::lock_guard<std::mutex> lock(timerMutex);
        return gateToRemainingTime.find(gateId) != gateToRemainingTime.end();
    }

    void TimerManager::updateTimers(float deltaTime) {
        if (deltaTime <= 0.0f) return;

        std::lock_guard<std::mutex> lock(timerMutex);

        // 모든 활성 타이머의 남은 시간 업데이트
        for (auto& pair : gateToRemainingTime) {
            pair.second -= deltaTime;
        }

        // 만료된 타이머들을 처리
        std::vector<uint32_t> expiredGates;
        
        while (!timerQueue.empty()) {
            const GateTimer& topTimer = timerQueue.top();
            
            // 이 게이트가 여전히 활성 상태이고 만료되었는지 확인
            auto it = gateToRemainingTime.find(topTimer.gateId);
            if (it == gateToRemainingTime.end()) {
                // 이미 취소된 타이머, 큐에서 제거
                timerQueue.pop();
                continue;
            }

            if (it->second <= 0.0f) {
                // 타이머 만료
                expiredTimers.emplace_back(topTimer.gateId, topTimer.pendingOutput);
                expiredGates.push_back(topTimer.gateId);
                timerQueue.pop();
            } else {
                // 아직 만료되지 않은 타이머에 도달했으므로 중단
                break;
            }
        }

        // 만료된 게이트들을 맵에서 제거
        for (uint32_t gateId : expiredGates) {
            gateToRemainingTime.erase(gateId);
        }
    }

    std::vector<std::pair<uint32_t, bool>> TimerManager::getExpiredTimers() {
        std::lock_guard<std::mutex> lock(timerMutex);
        return expiredTimers;
    }

    void TimerManager::clearExpiredTimers() {
        std::lock_guard<std::mutex> lock(timerMutex);
        expiredTimers.clear();
    }

    size_t TimerManager::getActiveTimerCount() const {
        std::lock_guard<std::mutex> lock(timerMutex);
        return gateToRemainingTime.size();
    }

    float TimerManager::getRemainingTime(uint32_t gateId) const {
        std::lock_guard<std::mutex> lock(timerMutex);
        
        auto it = gateToRemainingTime.find(gateId);
        if (it != gateToRemainingTime.end()) {
            return std::max(0.0f, it->second);
        }
        
        return 0.0f;
    }

    void TimerManager::reset() {
        std::lock_guard<std::mutex> lock(timerMutex);
        
        // 모든 타이머 제거
        while (!timerQueue.empty()) {
            timerQueue.pop();
        }
        
        gateToRemainingTime.clear();
        expiredTimers.clear();
    }

    void TimerManager::cleanupExpiredTimers() {
        // 만료된 타이머들을 큐에서 제거하는 내부 함수
        // updateTimers에서 이미 처리되므로 별도 구현 불필요
    }

    void TimerManager::validateTimerQueue() {
        // 디버그 모드에서 타이머 큐의 일관성 검증
        #ifdef _DEBUG
        std::lock_guard<std::mutex> lock(timerMutex);
        
        // 큐의 모든 타이머가 맵에 존재하는지 확인
        std::priority_queue<GateTimer> tempQueue = timerQueue;
        
        while (!tempQueue.empty()) {
            const GateTimer& timer = tempQueue.top();
            
            if (gateToRemainingTime.find(timer.gateId) == gateToRemainingTime.end()) {
                // 일관성 오류 - 큐에는 있지만 맵에는 없음
                // 로깅이나 예외 처리 필요
            }
            
            tempQueue.pop();
        }
        #endif
    }

} // namespace simulation