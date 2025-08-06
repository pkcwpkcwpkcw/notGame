#include "core/Gate.h"

namespace notgame {
namespace core {

Gate::Gate(int x, int y)
    : x_(x)
    , y_(y)
    , output_(false)
    , pendingOutput_(false)
    , outputDelay_(0.0f) {
    for (auto& input : inputs_) {
        input = false;
    }
}

Gate::~Gate() {
}

void Gate::setPosition(int x, int y) {
    x_ = x;
    y_ = y;
}

void Gate::setInput(size_t index, bool value) {
    if (index < NUM_INPUTS) {
        inputs_[index] = value;
        
        // Calculate NOT gate output (all inputs ORed then NOTed)
        bool orResult = false;
        for (const auto& input : inputs_) {
            orResult = orResult || input;
        }
        pendingOutput_ = !orResult;
        outputDelay_ = PROPAGATION_DELAY;
    }
}

void Gate::update(float deltaTime) {
    if (outputDelay_ > 0.0f) {
        outputDelay_ -= deltaTime;
        if (outputDelay_ <= 0.0f) {
            output_ = pendingOutput_;
            outputDelay_ = 0.0f;
        }
    }
}

bool Gate::connectInput(size_t index, std::shared_ptr<Signal> signal) {
    if (index < NUM_INPUTS) {
        inputConnections_[index] = signal;
        return true;
    }
    return false;
}

void Gate::disconnectInput(size_t index) {
    if (index < NUM_INPUTS) {
        inputConnections_[index].reset();
    }
}

} // namespace core
} // namespace notgame