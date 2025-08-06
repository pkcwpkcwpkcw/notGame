#pragma once

#include <vector>
#include <memory>

namespace notgame {
namespace core {

class Signal;

struct WireSegment {
    int x, y;
    enum Direction {
        NONE = 0,
        UP = 1,
        DOWN = 2,
        LEFT = 4,
        RIGHT = 8
    };
    int connections;  // Bitmask of directions
    
    WireSegment(int x_, int y_) : x(x_), y(y_), connections(NONE) {}
};

class Wire {
public:
    Wire();
    ~Wire();
    
    // Wire construction
    void addSegment(int x, int y);
    void removeSegment(int x, int y);
    void clear();
    
    // Signal propagation
    void setSignal(bool value);
    bool getSignal() const { return signal_; }
    
    // Connectivity
    bool isConnected(int x, int y) const;
    void updateConnections();
    
    // Getters
    const std::vector<WireSegment>& getSegments() const { return segments_; }
    
private:
    std::vector<WireSegment> segments_;
    bool signal_;
    
    void updateSegmentConnections(WireSegment& segment);
};

} // namespace core
} // namespace notgame