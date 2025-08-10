#pragma once

#include "SimulationTypes.h"
#include <vector>
#include <shared_mutex>
#include <unordered_set>

#ifdef __AVX2__
#include <immintrin.h>
#endif

namespace simulation {

    class SignalManager {
    public:
        SignalManager(size_t maxSignals = 1000000);
        ~SignalManager();

        // 신호 상태 조회/설정
        bool getSignal(uint32_t signalId) const;
        void setSignal(uint32_t signalId, bool value);

        // 배치 처리
        void setMultipleSignals(const std::vector<std::pair<uint32_t, bool>>& signals);
        std::vector<uint32_t> getChangedSignals() const;
        void clearChangedSignals();

        // 최적화된 연산
        void clearAllSignals();
        void applyBatchChanges();

        // SIMD 최적화된 연산
        void propagateSignalsSIMD();

        // 상태 조회
        size_t getSignalCount() const { return maxSignals; }
        size_t getChangedCount() const { return changedSignals.size(); }

    private:
        const size_t maxSignals;
        const size_t signalWords;

        // Structure of Arrays 패턴으로 캐시 효율성 극대화
        alignas(CACHE_LINE_SIZE) uint32_t* signalBits;
        alignas(CACHE_LINE_SIZE) uint32_t* previousBits;  // 변경 감지용
        alignas(CACHE_LINE_SIZE) uint32_t* dirtyMask;     // 더티 플래그

        // 변경된 신호 추적
        std::vector<uint32_t> changedSignals;
        std::vector<std::pair<uint32_t, bool>> pendingChanges;

        // 스레드 안전성
        mutable std::shared_mutex signalMutex;

        // 내부 메서드
        void markDirty(uint32_t signalId);
        void updateChangedList();
        void applyPendingChanges();
        
#ifdef __AVX2__
        void processChangedBatch(size_t wordIndex, __m256i changed);
#endif
    };

} // namespace simulation