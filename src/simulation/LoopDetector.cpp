#include "LoopDetector.h"
#include <algorithm>

namespace simulation {

    LoopDetector::LoopDetector(const Circuit* circuit) 
        : circuit(circuit) {
        if (circuit) {
            buildAdjacencyList();
        }
    }

    bool LoopDetector::detectLoops() {
        if (!circuit) return false;

        // 인접 리스트가 유효하지 않으면 재구축
        if (!adjacencyListValid) {
            buildAdjacencyList();
        }

        detectedLoops.clear();
        loopGates.clear();
        clearDFSState();

        // 모든 게이트에 대해 DFS 수행
        for (size_t i = 0; i < adjacencyList.size(); ++i) {
            if (dfsState[i] == DFSState::WHITE) {
                currentPath.clear();
                if (dfsVisit(i)) {
                    // 루프가 발견된 경우 처리는 dfsVisit 내부에서 완료
                }
            }
        }

        // 발견된 루프들의 발진 특성 분석
        for (auto& loop : detectedLoops) {
            loop.oscillationPeriod = calculateOscillationPeriod(loop);
            loop.isStable = isPotentiallyStable(loop);
        }

        return !detectedLoops.empty();
    }

    std::vector<LoopInfo> LoopDetector::getAllLoops() const {
        return detectedLoops;
    }

    bool LoopDetector::isGateInLoop(GateId gateId) const {
        return loopGates.find(gateId) != loopGates.end();
    }

    float LoopDetector::calculateOscillationPeriod(const LoopInfo& loop) const {
        if (loop.gateIds.empty()) return 0.0f;

        // 루프 내 모든 게이트의 딜레이 합산
        float totalDelay = 0.0f;
        for (GateId gateId : loop.gateIds) {
            totalDelay += estimateGateDelay(gateId);
        }

        // 발진 주기는 총 딜레이의 2배 (신호가 한 번 돌아오는 시간)
        return totalDelay * 2.0f;
    }

    bool LoopDetector::isPotentiallyStable(const LoopInfo& loop) const {
        if (loop.gateIds.empty()) return true;

        // 루프가 홀수 개의 NOT 게이트를 포함하면 불안정 (발진)
        // 짝수 개면 안정적일 수 있음
        int notGateCount = 0;
        
        for (GateId gateId : loop.gateIds) {
            const Gate* gate = circuit->getGate(gateId);
            if (gate && gate->type == GateType::NOT) {
                notGateCount++;
            }
        }

        // 홀수 개의 NOT 게이트 = 불안정 (발진)
        // 짝수 개의 NOT 게이트 = 안정적일 수 있음
        return (notGateCount % 2) == 0;
    }

    void LoopDetector::invalidateCache() {
        adjacencyListValid = false;
        detectedLoops.clear();
        loopGates.clear();
    }

    bool LoopDetector::dfsVisit(size_t gateIndex) {
        dfsState[gateIndex] = DFSState::GRAY;
        currentPath.push_back(getGateFromIndex(gateIndex));

        // 인접한 모든 게이트 탐색
        for (size_t neighborIndex : adjacencyList[gateIndex]) {
            if (dfsState[neighborIndex] == DFSState::GRAY) {
                // 백 엣지 발견 - 루프 발견!
                GateId neighborGateId = getGateFromIndex(neighborIndex);
                
                // 루프 추출
                auto it = std::find(currentPath.begin(), currentPath.end(), neighborGateId);
                if (it != currentPath.end()) {
                    std::vector<GateId> loopGates(it, currentPath.end());
                    
                    // 중복 루프 확인
                    bool isDuplicate = false;
                    for (const auto& existingLoop : detectedLoops) {
                        if (existingLoop.gateIds.size() == loopGates.size()) {
                            // 단순 비교 (실제로는 더 정교한 중복 검사가 필요할 수 있음)
                            std::vector<GateId> sortedExisting = existingLoop.gateIds;
                            std::vector<GateId> sortedNew = loopGates;
                            
                            std::sort(sortedExisting.begin(), sortedExisting.end());
                            std::sort(sortedNew.begin(), sortedNew.end());
                            
                            if (sortedExisting == sortedNew) {
                                isDuplicate = true;
                                break;
                            }
                        }
                    }

                    if (!isDuplicate) {
                        detectedLoops.emplace_back(loopGates, 0.0f, false);
                        
                        // 루프 게이트들을 집합에 추가
                        for (GateId gateId : loopGates) {
                            this->loopGates.insert(gateId);
                        }
                    }
                }
                
                return true;  // 루프 발견
            } else if (dfsState[neighborIndex] == DFSState::WHITE) {
                if (dfsVisit(neighborIndex)) {
                    // 하위 호출에서 루프가 발견됨
                }
            }
        }

        dfsState[gateIndex] = DFSState::BLACK;
        currentPath.pop_back();
        return false;
    }

    void LoopDetector::extractLoop(const std::vector<GateId>& path, size_t loopStart) {
        if (loopStart >= path.size()) return;

        std::vector<GateId> loopGates(path.begin() + loopStart, path.end());
        detectedLoops.emplace_back(loopGates, 0.0f, false);

        // 루프 게이트들을 집합에 추가
        for (GateId gateId : loopGates) {
            this->loopGates.insert(gateId);
        }
    }

    void LoopDetector::buildAdjacencyList() {
        if (!circuit) return;

        // 게이트 ID를 인덱스로 매핑
        gateToIndex.clear();
        size_t index = 0;
        
        for (auto it = circuit->gatesBegin(); it != circuit->gatesEnd(); ++it) {
            gateToIndex[it->first] = index++;
        }

        // 인접 리스트 초기화
        adjacencyList.clear();
        adjacencyList.resize(gateToIndex.size());

        // 와이어를 통한 연결 관계 구축
        for (auto wireIt = circuit->wiresBegin(); wireIt != circuit->wiresEnd(); ++wireIt) {
            const Wire* wire = &wireIt->second;
            if (!wire) continue;

            // 와이어의 시작 게이트와 끝 게이트 찾기
            GateId fromGateId = Constants::INVALID_GATE_ID;
            GateId toGateId = Constants::INVALID_GATE_ID;

            // 출력 포트를 가진 게이트 찾기 (wire의 from)
            for (auto gateIt = circuit->gatesBegin(); gateIt != circuit->gatesEnd(); ++gateIt) {
                const Gate* gate = &gateIt->second;
                if (gate->outputWire == wireIt->first) {
                    fromGateId = gate->id;
                    break;
                }
            }

            // 입력 포트를 가진 게이트 찾기 (wire의 to)
            for (auto gateIt = circuit->gatesBegin(); gateIt != circuit->gatesEnd(); ++gateIt) {
                const Gate* gate = &gateIt->second;
                for (size_t i = 0; i < gate->inputWires.size(); ++i) {
                    if (gate->inputWires[i] == wireIt->first) {
                        toGateId = gate->id;
                        break;
                    }
                }
                if (toGateId != Constants::INVALID_GATE_ID) break;
            }

            // 유효한 연결인 경우 인접 리스트에 추가
            if (fromGateId != Constants::INVALID_GATE_ID && 
                toGateId != Constants::INVALID_GATE_ID) {
                
                auto fromIt = gateToIndex.find(fromGateId);
                auto toIt = gateToIndex.find(toGateId);
                
                if (fromIt != gateToIndex.end() && toIt != gateToIndex.end()) {
                    adjacencyList[fromIt->second].push_back(toIt->second);
                }
            }
        }

        adjacencyListValid = true;
    }

    void LoopDetector::clearDFSState() {
        dfsState.clear();
        dfsState.resize(adjacencyList.size(), DFSState::WHITE);
        dfsStack.clear();
        currentPath.clear();
    }

    size_t LoopDetector::getGateIndex(GateId gateId) const {
        auto it = gateToIndex.find(gateId);
        return (it != gateToIndex.end()) ? it->second : SIZE_MAX;
    }

    GateId LoopDetector::getGateFromIndex(size_t index) const {
        for (const auto& pair : gateToIndex) {
            if (pair.second == index) {
                return pair.first;
            }
        }
        return Constants::INVALID_GATE_ID;
    }

    std::vector<GateId> LoopDetector::findConnectedGates(GateId gateId) const {
        std::vector<GateId> connected;
        
        size_t gateIndex = getGateIndex(gateId);
        if (gateIndex < adjacencyList.size()) {
            for (size_t neighborIndex : adjacencyList[gateIndex]) {
                connected.push_back(getGateFromIndex(neighborIndex));
            }
        }
        
        return connected;
    }

    float LoopDetector::estimateGateDelay(GateId gateId) const {
        const Gate* gate = circuit->getGate(gateId);
        if (!gate) return 0.0f;

        // NOT 게이트의 기본 딜레이
        switch (gate->type) {
            case GateType::NOT:
                return DEFAULT_GATE_DELAY;  // 0.1초
            default:
                return DEFAULT_GATE_DELAY;
        }
    }

} // namespace simulation