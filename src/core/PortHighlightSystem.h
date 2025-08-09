#pragma once

#include "core/Types.h"
#include "core/Vec2.h"
#include <vector>
#include <unordered_map>
#include <functional>

class Circuit;

enum class HighlightType {
    None,
    Available,
    Compatible,
    Incompatible,
    Connected,
    Hover
};

struct PortHighlight {
    GateId gateId{Constants::INVALID_GATE_ID};
    PortIndex portIndex{Constants::INVALID_PORT};
    Vec2 position{0, 0};
    HighlightType type{HighlightType::None};
    float intensity{0.0f};
    float pulsePhase{0.0f};
};

class PortHighlightSystem {
public:
    explicit PortHighlightSystem(Circuit* circuit);
    ~PortHighlightSystem() = default;
    
    void startHighlighting(GateId sourceGate, PortIndex sourcePort) noexcept;
    void updateHighlights(Vec2 mousePos) noexcept;
    void stopHighlighting() noexcept;
    
    void highlightPort(GateId gateId, PortIndex port, HighlightType type) noexcept;
    void clearPortHighlight(GateId gateId, PortIndex port) noexcept;
    void clearAllHighlights() noexcept;
    
    [[nodiscard]] bool isHighlighted(GateId gateId, PortIndex port) const noexcept;
    [[nodiscard]] HighlightType getHighlightType(GateId gateId, PortIndex port) const noexcept;
    [[nodiscard]] const std::vector<PortHighlight>& getHighlights() const noexcept { 
        return m_activeHighlights; 
    }
    
    void setHighlightRadius(float radius) noexcept { m_highlightRadius = radius; }
    void setPulseEnabled(bool enable) noexcept { m_pulseEnabled = enable; }
    void setPulseSpeed(float speed) noexcept { m_pulseSpeed = speed; }
    void setFadeSpeed(float speed) noexcept { m_fadeSpeed = speed; }
    
    void update(float deltaTime) noexcept;
    
    using PortHoverCallback = std::function<void(GateId, PortIndex)>;
    void setPortHoverCallback(PortHoverCallback cb) { m_onPortHover = cb; }
    
private:
    void updateCompatibility() noexcept;
    void updateProximity(Vec2 mousePos) noexcept;
    void updateAnimations(float deltaTime) noexcept;
    
    [[nodiscard]] bool isPortCompatible(GateId gateId, PortIndex port) const noexcept;
    [[nodiscard]] float calculateDistance(Vec2 mousePos, GateId gateId, PortIndex port) const noexcept;
    
    Circuit* m_circuit;
    std::vector<PortHighlight> m_activeHighlights;
    struct PairHash {
        size_t operator()(const std::pair<GateId, PortIndex>& p) const noexcept {
            return std::hash<GateId>{}(p.first) ^ (std::hash<PortIndex>{}(p.second) << 1);
        }
    };
    
    std::unordered_map<std::pair<GateId, PortIndex>, size_t, PairHash> m_highlightMap;
    
    GateId m_sourceGate{Constants::INVALID_GATE_ID};
    PortIndex m_sourcePort{Constants::INVALID_PORT};
    bool m_isHighlighting{false};
    
    float m_highlightRadius{50.0f};
    bool m_pulseEnabled{true};
    float m_pulseSpeed{2.0f};
    float m_fadeSpeed{5.0f};
    
    PortHoverCallback m_onPortHover;
};