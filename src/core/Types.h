#pragma once
#include <cstdint>
#include <limits>

using GateId = uint32_t;
using WireId = uint32_t;
using PortIndex = int8_t;

namespace Constants {
    constexpr GateId INVALID_GATE_ID = 0;
    constexpr WireId INVALID_WIRE_ID = 0;
    constexpr PortIndex INVALID_PORT = -2;
    constexpr PortIndex OUTPUT_PORT = -1;
    
    constexpr float GATE_DELAY = 0.1f;
    constexpr int MAX_INPUT_PORTS = 3;
    constexpr float GRID_CELL_SIZE = 32.0f;
    
    constexpr size_t GATE_POOL_SIZE = 10000;
    constexpr size_t WIRE_POOL_SIZE = 50000;
    constexpr size_t CACHE_LINE_SIZE = 64;
}

enum class GateType : uint8_t {
    NOT = 0,
};

enum class SignalState : uint8_t {
    LOW = 0,
    HIGH = 1,
    UNDEFINED = 2,
    FLOATING = 3
};

enum class ErrorCode : int32_t {
    SUCCESS = 0,
    INVALID_ID = -1,
    POSITION_OCCUPIED = -2,
    PORT_ALREADY_CONNECTED = -3,
    CIRCULAR_DEPENDENCY = -4,
    OUT_OF_BOUNDS = -5,
    OUT_OF_MEMORY = -6,
    NOT_INITIALIZED = -7,
    INVALID_POSITION = -8
};

enum class MouseButton : uint8_t {
    Left = 0,
    Middle = 1,
    Right = 2
};

enum class Key : uint16_t {
    Unknown = 0,
    A = 'A',
    B = 'B',
    C = 'C',
    D = 'D',
    E = 'E',
    F = 'F',
    G = 'G',
    H = 'H',
    I = 'I',
    J = 'J',
    K = 'K',
    L = 'L',
    M = 'M',
    N = 'N',
    O = 'O',
    P = 'P',
    Q = 'Q',
    R = 'R',
    S = 'S',
    T = 'T',
    U = 'U',
    V = 'V',
    W = 'W',
    X = 'X',
    Y = 'Y',
    Z = 'Z',
    Delete = 127,
    Backspace = 8,
    Escape = 27,
    LeftShift = 256,
    RightShift = 257,
    LeftCtrl = 258,
    RightCtrl = 259,
    LeftAlt = 260,
    RightAlt = 261
};

template<typename T>
struct Result {
    T value;
    ErrorCode error;
    
    Result() : value{}, error(ErrorCode::SUCCESS) {}
    Result(T val) : value(val), error(ErrorCode::SUCCESS) {}
    Result(T val, ErrorCode err) : value(val), error(err) {}
    
    [[nodiscard]] bool success() const noexcept { 
        return error == ErrorCode::SUCCESS; 
    }
    [[nodiscard]] bool isOk() const noexcept { 
        return error == ErrorCode::SUCCESS; 
    }
    [[nodiscard]] bool isError() const noexcept { 
        return error != ErrorCode::SUCCESS; 
    }
    
    [[nodiscard]] T& operator*() { return value; }
    [[nodiscard]] const T& operator*() const { return value; }
    [[nodiscard]] T* operator->() { return &value; }
    [[nodiscard]] const T* operator->() const { return &value; }
};