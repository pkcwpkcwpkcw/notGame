#pragma once

#include "core/Types.h"
#include "core/Vec2.h"
#include "core/Wire.h"
#include "core/Circuit.h"
#include "input/InputTypes.h"
#include <memory>
#include <vector>
#include <unordered_set>
#include <functional>


class WirePreviewSystem;
class PortHighlightSystem;
class WirePathCalculator;
class ConnectionValidator;

enum class WireConnectionState {
    Idle,
    Connecting,
    Previewing,
    Validating
};

struct WireConnectionContext {
    WireConnectionState state{WireConnectionState::Idle};
    
    GateId sourceGateId{Constants::INVALID_GATE_ID};
    PortIndex sourcePort{Constants::INVALID_PORT};
    Vec2 sourcePos{0, 0};
    
    GateId targetGateId{Constants::INVALID_GATE_ID};
    PortIndex targetPort{Constants::INVALID_PORT};
    Vec2 targetPos{0, 0};
    
    Vec2 currentMousePos{0, 0};
    std::vector<Vec2> previewPath;
    
    bool isValid{false};
    ErrorCode validationError{ErrorCode::SUCCESS};
};

class WireManager {
public:
    WireManager(Circuit* circuit);
    ~WireManager();
    
    WireManager(const WireManager&) = delete;
    WireManager& operator=(const WireManager&) = delete;
    
    void initialize() noexcept;
    void shutdown() noexcept;
    
    void onDragStart(const Input::DragEvent& event) noexcept;
    void onDragMove(const Input::DragEvent& event) noexcept;
    void onDragEnd(const Input::DragEvent& event) noexcept;
    void onDragCancel(const Input::DragEvent& event) noexcept;
    
    void onMouseMove(const Vec2& worldPos) noexcept;
    void onClick(const Input::ClickEvent& event) noexcept;
    
    [[nodiscard]] Result<WireId> createWire(
        GateId fromGate, PortIndex fromPort,
        GateId toGate, PortIndex toPort) noexcept;
    
    ErrorCode deleteWire(WireId wireId) noexcept;
    ErrorCode deleteWiresAt(Vec2 position, float tolerance = 0.1f) noexcept;
    ErrorCode deleteWiresForGate(GateId gateId) noexcept;
    
    void startWireConnection(GateId gateId, PortIndex port, Vec2 startPos) noexcept;
    void updateWirePreview(Vec2 currentPos) noexcept;
    void completeWireConnection(GateId targetGate, PortIndex targetPort) noexcept;
    void cancelWireConnection() noexcept;
    
    [[nodiscard]] bool isConnecting() const noexcept {
        return m_context.state == WireConnectionState::Connecting ||
               m_context.state == WireConnectionState::Previewing;
    }
    
    [[nodiscard]] const WireConnectionContext& getContext() const noexcept {
        return m_context;
    }
    
    [[nodiscard]] const std::vector<Vec2>& getPreviewPath() const noexcept {
        return m_context.previewPath;
    }
    
    [[nodiscard]] std::vector<std::pair<GateId, PortIndex>> getHighlightedPorts() const noexcept;
    
    [[nodiscard]] bool canConnect(
        GateId fromGate, PortIndex fromPort,
        GateId toGate, PortIndex toPort) const noexcept;
    
    void setHighlightRadius(float radius) noexcept { m_highlightRadius = radius; }
    void setSnapDistance(float distance) noexcept { m_snapDistance = distance; }
    void setPathSmoothing(bool enable) noexcept { m_enablePathSmoothing = enable; }
    
    using WireCreatedCallback = std::function<void(WireId)>;
    using WireDeletedCallback = std::function<void(WireId)>;
    using ConnectionStateChangedCallback = std::function<void(WireConnectionState)>;
    
    void setWireCreatedCallback(WireCreatedCallback cb) { m_onWireCreated = cb; }
    void setWireDeletedCallback(WireDeletedCallback cb) { m_onWireDeleted = cb; }
    void setConnectionStateChangedCallback(ConnectionStateChangedCallback cb) {
        m_onStateChanged = cb;
    }
    
private:
    void updateState(WireConnectionState newState) noexcept;
    void updateHighlightedPorts() noexcept;
    void calculatePreviewPath() noexcept;
    ErrorCode validateConnection(
        GateId fromGate, PortIndex fromPort,
        GateId toGate, PortIndex toPort) const noexcept;
    
    Vec2 getPortPosition(GateId gateId, PortIndex port) const noexcept;
    bool isPortAvailable(GateId gateId, PortIndex port) const noexcept;
    
    std::pair<GateId, PortIndex> findNearestPort(
        Vec2 position, bool inputPorts) const noexcept;
    void createCellToCellWire(Vec2 startPos, Vec2 endPos) noexcept;
    void createPathWire(const std::vector<Vec2>& path) noexcept;
    std::vector<glm::ivec2> bresenhamLine(glm::ivec2 start, glm::ivec2 end) const noexcept;
    
    Circuit* m_circuit;
    WireConnectionContext m_context;
    
    std::unique_ptr<WirePreviewSystem> m_previewSystem;
    std::unique_ptr<PortHighlightSystem> m_highlightSystem;
    std::unique_ptr<WirePathCalculator> m_pathCalculator;
    std::unique_ptr<ConnectionValidator> m_validator;
    
    struct PairHash {
        size_t operator()(const std::pair<GateId, PortIndex>& p) const noexcept {
            return std::hash<GateId>{}(p.first) ^ (std::hash<PortIndex>{}(p.second) << 1);
        }
    };
    
    std::unordered_set<std::pair<GateId, PortIndex>, PairHash> m_highlightedPorts;
    
    float m_highlightRadius{50.0f};
    float m_snapDistance{10.0f};
    bool m_enablePathSmoothing{true};
    
    WireCreatedCallback m_onWireCreated;
    WireDeletedCallback m_onWireDeleted;
    ConnectionStateChangedCallback m_onStateChanged;
    
    // Drag path tracking
    std::vector<Vec2> m_dragPath;
    glm::ivec2 m_lastGridPos;
};