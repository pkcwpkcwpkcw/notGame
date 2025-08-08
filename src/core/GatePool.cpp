#include "GatePool.h"
#include <cassert>

GatePool::GatePool() {
    blocks.reserve(10);
    expandPool();
}

Gate* GatePool::allocate() noexcept {
    if (isFull()) {
        return nullptr;
    }
    
    GateId id;
    
    if (!freeList.empty()) {
        id = freeList.top();
        freeList.pop();
    } else {
        if (usedCount >= getCapacity()) {
            expandPool();
        }
        id = nextId++;
    }
    
    auto [blockIdx, gateIdx] = idToBlockIndex(id);
    
    if (blockIdx >= blocks.size()) {
        while (blocks.size() <= blockIdx && blocks.size() < MAX_BLOCKS) {
            expandPool();
        }
        if (blockIdx >= blocks.size()) {
            return nullptr;
        }
    }
    
    Block* block = blocks[blockIdx].get();
    Gate* gate = &block->gates[gateIdx];
    
    gate->id = id;
    gate->type = GateType::NOT;
    gate->position = Vec2{0, 0};
    gate->inputWires = {Constants::INVALID_WIRE_ID, Constants::INVALID_WIRE_ID, Constants::INVALID_WIRE_ID};
    gate->outputWire = Constants::INVALID_WIRE_ID;
    gate->currentOutput = SignalState::LOW;
    gate->pendingOutput = SignalState::LOW;
    gate->delayTimer = 0.0f;
    gate->isDirty = false;
    gate->isDelayActive = false;
    gate->isSelected = false;
    gate->isHovered = false;
    
    block->used.set(gateIdx);
    block->freeCount--;
    usedCount++;
    
    return gate;
}

void GatePool::deallocate(GateId id) noexcept {
    if (id == Constants::INVALID_GATE_ID) {
        return;
    }
    
    auto [blockIdx, gateIdx] = idToBlockIndex(id);
    
    if (blockIdx >= blocks.size()) {
        return;
    }
    
    Block* block = blocks[blockIdx].get();
    
    if (!block->used.test(gateIdx)) {
        return;
    }
    
    block->gates[gateIdx].id = Constants::INVALID_GATE_ID;
    block->used.reset(gateIdx);
    block->freeCount++;
    usedCount--;
    
    freeList.push(id);
}

Gate* GatePool::getGate(GateId id) noexcept {
    if (id == Constants::INVALID_GATE_ID) {
        return nullptr;
    }
    
    auto [blockIdx, gateIdx] = idToBlockIndex(id);
    
    if (blockIdx >= blocks.size()) {
        return nullptr;
    }
    
    Block* block = blocks[blockIdx].get();
    
    if (!block->used.test(gateIdx)) {
        return nullptr;
    }
    
    return &block->gates[gateIdx];
}

const Gate* GatePool::getGate(GateId id) const noexcept {
    if (id == Constants::INVALID_GATE_ID) {
        return nullptr;
    }
    
    auto [blockIdx, gateIdx] = idToBlockIndex(id);
    
    if (blockIdx >= blocks.size()) {
        return nullptr;
    }
    
    const Block* block = blocks[blockIdx].get();
    
    if (!block->used.test(gateIdx)) {
        return nullptr;
    }
    
    return &block->gates[gateIdx];
}

void GatePool::clear() noexcept {
    for (auto& block : blocks) {
        block->used.reset();
        block->freeCount = BLOCK_SIZE;
        for (size_t i = 0; i < BLOCK_SIZE; ++i) {
            block->gates[i].id = Constants::INVALID_GATE_ID;
        }
    }
    
    while (!freeList.empty()) {
        freeList.pop();
    }
    
    usedCount = 0;
    nextId = 1;
}

std::pair<size_t, size_t> GatePool::idToBlockIndex(GateId id) const noexcept {
    size_t adjustedId = static_cast<size_t>(id - 1);
    size_t blockIdx = adjustedId / BLOCK_SIZE;
    size_t gateIdx = adjustedId % BLOCK_SIZE;
    return {blockIdx, gateIdx};
}

GateId GatePool::blockIndexToId(size_t blockIdx, size_t gateIdx) const noexcept {
    return static_cast<GateId>(blockIdx * BLOCK_SIZE + gateIdx + 1);
}

void GatePool::expandPool() {
    if (blocks.size() >= MAX_BLOCKS) {
        return;
    }
    
    blocks.push_back(std::make_unique<Block>());
}