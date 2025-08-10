#pragma once

#include "SimulationTypes.h"
#include "../core/Circuit.h"
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace simulation {

    struct LoopInfo {
        std::vector<GateId> gateIds;
        float oscillationPeriod;
        bool isStable;
        
        LoopInfo() : oscillationPeriod(0.0f), isStable(false) {}
        LoopInfo(const std::vector<GateId>& gates, float period, bool stable)
            : gateIds(gates), oscillationPeriod(period), isStable(stable) {}
    };

    class LoopDetector {
    public:
        explicit LoopDetector(const Circuit* circuit);
        ~LoopDetector() = default;

        // 루프 감지
        bool detectLoops();
        std::vector<LoopInfo> getAllLoops() const;
        bool isGateInLoop(GateId gateId) const;

        // 발진 분석
        float calculateOscillationPeriod(const LoopInfo& loop) const;
        bool isPotentiallyStable(const LoopInfo& loop) const;

        // 캐시 무효화 (회로 변경 시 호출)
        void invalidateCache();

    private:
        const Circuit* circuit;
        std::vector<LoopInfo> detectedLoops;
        std::unordered_set<GateId> loopGates;

        // DFS 상태
        enum class DFSState {
            WHITE,  // 미방문
            GRAY,   // 방문 중
            BLACK   // 방문 완료
        };

        mutable std::vector<DFSState> dfsState;
        mutable std::vector<GateId> dfsStack;
        mutable std::vector<GateId> currentPath;

        // 인접 리스트 (캐싱용)
        std::vector<std::vector<GateId>> adjacencyList;
        std::unordered_map<GateId, size_t> gateToIndex;
        bool adjacencyListValid = false;

        // 내부 메서드
        bool dfsVisit(size_t gateIndex);
        void extractLoop(const std::vector<GateId>& path, size_t loopStart);
        void buildAdjacencyList();
        void clearDFSState();
        
        // 인덱스 변환 유틸리티
        size_t getGateIndex(GateId gateId) const;
        GateId getGateFromIndex(size_t index) const;
        
        // 루프 분석 헬퍼
        std::vector<GateId> findConnectedGates(GateId gateId) const;
        float estimateGateDelay(GateId gateId) const;
    };

} // namespace simulation