#pragma once

#include <vector>
#include <memory>
#include <cstdint>

namespace notgame {
namespace core {

class Gate;
class Wire;
class Signal;

class Circuit {
public:
    Circuit();
    ~Circuit();

    // Circuit management
    void clear();
    void reset();
    
    // Gate operations
    void addGate(std::shared_ptr<Gate> gate);
    void removeGate(std::shared_ptr<Gate> gate);
    
    // Wire operations  
    void addWire(std::shared_ptr<Wire> wire);
    void removeWire(std::shared_ptr<Wire> wire);
    
    // Simulation
    void update(float deltaTime);
    bool isStable() const;
    
    // Getters
    size_t getGateCount() const { return gates_.size(); }
    size_t getWireCount() const { return wires_.size(); }

private:
    std::vector<std::shared_ptr<Gate>> gates_;
    std::vector<std::shared_ptr<Wire>> wires_;
    
    bool stable_;
    float simulationTime_;
};

} // namespace core
} // namespace notgame