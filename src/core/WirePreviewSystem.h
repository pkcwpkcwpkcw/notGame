#pragma once

#include "core/Types.h"
#include "core/Vec2.h"
#include <vector>
#include <memory>

class Circuit;

enum class PreviewState {
    Inactive,
    Active,
    Valid,
    Invalid
};

struct WirePreview {
    Vec2 startPos{0, 0};
    Vec2 endPos{0, 0};
    std::vector<Vec2> pathPoints;
    PreviewState state{PreviewState::Inactive};
    bool isSnapped{false};
    float opacity{1.0f};
};

class WirePreviewSystem {
public:
    explicit WirePreviewSystem(Circuit* circuit);
    ~WirePreviewSystem() = default;
    
    void startPreview(Vec2 startPos, GateId sourceGate, PortIndex sourcePort) noexcept;
    void updatePreview(Vec2 currentPos, GateId targetGate = Constants::INVALID_GATE_ID, 
                      PortIndex targetPort = Constants::INVALID_PORT) noexcept;
    void endPreview() noexcept;
    void cancelPreview() noexcept;
    
    [[nodiscard]] bool isActive() const noexcept { 
        return m_preview.state != PreviewState::Inactive; 
    }
    
    [[nodiscard]] const WirePreview& getPreview() const noexcept { 
        return m_preview; 
    }
    
    [[nodiscard]] const std::vector<Vec2>& getPath() const noexcept { 
        return m_preview.pathPoints; 
    }
    
    [[nodiscard]] PreviewState getState() const noexcept { 
        return m_preview.state; 
    }
    
    void setValidationResult(bool isValid) noexcept {
        m_preview.state = isValid ? PreviewState::Valid : PreviewState::Invalid;
    }
    
    void setSnapEnabled(bool enable) noexcept { m_snapEnabled = enable; }
    void setSnapDistance(float distance) noexcept { m_snapDistance = distance; }
    void setAnimationEnabled(bool enable) noexcept { m_animationEnabled = enable; }
    
    void update(float deltaTime) noexcept;
    
private:
    void calculatePath() noexcept;
    Vec2 snapToGrid(Vec2 pos) const noexcept;
    Vec2 snapToPort(Vec2 pos, GateId gateId, PortIndex port) const noexcept;
    
    Circuit* m_circuit;
    WirePreview m_preview;
    
    GateId m_sourceGate{Constants::INVALID_GATE_ID};
    PortIndex m_sourcePort{Constants::INVALID_PORT};
    GateId m_targetGate{Constants::INVALID_GATE_ID};
    PortIndex m_targetPort{Constants::INVALID_PORT};
    
    bool m_snapEnabled{true};
    float m_snapDistance{10.0f};
    bool m_animationEnabled{true};
    float m_animationTime{0.0f};
};