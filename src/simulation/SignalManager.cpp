#include "SignalManager.h"
#include <algorithm>
#include <cstring>
#ifdef _MSC_VER
#include <malloc.h>
#endif

namespace simulation {

    SignalManager::SignalManager(size_t maxSignals)
        : maxSignals(maxSignals)
        , signalWords((maxSignals + SIGNALS_PER_WORD - 1) / SIGNALS_PER_WORD)
    {
        // 캐시 라인 정렬된 메모리 할당
#ifdef _MSC_VER
        signalBits = static_cast<uint32_t*>(_aligned_malloc(signalWords * sizeof(uint32_t), CACHE_LINE_SIZE));
        previousBits = static_cast<uint32_t*>(_aligned_malloc(signalWords * sizeof(uint32_t), CACHE_LINE_SIZE));
        dirtyMask = static_cast<uint32_t*>(_aligned_malloc(signalWords * sizeof(uint32_t), CACHE_LINE_SIZE));
#else
        signalBits = static_cast<uint32_t*>(std::aligned_alloc(CACHE_LINE_SIZE, signalWords * sizeof(uint32_t)));
        previousBits = static_cast<uint32_t*>(std::aligned_alloc(CACHE_LINE_SIZE, signalWords * sizeof(uint32_t)));
        dirtyMask = static_cast<uint32_t*>(std::aligned_alloc(CACHE_LINE_SIZE, signalWords * sizeof(uint32_t)));
#endif

        if (!signalBits || !previousBits || !dirtyMask) {
            throw std::bad_alloc();
        }

        // 초기화
        clearAllSignals();
        changedSignals.reserve(1024);
        pendingChanges.reserve(1024);
    }

    SignalManager::~SignalManager() {
#ifdef _MSC_VER
        _aligned_free(signalBits);
        _aligned_free(previousBits);
        _aligned_free(dirtyMask);
#else
        std::free(signalBits);
        std::free(previousBits);
        std::free(dirtyMask);
#endif
    }

    bool SignalManager::getSignal(uint32_t signalId) const {
        if (signalId >= maxSignals) return false;

        std::shared_lock lock(signalMutex);
        
        uint32_t wordIndex = signalId / SIGNALS_PER_WORD;
        uint32_t bitIndex = signalId % SIGNALS_PER_WORD;
        
        return (signalBits[wordIndex] >> bitIndex) & 1;
    }

    void SignalManager::setSignal(uint32_t signalId, bool value) {
        if (signalId >= maxSignals) return;

        std::unique_lock lock(signalMutex);
        
        uint32_t wordIndex = signalId / SIGNALS_PER_WORD;
        uint32_t bitIndex = signalId % SIGNALS_PER_WORD;
        
        uint32_t mask = 1U << bitIndex;
        bool currentValue = (signalBits[wordIndex] & mask) != 0;
        
        // 값이 변경된 경우에만 처리
        if (currentValue != value) {
            if (value) {
                signalBits[wordIndex] |= mask;
            } else {
                signalBits[wordIndex] &= ~mask;
            }
            
            markDirty(signalId);
        }
    }

    void SignalManager::setMultipleSignals(const std::vector<std::pair<uint32_t, bool>>& signals) {
        std::unique_lock lock(signalMutex);
        
        for (const auto& [signalId, value] : signals) {
            if (signalId >= maxSignals) continue;
            
            uint32_t wordIndex = signalId / SIGNALS_PER_WORD;
            uint32_t bitIndex = signalId % SIGNALS_PER_WORD;
            
            uint32_t mask = 1U << bitIndex;
            bool currentValue = (signalBits[wordIndex] & mask) != 0;
            
            if (currentValue != value) {
                if (value) {
                    signalBits[wordIndex] |= mask;
                } else {
                    signalBits[wordIndex] &= ~mask;
                }
                
                markDirty(signalId);
            }
        }
    }

    std::vector<uint32_t> SignalManager::getChangedSignals() const {
        std::shared_lock lock(signalMutex);
        return changedSignals;
    }

    void SignalManager::clearChangedSignals() {
        std::unique_lock lock(signalMutex);
        changedSignals.clear();
    }

    void SignalManager::clearAllSignals() {
        std::unique_lock lock(signalMutex);
        
        std::memset(signalBits, 0, signalWords * sizeof(uint32_t));
        std::memset(previousBits, 0, signalWords * sizeof(uint32_t));
        std::memset(dirtyMask, 0, signalWords * sizeof(uint32_t));
        
        changedSignals.clear();
        pendingChanges.clear();
    }

    void SignalManager::applyBatchChanges() {
        if (pendingChanges.empty()) return;
        
        std::unique_lock lock(signalMutex);
        applyPendingChanges();
    }

    void SignalManager::propagateSignalsSIMD() {
#ifdef __AVX2__
        const size_t simdWords = signalWords / 8; // 256비트씩 처리
        
        for (size_t i = 0; i < simdWords; ++i) {
            // 8개의 uint32_t를 한 번에 로드
            __m256i current = _mm256_load_si256(
                reinterpret_cast<const __m256i*>(&signalBits[i * 8]));
            __m256i previous = _mm256_load_si256(
                reinterpret_cast<const __m256i*>(&previousBits[i * 8]));
            
            // XOR로 변경된 비트 감지
            __m256i changed = _mm256_xor_si256(current, previous);
            
            // 변경된 비트가 있으면 처리
            if (!_mm256_testz_si256(changed, changed)) {
                processChangedBatch(i * 8, changed);
            }
            
            // 이전 상태 업데이트
            _mm256_store_si256(
                reinterpret_cast<__m256i*>(&previousBits[i * 8]), current);
        }
        
        // 남은 워드들 처리 (SIMD로 처리되지 않은 부분)
        for (size_t i = simdWords * 8; i < signalWords; ++i) {
            if (signalBits[i] != previousBits[i]) {
                uint32_t changed = signalBits[i] ^ previousBits[i];
                
                // 변경된 비트들을 찾아서 changedSignals에 추가
                for (uint32_t bit = 0; bit < SIGNALS_PER_WORD; ++bit) {
                    if (changed & (1U << bit)) {
                        uint32_t signalId = static_cast<uint32_t>(i * SIGNALS_PER_WORD + bit);
                        if (signalId < maxSignals) {
                            changedSignals.push_back(signalId);
                        }
                    }
                }
                
                previousBits[i] = signalBits[i];
            }
        }
#else
        // 비SIMD 버전: 모든 워드를 순차적으로 처리
        updateChangedList();
#endif
    }

    void SignalManager::markDirty(uint32_t signalId) {
        uint32_t wordIndex = signalId / SIGNALS_PER_WORD;
        uint32_t bitIndex = signalId % SIGNALS_PER_WORD;
        
        dirtyMask[wordIndex] |= (1U << bitIndex);
        changedSignals.push_back(signalId);
    }

    void SignalManager::updateChangedList() {
        changedSignals.clear();
        
        for (size_t i = 0; i < signalWords; ++i) {
            if (signalBits[i] != previousBits[i]) {
                uint32_t changed = signalBits[i] ^ previousBits[i];
                
                // 변경된 비트들을 찾아서 changedSignals에 추가
                for (uint32_t bit = 0; bit < SIGNALS_PER_WORD; ++bit) {
                    if (changed & (1U << bit)) {
                        uint32_t signalId = static_cast<uint32_t>(i * SIGNALS_PER_WORD + bit);
                        if (signalId < maxSignals) {
                            changedSignals.push_back(signalId);
                        }
                    }
                }
                
                previousBits[i] = signalBits[i];
            }
        }
    }

    void SignalManager::applyPendingChanges() {
        for (const auto& [signalId, value] : pendingChanges) {
            if (signalId >= maxSignals) continue;
            
            uint32_t wordIndex = signalId / SIGNALS_PER_WORD;
            uint32_t bitIndex = signalId % SIGNALS_PER_WORD;
            
            uint32_t mask = 1U << bitIndex;
            bool currentValue = (signalBits[wordIndex] & mask) != 0;
            
            if (currentValue != value) {
                if (value) {
                    signalBits[wordIndex] |= mask;
                } else {
                    signalBits[wordIndex] &= ~mask;
                }
                
                markDirty(signalId);
            }
        }
        
        pendingChanges.clear();
    }

#ifdef __AVX2__
    void SignalManager::processChangedBatch(size_t wordIndex, __m256i changed) {
        // AVX2 레지스터를 배열로 변환
        alignas(32) uint32_t changedWords[8];
        _mm256_store_si256(reinterpret_cast<__m256i*>(changedWords), changed);
        
        // 각 워드에서 변경된 비트들을 찾아 처리
        for (size_t i = 0; i < 8; ++i) {
            if (changedWords[i] != 0) {
                for (uint32_t bit = 0; bit < SIGNALS_PER_WORD; ++bit) {
                    if (changedWords[i] & (1U << bit)) {
                        uint32_t signalId = static_cast<uint32_t>((wordIndex + i) * SIGNALS_PER_WORD + bit);
                        if (signalId < maxSignals) {
                            changedSignals.push_back(signalId);
                        }
                    }
                }
            }
        }
    }
#endif

} // namespace simulation