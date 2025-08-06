#include "core/Circuit.h"
#include "core/Gate.h"
#include "core/Wire.h"

namespace notgame {
namespace core {

Circuit::Circuit() 
    : stable_(true)
    , simulationTime_(0.0f) {
}

Circuit::~Circuit() {
}

void Circuit::clear() {
    gates_.clear();
    wires_.clear();
    stable_ = true;
    simulationTime_ = 0.0f;
}

void Circuit::reset() {
    simulationTime_ = 0.0f;
    stable_ = true;
}

void Circuit::addGate(std::shared_ptr<Gate> gate) {
    if (gate) {
        gates_.push_back(gate);
        stable_ = false;
    }
}

void Circuit::removeGate(std::shared_ptr<Gate> gate) {
    auto it = std::find(gates_.begin(), gates_.end(), gate);
    if (it != gates_.end()) {
        gates_.erase(it);
        stable_ = false;
    }
}

void Circuit::addWire(std::shared_ptr<Wire> wire) {
    if (wire) {
        wires_.push_back(wire);
        stable_ = false;
    }
}

void Circuit::removeWire(std::shared_ptr<Wire> wire) {
    auto it = std::find(wires_.begin(), wires_.end(), wire);
    if (it != wires_.end()) {
        wires_.erase(it);
        stable_ = false;
    }
}

void Circuit::update(float deltaTime) {
    simulationTime_ += deltaTime;
    
    // Update all gates
    for (auto& gate : gates_) {
        gate->update(deltaTime);
    }
    
    // Check for stability (simplified for now)
    stable_ = true;
}

bool Circuit::isStable() const {
    return stable_;
}

} // namespace core
} // namespace notgame