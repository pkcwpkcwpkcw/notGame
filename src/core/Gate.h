#pragma once

#include <array>
#include <memory>
#include <cstdint>

namespace notgame {
namespace core {

class Signal;

class Gate {
public:
    static constexpr size_t NUM_INPUTS = 3;
    static constexpr float PROPAGATION_DELAY = 0.1f; // 0.1 second delay
    
    Gate(int x, int y);
    ~Gate();
    
    // Position
    int getX() const { return x_; }
    int getY() const { return y_; }
    void setPosition(int x, int y);
    
    // Input/Output
    void setInput(size_t index, bool value);
    bool getOutput() const { return output_; }
    
    // Update
    void update(float deltaTime);
    
    // Connection management
    bool connectInput(size_t index, std::shared_ptr<Signal> signal);
    void disconnectInput(size_t index);
    
private:
    int x_, y_;  // Grid position
    
    std::array<bool, NUM_INPUTS> inputs_;
    bool output_;
    bool pendingOutput_;
    float outputDelay_;
    
    std::array<std::shared_ptr<Signal>, NUM_INPUTS> inputConnections_;
};

} // namespace core
} // namespace notgame