#pragma once

#include "core/Types.h"
#include <vector>
#include <unordered_set>

class Circuit;

struct ValidationResult {
    bool isValid{false};
    ErrorCode errorCode{ErrorCode::SUCCESS};
    const char* errorMessage{nullptr};
    
    static ValidationResult Valid() {
        return ValidationResult{true, ErrorCode::SUCCESS, nullptr};
    }
    
    static ValidationResult Invalid(ErrorCode code, const char* msg) {
        return ValidationResult{false, code, msg};
    }
};

class ConnectionValidator {
public:
    explicit ConnectionValidator(Circuit* circuit);
    ~ConnectionValidator() = default;
    
    [[nodiscard]] ValidationResult validateConnection(
        GateId fromGate, PortIndex fromPort,
        GateId toGate, PortIndex toPort) const noexcept;
    
    [[nodiscard]] bool canConnect(
        GateId fromGate, PortIndex fromPort,
        GateId toGate, PortIndex toPort) const noexcept;
    
    [[nodiscard]] bool isPortAvailable(
        GateId gateId, PortIndex port) const noexcept;
    
    [[nodiscard]] bool wouldCreateCycle(
        GateId fromGate, GateId toGate) const noexcept;
    
    [[nodiscard]] bool isValidPort(
        GateId gateId, PortIndex port) const noexcept;
    
    [[nodiscard]] bool areGatesAdjacent(
        GateId gate1, GateId gate2) const noexcept;
    
    [[nodiscard]] size_t getConnectionCount(
        GateId gateId) const noexcept;
    
    [[nodiscard]] std::vector<WireId> getIncomingWires(
        GateId gateId) const noexcept;
    
    [[nodiscard]] std::vector<WireId> getOutgoingWires(
        GateId gateId) const noexcept;
    
    void setAllowMultipleOutputs(bool allow) noexcept { 
        m_allowMultipleOutputs = allow; 
    }
    
    void setAllowSelfConnection(bool allow) noexcept { 
        m_allowSelfConnection = allow; 
    }
    
    void setMaxWireLength(float maxLength) noexcept { 
        m_maxWireLength = maxLength; 
    }
    
private:
    [[nodiscard]] bool detectCycleDFS(
        GateId current, GateId target,
        std::unordered_set<GateId>& visited,
        std::unordered_set<GateId>& recursionStack) const noexcept;
    
    [[nodiscard]] float calculateDistance(
        GateId gate1, GateId gate2) const noexcept;
    
    Circuit* m_circuit;
    bool m_allowMultipleOutputs{true};
    bool m_allowSelfConnection{false};
    float m_maxWireLength{1000.0f};
};