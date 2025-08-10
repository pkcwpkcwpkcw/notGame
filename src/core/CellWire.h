#pragma once
#include "Types.h"
#include "Vec2.h"
#include <bitset>

// 와이어의 연결 방향을 나타내는 enum
enum class WireDirection : uint8_t {
    None   = 0,
    Up     = 1 << 0,  // 0001
    Right  = 1 << 1,  // 0010
    Down   = 1 << 2,  // 0100
    Left   = 1 << 3,  // 1000
    All    = Up | Right | Down | Left
};

inline WireDirection operator|(WireDirection a, WireDirection b) {
    return static_cast<WireDirection>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline WireDirection operator&(WireDirection a, WireDirection b) {
    return static_cast<WireDirection>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline WireDirection& operator|=(WireDirection& a, WireDirection b) {
    a = a | b;
    return a;
}

// 셀 하나에 있는 와이어 정보
struct CellWire {
    Vec2 cellPos;                     // 셀 위치 (그리드 좌표)
    WireDirection connections{WireDirection::None};  // 연결된 방향들
    SignalState signalState{SignalState::LOW};       // 신호 상태
    bool exists{false};               // 이 셀에 와이어가 있는지
    bool hasSignal{false};            // HIGH 신호가 있는지
    
    // 특정 방향으로 연결되어 있는지 확인
    bool hasConnection(WireDirection dir) const {
        return (connections & dir) != WireDirection::None;
    }
    
    // 방향 추가
    void addConnection(WireDirection dir) {
        connections |= dir;
        exists = true;
    }
    
    // 방향 제거
    void removeConnection(WireDirection dir) {
        connections = static_cast<WireDirection>(
            static_cast<uint8_t>(connections) & ~static_cast<uint8_t>(dir)
        );
        if (connections == WireDirection::None) {
            exists = false;
        }
    }
    
    // 셀 중앙 월드 좌표 얻기
    Vec2 getCenterPos() const {
        return Vec2{cellPos.x + 0.5f, cellPos.y + 0.5f};
    }
};