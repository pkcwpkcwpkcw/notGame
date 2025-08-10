#pragma once
#include "CellWire.h"
#include "Types.h"
#include <unordered_map>
#include <set>
#include <vector>
#include <glm/glm.hpp>
#include <SDL.h>

class Circuit;

class CellWireManager {
public:
    CellWireManager(Circuit* circuit);
    ~CellWireManager();
    
    // 드래그 이벤트 처리
    void onDragStart(const glm::vec2& worldPos);
    void onDragMove(const glm::vec2& worldPos);
    void onDragEnd(const glm::vec2& worldPos);
    
    // 셀에 와이어 설치/제거
    void placeWireAt(const glm::ivec2& gridPos);
    void removeWireAt(const glm::ivec2& gridPos);
    void removeWiresInArea(const glm::ivec2& min, const glm::ivec2& max);  // 영역 내 와이어 제거
    
    // 두 셀 사이 연결
    void connectCells(const glm::ivec2& from, const glm::ivec2& to);
    
    // 와이어 정보 조회
    CellWire* getWireAt(const glm::ivec2& gridPos);
    const CellWire* getWireAt(const glm::ivec2& gridPos) const;
    
    // 모든 와이어 가져오기 (렌더링용)
    const std::unordered_map<uint64_t, CellWire>& getAllWires() const { return m_cellWires; }
    
    // 신호 업데이트
    void updateSignals();
    
private:
    Circuit* m_circuit;
    
    // 그리드 좌표를 키로 사용하는 와이어 맵
    std::unordered_map<uint64_t, CellWire> m_cellWires;
    
    // 드래그 상태
    bool m_isDragging{false};
    glm::ivec2 m_lastGridPos;
    glm::ivec2 m_dragStartPos;
    
    // 그리드 좌표를 해시키로 변환
    uint64_t gridToKey(const glm::ivec2& gridPos) const {
        // 32비트씩 나눠서 64비트 키 생성
        uint32_t x = static_cast<uint32_t>(gridPos.x + 0x7FFFFFFF);
        uint32_t y = static_cast<uint32_t>(gridPos.y + 0x7FFFFFFF);
        return (static_cast<uint64_t>(x) << 32) | static_cast<uint64_t>(y);
    }
    
    // 두 셀 사이의 방향 계산
    WireDirection getDirection(const glm::ivec2& from, const glm::ivec2& to) const;
    WireDirection getOppositeDirection(WireDirection dir) const;
    
    // 신호 전파 헬퍼 함수들
    void propagateSignal(const glm::ivec2& startPos);
    void checkGateInputs(const glm::ivec2& wirePos);
};