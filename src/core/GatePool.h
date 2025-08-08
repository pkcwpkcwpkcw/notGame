#pragma once
#include "Types.h"
#include "Gate.h"
#include <vector>
#include <memory>
#include <bitset>
#include <stack>

class GatePool {
private:
    static constexpr size_t BLOCK_SIZE = 1000;
    static constexpr size_t MAX_BLOCKS = 1000;  // Maximum 1,000,000 gates
    
    struct Block {
        alignas(64) std::array<Gate, BLOCK_SIZE> gates;
        std::bitset<BLOCK_SIZE> used;
        uint32_t freeCount;
        
        Block() : freeCount(BLOCK_SIZE) {
            used.reset();
            for (size_t i = 0; i < BLOCK_SIZE; ++i) {
                gates[i].id = Constants::INVALID_GATE_ID;
            }
        }
    };
    
    std::vector<std::unique_ptr<Block>> blocks;
    std::stack<GateId> freeList;
    GateId nextId{1};
    size_t usedCount{0};
    
public:
    GatePool();
    ~GatePool() = default;
    
    GatePool(const GatePool&) = delete;
    GatePool& operator=(const GatePool&) = delete;
    GatePool(GatePool&&) = default;
    GatePool& operator=(GatePool&&) = default;
    
    [[nodiscard]] Gate* allocate() noexcept;
    void deallocate(GateId id) noexcept;
    [[nodiscard]] Gate* getGate(GateId id) noexcept;
    [[nodiscard]] const Gate* getGate(GateId id) const noexcept;
    
    [[nodiscard]] size_t getUsedCount() const noexcept { return usedCount; }
    [[nodiscard]] size_t getCapacity() const noexcept { return blocks.size() * BLOCK_SIZE; }
    [[nodiscard]] bool isFull() const noexcept { return usedCount >= MAX_BLOCKS * BLOCK_SIZE; }
    
    void clear() noexcept;
    
private:
    [[nodiscard]] std::pair<size_t, size_t> idToBlockIndex(GateId id) const noexcept;
    [[nodiscard]] GateId blockIndexToId(size_t blockIdx, size_t gateIdx) const noexcept;
    void expandPool();
};