#include "core/Wire.h"
#include <algorithm>

namespace notgame {
namespace core {

Wire::Wire() 
    : signal_(false) {
}

Wire::~Wire() {
}

void Wire::addSegment(int x, int y) {
    // Check if segment already exists
    auto it = std::find_if(segments_.begin(), segments_.end(),
        [x, y](const WireSegment& seg) {
            return seg.x == x && seg.y == y;
        });
    
    if (it == segments_.end()) {
        segments_.emplace_back(x, y);
        updateConnections();
    }
}

void Wire::removeSegment(int x, int y) {
    auto it = std::find_if(segments_.begin(), segments_.end(),
        [x, y](const WireSegment& seg) {
            return seg.x == x && seg.y == y;
        });
    
    if (it != segments_.end()) {
        segments_.erase(it);
        updateConnections();
    }
}

void Wire::clear() {
    segments_.clear();
    signal_ = false;
}

void Wire::setSignal(bool value) {
    signal_ = value;
}

bool Wire::isConnected(int x, int y) const {
    return std::any_of(segments_.begin(), segments_.end(),
        [x, y](const WireSegment& seg) {
            return seg.x == x && seg.y == y;
        });
}

void Wire::updateConnections() {
    for (auto& segment : segments_) {
        updateSegmentConnections(segment);
    }
}

void Wire::updateSegmentConnections(WireSegment& segment) {
    segment.connections = WireSegment::NONE;
    
    // Check for adjacent segments
    if (isConnected(segment.x, segment.y - 1)) {
        segment.connections |= WireSegment::UP;
    }
    if (isConnected(segment.x, segment.y + 1)) {
        segment.connections |= WireSegment::DOWN;
    }
    if (isConnected(segment.x - 1, segment.y)) {
        segment.connections |= WireSegment::LEFT;
    }
    if (isConnected(segment.x + 1, segment.y)) {
        segment.connections |= WireSegment::RIGHT;
    }
}

} // namespace core
} // namespace notgame