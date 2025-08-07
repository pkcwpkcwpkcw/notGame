#pragma once

#include "InputTypes.h"
#include "CoordinateTransformer.h"
#include "HitDetector.h"
#include "EventDispatcher.h"
#include "DragManager.h"
#include "../render/Camera.h"
#include "../core/Circuit.h"
#include <SDL.h>
#include <unordered_set>
#include <imgui.h>

namespace Input {

class InputManager {
private:
    Camera* m_camera = nullptr;
    Circuit* m_circuit = nullptr;
    
    CoordinateTransformer m_transformer;
    HitDetector m_hitDetector;
    EventDispatcher m_dispatcher;
    DragManager m_dragManager;
    
    struct MouseState {
        glm::vec2 position{0, 0};
        glm::vec2 lastPosition{0, 0};
        glm::vec2 worldPosition{0, 0};
        glm::ivec2 gridPosition{0, 0};
        bool buttons[3] = {false, false, false};
        bool buttonsPressed[3] = {false, false, false};
        bool buttonsReleased[3] = {false, false, false};
        float scrollDelta = 0;
    } m_mouseState;
    
    struct SelectionState {
        std::unordered_set<uint32_t> selectedGates;
        std::unordered_set<uint32_t> selectedWires;
        uint32_t primarySelection = 0;
        ClickTarget selectionType = ClickTarget::None;
    } m_selection;
    
    struct HoverState {
        HitResult current;
        HitResult previous;
        bool changed = false;
    } m_hover;
    
    struct Settings {
        float dragThreshold = 5.0f;
        float doubleClickTime = 0.3f;
        float wireHitThreshold = 0.1f;
        float portHitRadius = 0.2f;
        bool invertScroll = false;
    } m_settings;
    
    uint32_t m_frameNumber = 0;
    uint32_t m_lastClickTime = 0;
    glm::vec2 m_lastClickPos{0, 0};
    int m_lastClickButton = -1;
    
    bool m_debugOverlay = false;
    
public:
    InputManager() {
        m_dragManager.setEventDispatcher(&m_dispatcher);
        m_dragManager.setCoordinateTransformer(&m_transformer);
        m_dragManager.setHitDetector(&m_hitDetector);
        m_dragManager.setDragThreshold(m_settings.dragThreshold);
        
        m_hitDetector.setWireHitThreshold(m_settings.wireHitThreshold);
        m_hitDetector.setPortHitRadius(m_settings.portHitRadius);
    }
    
    void initialize(Camera* camera, Circuit* circuit) {
        m_camera = camera;
        m_circuit = circuit;
        
        m_transformer.setCamera(camera);
        m_hitDetector.setCircuit(circuit);
    }
    
    void setViewport(float width, float height) {
        m_transformer.setViewport(width, height);
    }
    
    void handleEvent(const SDL_Event& event) {
        switch (event.type) {
            case SDL_MOUSEMOTION:
                handleMouseMove(event.motion);
                break;
            case SDL_MOUSEBUTTONDOWN:
                handleMouseDown(event.button);
                break;
            case SDL_MOUSEBUTTONUP:
                handleMouseUp(event.button);
                break;
            case SDL_MOUSEWHEEL:
                handleMouseWheel(event.wheel);
                break;
            case SDL_KEYDOWN:
                handleKeyDown(event.key);
                break;
        }
    }
    
    void update(float deltaTime) {
        m_frameNumber++;
        m_transformer.updateCache(m_frameNumber);
        
        updateMouseState();
        updateHover();
        
        m_dispatcher.processQueue();
    }
    
    template<typename EventType>
    void subscribe(EventCallback<EventType> callback) {
        m_dispatcher.subscribe(callback);
    }
    
    void setOnClick(EventCallback<ClickEvent> callback) {
        m_dispatcher.subscribe(callback);
    }
    
    void setOnDragStart(EventCallback<DragEvent> callback) {
        m_dispatcher.subscribe<DragEvent>([callback](const DragEvent& e) {
            if (e.phase == DragPhase::Start) callback(e);
        });
    }
    
    void setOnDragMove(EventCallback<DragEvent> callback) {
        m_dispatcher.subscribe<DragEvent>([callback](const DragEvent& e) {
            if (e.phase == DragPhase::Move) callback(e);
        });
    }
    
    void setOnDragEnd(EventCallback<DragEvent> callback) {
        m_dispatcher.subscribe<DragEvent>([callback](const DragEvent& e) {
            if (e.phase == DragPhase::End) callback(e);
        });
    }
    
    void setOnHover(EventCallback<HoverEvent> callback) {
        m_dispatcher.subscribe(callback);
    }
    
    bool isMouseDown(int button) const {
        return (button >= 0 && button < 3) ? m_mouseState.buttons[button] : false;
    }
    
    bool isMousePressed(int button) const {
        return (button >= 0 && button < 3) ? m_mouseState.buttonsPressed[button] : false;
    }
    
    bool isMouseReleased(int button) const {
        return (button >= 0 && button < 3) ? m_mouseState.buttonsReleased[button] : false;
    }
    
    bool isDragging() const { return m_dragManager.isDragging(); }
    
    glm::vec2 getMousePos() const { return m_mouseState.position; }
    glm::vec2 getWorldPos() const { return m_mouseState.worldPosition; }
    glm::ivec2 getGridPos() const { return m_mouseState.gridPosition; }
    float getScrollDelta() const { return m_mouseState.scrollDelta; }
    
    const SelectionState& getSelection() const { return m_selection; }
    const HoverState& getHover() const { return m_hover; }
    const HitResult& getLastHit() const { return m_hover.current; }
    
    void clearSelection() {
        m_selection.selectedGates.clear();
        m_selection.selectedWires.clear();
        m_selection.primarySelection = 0;
        m_selection.selectionType = ClickTarget::None;
    }
    
    void selectGate(uint32_t gateId, bool addToSelection = false) {
        if (!addToSelection) {
            clearSelection();
        }
        m_selection.selectedGates.insert(gateId);
        m_selection.primarySelection = gateId;
        m_selection.selectionType = ClickTarget::Gate;
    }
    
    void selectWire(uint32_t wireId, bool addToSelection = false) {
        if (!addToSelection) {
            clearSelection();
        }
        m_selection.selectedWires.insert(wireId);
        m_selection.primarySelection = wireId;
        m_selection.selectionType = ClickTarget::Wire;
    }
    
    bool isSelected(uint32_t id, ClickTarget type) const {
        if (type == ClickTarget::Gate) {
            return m_selection.selectedGates.count(id) > 0;
        } else if (type == ClickTarget::Wire) {
            return m_selection.selectedWires.count(id) > 0;
        }
        return false;
    }
    
    void setDebugOverlay(bool enabled) { m_debugOverlay = enabled; }
    
    void renderDebugOverlay() {
        if (!m_debugOverlay) return;
        
        ImGui::Begin("Input Debug", &m_debugOverlay);
        
        ImGui::Text("Frame: %u", m_frameNumber);
        ImGui::Separator();
        
        ImGui::Text("Mouse Screen: %.1f, %.1f", m_mouseState.position.x, m_mouseState.position.y);
        ImGui::Text("Mouse World: %.2f, %.2f", m_mouseState.worldPosition.x, m_mouseState.worldPosition.y);
        ImGui::Text("Mouse Grid: %d, %d", m_mouseState.gridPosition.x, m_mouseState.gridPosition.y);
        ImGui::Text("Buttons: L:%d M:%d R:%d", 
            m_mouseState.buttons[0], m_mouseState.buttons[1], m_mouseState.buttons[2]);
        
        if (m_mouseState.scrollDelta != 0) {
            ImGui::Text("Scroll: %.2f", m_mouseState.scrollDelta);
        }
        
        ImGui::Separator();
        
        const char* hitTypeStr = "None";
        switch (m_hover.current.type) {
            case ClickTarget::Gate: hitTypeStr = "Gate"; break;
            case ClickTarget::Wire: hitTypeStr = "Wire"; break;
            case ClickTarget::Port: hitTypeStr = "Port"; break;
            case ClickTarget::Empty: hitTypeStr = "Empty"; break;
        }
        ImGui::Text("Hover: %s", hitTypeStr);
        if (m_hover.current.type != ClickTarget::None && m_hover.current.type != ClickTarget::Empty) {
            ImGui::Text("  ID: %u", m_hover.current.objectId);
            ImGui::Text("  Distance: %.3f", m_hover.current.distance);
            if (m_hover.current.type == ClickTarget::Port) {
                ImGui::Text("  Port: %s[%d]", 
                    m_hover.current.isInput ? "Input" : "Output",
                    m_hover.current.portIndex);
            }
        }
        
        ImGui::Separator();
        
        if (m_dragManager.isDragging()) {
            ImGui::Text("Dragging: Yes");
            ImGui::Text("  Distance: %.1f px", m_dragManager.getDragDistance());
            glm::vec2 delta = m_dragManager.getDragDelta();
            ImGui::Text("  Delta: %.2f, %.2f", delta.x, delta.y);
        } else {
            ImGui::Text("Dragging: No");
        }
        
        ImGui::Separator();
        
        ImGui::Text("Selection:");
        ImGui::Text("  Gates: %zu", m_selection.selectedGates.size());
        ImGui::Text("  Wires: %zu", m_selection.selectedWires.size());
        if (m_selection.primarySelection != 0) {
            ImGui::Text("  Primary: %u", m_selection.primarySelection);
        }
        
        ImGui::End();
    }
    
private:
    void handleMouseMove(const SDL_MouseMotionEvent& event) {
        m_mouseState.lastPosition = m_mouseState.position;
        m_mouseState.position = glm::vec2(event.x, event.y);
        
        m_mouseState.worldPosition = m_transformer.screenToWorld(m_mouseState.position);
        m_mouseState.gridPosition = m_transformer.worldToGrid(m_mouseState.worldPosition);
        
        MouseEvent mouseEvent;
        mouseEvent.type = MouseEvent::Move;
        mouseEvent.screenPos = m_mouseState.position;
        mouseEvent.worldPos = m_mouseState.worldPosition;
        mouseEvent.gridPos = m_mouseState.gridPosition;
        mouseEvent.timestamp = SDL_GetTicks();
        
        m_dragManager.onMouseMove(mouseEvent);
    }
    
    void handleMouseDown(const SDL_MouseButtonEvent& event) {
        int button = event.button - 1;
        if (button < 0 || button >= 3) return;
        
        m_mouseState.buttons[button] = true;
        m_mouseState.buttonsPressed[button] = true;
        
        glm::vec2 screenPos(event.x, event.y);
        glm::vec2 worldPos = m_transformer.screenToWorld(screenPos);
        glm::ivec2 gridPos = m_transformer.worldToGrid(worldPos);
        
        uint32_t currentTime = SDL_GetTicks();
        bool isDoubleClick = false;
        
        if (button == m_lastClickButton && 
            (currentTime - m_lastClickTime) < (m_settings.doubleClickTime * 1000) &&
            glm::length(screenPos - m_lastClickPos) < 5.0f) {
            isDoubleClick = true;
        }
        
        m_lastClickTime = currentTime;
        m_lastClickPos = screenPos;
        m_lastClickButton = button;
        
        MouseEvent mouseEvent;
        mouseEvent.type = MouseEvent::Down;
        mouseEvent.button = button;
        mouseEvent.screenPos = screenPos;
        mouseEvent.worldPos = worldPos;
        mouseEvent.gridPos = gridPos;
        mouseEvent.timestamp = currentTime;
        
        m_dragManager.onMouseDown(mouseEvent);
        
        if (isDoubleClick) {
            ClickEvent clickEvent;
            clickEvent.worldPos = worldPos;
            clickEvent.gridPos = gridPos;
            clickEvent.button = button;
            clickEvent.hit = m_hitDetector.detectHit(worldPos);
            clickEvent.doubleClick = true;
            clickEvent.timestamp = currentTime;
            
            m_dispatcher.dispatch(clickEvent);
        }
    }
    
    void handleMouseUp(const SDL_MouseButtonEvent& event) {
        int button = event.button - 1;
        if (button < 0 || button >= 3) return;
        
        m_mouseState.buttons[button] = false;
        m_mouseState.buttonsReleased[button] = true;
        
        glm::vec2 screenPos(event.x, event.y);
        glm::vec2 worldPos = m_transformer.screenToWorld(screenPos);
        glm::ivec2 gridPos = m_transformer.worldToGrid(worldPos);
        
        MouseEvent mouseEvent;
        mouseEvent.type = MouseEvent::Up;
        mouseEvent.button = button;
        mouseEvent.screenPos = screenPos;
        mouseEvent.worldPos = worldPos;
        mouseEvent.gridPos = gridPos;
        mouseEvent.timestamp = SDL_GetTicks();
        
        m_dragManager.onMouseUp(mouseEvent);
    }
    
    void handleMouseWheel(const SDL_MouseWheelEvent& event) {
        m_mouseState.scrollDelta = event.y * (m_settings.invertScroll ? -1 : 1);
        
        MouseEvent mouseEvent;
        mouseEvent.type = MouseEvent::Wheel;
        mouseEvent.wheelDelta = m_mouseState.scrollDelta;
        mouseEvent.timestamp = SDL_GetTicks();
        
        m_dispatcher.dispatch(mouseEvent);
    }
    
    void handleKeyDown(const SDL_KeyboardEvent& event) {
        if (event.keysym.sym == SDLK_ESCAPE) {
            if (m_dragManager.isDragging()) {
                m_dragManager.cancelDrag();
            } else {
                clearSelection();
            }
        }
    }
    
    void updateMouseState() {
        for (int i = 0; i < 3; i++) {
            m_mouseState.buttonsPressed[i] = false;
            m_mouseState.buttonsReleased[i] = false;
        }
        m_mouseState.scrollDelta = 0;
    }
    
    void updateHover() {
        m_hover.previous = m_hover.current;
        m_hover.current = m_hitDetector.detectHit(m_mouseState.worldPosition);
        
        m_hover.changed = (m_hover.current.type != m_hover.previous.type ||
                          m_hover.current.objectId != m_hover.previous.objectId);
        
        if (m_hover.changed) {
            HoverEvent hoverEvent;
            hoverEvent.worldPos = m_mouseState.worldPosition;
            hoverEvent.gridPos = m_mouseState.gridPosition;
            hoverEvent.hit = m_hover.current;
            hoverEvent.previousHit = m_hover.previous;
            
            m_dispatcher.dispatch(hoverEvent);
        }
    }
};

} // namespace Input