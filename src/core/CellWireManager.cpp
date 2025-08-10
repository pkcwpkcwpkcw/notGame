#include "CellWireManager.h"
#include "Circuit.h"
#include <cmath>

CellWireManager::CellWireManager(Circuit* circuit)
    : m_circuit(circuit) {
}

CellWireManager::~CellWireManager() = default;

void CellWireManager::onDragStart(const glm::vec2& worldPos) {
    m_isDragging = true;
    m_dragStartPos = glm::ivec2(std::floor(worldPos.x), std::floor(worldPos.y));
    m_lastGridPos = m_dragStartPos;
    
    // 시작 위치에 와이어 설치 (중앙 점)
    placeWireAt(m_dragStartPos);
    
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, 
                "[CellWireManager] Drag started at cell (%d, %d)", 
                m_dragStartPos.x, m_dragStartPos.y);
}

void CellWireManager::onDragMove(const glm::vec2& worldPos) {
    if (!m_isDragging) return;
    
    glm::ivec2 currentGridPos(std::floor(worldPos.x), std::floor(worldPos.y));
    
    // 새로운 셀로 이동했는지 확인
    if (currentGridPos != m_lastGridPos) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, 
                    "[CellWireManager] Moved from cell (%d, %d) to (%d, %d)", 
                    m_lastGridPos.x, m_lastGridPos.y,
                    currentGridPos.x, currentGridPos.y);
        
        // 현재 셀에 와이어 설치
        placeWireAt(currentGridPos);
        
        // 이전 셀과 현재 셀을 연결
        connectCells(m_lastGridPos, currentGridPos);
        
        m_lastGridPos = currentGridPos;
    }
}

void CellWireManager::onDragEnd(const glm::vec2& worldPos) {
    if (!m_isDragging) return;
    
    m_isDragging = false;
    
    glm::ivec2 endGridPos(std::floor(worldPos.x), std::floor(worldPos.y));
    
    // 마지막 위치가 다르면 연결
    if (endGridPos != m_lastGridPos) {
        placeWireAt(endGridPos);
        connectCells(m_lastGridPos, endGridPos);
    }
    
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, 
                "[CellWireManager] Drag ended. Total wires: %zu", 
                m_cellWires.size());
}

void CellWireManager::placeWireAt(const glm::ivec2& gridPos) {
    // 해당 위치에 게이트가 있는지 확인
    if (m_circuit) {
        Vec2 worldPos{static_cast<float>(gridPos.x) + 0.5f, static_cast<float>(gridPos.y) + 0.5f};
        GateId gateId = m_circuit->getGateAt(worldPos, 0.7f);
        if (gateId != Constants::INVALID_GATE_ID) {
            // 게이트가 있으면 와이어 설치 불가
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, 
                        "[CellWireManager] Cannot place wire at (%d, %d) - gate exists", 
                        gridPos.x, gridPos.y);
            return;
        }
    }
    
    uint64_t key = gridToKey(gridPos);
    
    // 이미 와이어가 있으면 스킵
    if (m_cellWires.find(key) != m_cellWires.end()) {
        return;
    }
    
    // 새 와이어 생성
    CellWire wire;
    wire.cellPos = Vec2{static_cast<float>(gridPos.x), static_cast<float>(gridPos.y)};
    wire.exists = true;
    wire.connections = WireDirection::None;  // 연결은 connectCells에서 설정
    
    m_cellWires[key] = wire;
    
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, 
                "[CellWireManager] Wire placed at cell (%d, %d)", 
                gridPos.x, gridPos.y);
}

void CellWireManager::removeWireAt(const glm::ivec2& gridPos) {
    uint64_t key = gridToKey(gridPos);
    
    // 와이어 제거 전에 연결된 와이어들의 연결 정보도 업데이트
    CellWire* wire = getWireAt(gridPos);
    if (wire) {
        // 상하좌우 인접 와이어들의 연결 정보 제거
        if (wire->hasConnection(WireDirection::Up)) {
            CellWire* upWire = getWireAt(gridPos + glm::ivec2(0, -1));
            if (upWire) {
                upWire->removeConnection(WireDirection::Down);
            }
        }
        if (wire->hasConnection(WireDirection::Down)) {
            CellWire* downWire = getWireAt(gridPos + glm::ivec2(0, 1));
            if (downWire) {
                downWire->removeConnection(WireDirection::Up);
            }
        }
        if (wire->hasConnection(WireDirection::Left)) {
            CellWire* leftWire = getWireAt(gridPos + glm::ivec2(-1, 0));
            if (leftWire) {
                leftWire->removeConnection(WireDirection::Right);
            }
        }
        if (wire->hasConnection(WireDirection::Right)) {
            CellWire* rightWire = getWireAt(gridPos + glm::ivec2(1, 0));
            if (rightWire) {
                rightWire->removeConnection(WireDirection::Left);
            }
        }
    }
    
    m_cellWires.erase(key);
    
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, 
                "[CellWireManager] Wire removed at cell (%d, %d)", 
                gridPos.x, gridPos.y);
}

void CellWireManager::removeWiresInArea(const glm::ivec2& min, const glm::ivec2& max) {
    std::vector<glm::ivec2> toRemove;
    
    // 영역 내의 모든 와이어 찾기
    for (const auto& [key, wire] : m_cellWires) {
        glm::ivec2 pos(wire.cellPos.x, wire.cellPos.y);
        if (pos.x >= min.x && pos.x <= max.x && 
            pos.y >= min.y && pos.y <= max.y) {
            toRemove.push_back(pos);
        }
    }
    
    // 찾은 와이어들 제거
    for (const auto& pos : toRemove) {
        removeWireAt(pos);
    }
    
    if (!toRemove.empty()) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, 
                    "[CellWireManager] Removed %zu wires in area (%d,%d) to (%d,%d)", 
                    toRemove.size(), min.x, min.y, max.x, max.y);
    }
}

void CellWireManager::connectCells(const glm::ivec2& from, const glm::ivec2& to) {
    // 인접한 셀인지 확인
    glm::ivec2 diff = to - from;
    if (std::abs(diff.x) + std::abs(diff.y) != 1) {
        // 대각선이거나 너무 멀면 스킵
        return;
    }
    
    // from 셀의 와이어 가져오기
    CellWire* fromWire = getWireAt(from);
    if (!fromWire) {
        placeWireAt(from);
        fromWire = getWireAt(from);
        if (!fromWire) return; // 게이트가 있어서 생성 실패
    }
    
    // to 셀의 와이어 가져오기
    CellWire* toWire = getWireAt(to);
    if (!toWire) {
        placeWireAt(to);
        toWire = getWireAt(to);
        if (!toWire) return; // 게이트가 있어서 생성 실패
    }
    
    // 방향 계산
    WireDirection fromToDir = getDirection(from, to);
    WireDirection toFromDir = getOppositeDirection(fromToDir);
    
    // 연결 추가
    if (fromWire) {
        fromWire->addConnection(fromToDir);
    }
    if (toWire) {
        toWire->addConnection(toFromDir);
    }
    
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, 
                "[CellWireManager] Connected cells (%d, %d) -> (%d, %d)", 
                from.x, from.y, to.x, to.y);
}

CellWire* CellWireManager::getWireAt(const glm::ivec2& gridPos) {
    uint64_t key = gridToKey(gridPos);
    auto it = m_cellWires.find(key);
    return (it != m_cellWires.end()) ? &it->second : nullptr;
}

const CellWire* CellWireManager::getWireAt(const glm::ivec2& gridPos) const {
    uint64_t key = gridToKey(gridPos);
    auto it = m_cellWires.find(key);
    return (it != m_cellWires.end()) ? &it->second : nullptr;
}

void CellWireManager::updateSignals() {
    if (!m_circuit) return;
    
    // 모든 와이어의 신호 상태 초기화
    for (auto& [key, wire] : m_cellWires) {
        wire.hasSignal = false;
    }
    
    // 게이트의 출력 신호를 인접 와이어로 전파
    for (auto it = m_circuit->gatesBegin(); it != m_circuit->gatesEnd(); ++it) {
        const Gate& gate = it->second;
        
        // 게이트가 HIGH 출력일 때만 신호 전파
        if (gate.currentOutput == SignalState::HIGH) {
            // 게이트의 출력 포트 위치 계산
            Vec2 outputPortPos = gate.getOutputPortPosition();
            glm::ivec2 outputCellPos(std::floor(outputPortPos.x), std::floor(outputPortPos.y));
            
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                       "[CellWireManager] Gate at (%.1f, %.1f) outputting HIGH to cell (%d, %d)",
                       gate.position.x, gate.position.y, outputCellPos.x, outputCellPos.y);
            
            // 해당 위치의 와이어에 신호 설정
            CellWire* wire = getWireAt(outputCellPos);
            if (wire) {
                wire->hasSignal = true;
                SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                           "[CellWireManager] Setting signal HIGH at wire (%d, %d)",
                           outputCellPos.x, outputCellPos.y);
                propagateSignal(outputCellPos);  // 연결된 와이어로 신호 전파
            } else {
                SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                           "[CellWireManager] No wire found at output position (%d, %d)",
                           outputCellPos.x, outputCellPos.y);
            }
        }
    }
}

void CellWireManager::propagateSignal(const glm::ivec2& startPos) {
    std::vector<glm::ivec2> toVisit;
    std::set<uint64_t> visited;
    
    toVisit.push_back(startPos);
    visited.insert(gridToKey(startPos));
    
    while (!toVisit.empty()) {
        glm::ivec2 currentPos = toVisit.back();
        toVisit.pop_back();
        
        CellWire* currentWire = getWireAt(currentPos);
        if (!currentWire) continue;
        
        currentWire->hasSignal = true;
        
        // 연결된 인접 와이어들로 신호 전파
        if (currentWire->hasConnection(WireDirection::Up)) {
            glm::ivec2 upPos = currentPos + glm::ivec2(0, -1);
            uint64_t upKey = gridToKey(upPos);
            if (visited.find(upKey) == visited.end()) {
                visited.insert(upKey);
                toVisit.push_back(upPos);
            }
        }
        if (currentWire->hasConnection(WireDirection::Down)) {
            glm::ivec2 downPos = currentPos + glm::ivec2(0, 1);
            uint64_t downKey = gridToKey(downPos);
            if (visited.find(downKey) == visited.end()) {
                visited.insert(downKey);
                toVisit.push_back(downPos);
            }
        }
        if (currentWire->hasConnection(WireDirection::Left)) {
            glm::ivec2 leftPos = currentPos + glm::ivec2(-1, 0);
            uint64_t leftKey = gridToKey(leftPos);
            if (visited.find(leftKey) == visited.end()) {
                visited.insert(leftKey);
                toVisit.push_back(leftPos);
            }
        }
        if (currentWire->hasConnection(WireDirection::Right)) {
            glm::ivec2 rightPos = currentPos + glm::ivec2(1, 0);
            uint64_t rightKey = gridToKey(rightPos);
            if (visited.find(rightKey) == visited.end()) {
                visited.insert(rightKey);
                toVisit.push_back(rightPos);
            }
        }
        
        // 게이트 입력 포트 확인
        checkGateInputs(currentPos);
    }
}

void CellWireManager::checkGateInputs(const glm::ivec2& wirePos) {
    if (!m_circuit) return;
    
    // 와이어가 게이트의 입력 포트 근처에 있는지 확인
    for (auto it = m_circuit->gatesBegin(); it != m_circuit->gatesEnd(); ++it) {
        Gate& gate = it->second;
        
        // 3개의 입력 포트 위치 확인
        for (int port = 0; port < 3; ++port) {
            Vec2 inputPortPos = gate.getInputPortPosition(port);
            glm::ivec2 inputCellPos(std::floor(inputPortPos.x), std::floor(inputPortPos.y));
            
            // 와이어 위치와 입력 포트 위치가 일치하는지 확인
            if (wirePos == inputCellPos) {
                // 해당 포트에 신호가 있음을 표시
                gate.isDirty = true;
                
                SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                           "[CellWireManager] Wire at (%d, %d) connected to gate input port %d",
                           wirePos.x, wirePos.y, port);
                break;
            }
        }
    }
}

WireDirection CellWireManager::getDirection(const glm::ivec2& from, const glm::ivec2& to) const {
    glm::ivec2 diff = to - from;
    
    if (diff.x == 1 && diff.y == 0) return WireDirection::Right;
    if (diff.x == -1 && diff.y == 0) return WireDirection::Left;
    if (diff.x == 0 && diff.y == 1) return WireDirection::Down;
    if (diff.x == 0 && diff.y == -1) return WireDirection::Up;
    
    return WireDirection::None;
}

WireDirection CellWireManager::getOppositeDirection(WireDirection dir) const {
    switch (dir) {
        case WireDirection::Up: return WireDirection::Down;
        case WireDirection::Down: return WireDirection::Up;
        case WireDirection::Left: return WireDirection::Right;
        case WireDirection::Right: return WireDirection::Left;
        default: return WireDirection::None;
    }
}