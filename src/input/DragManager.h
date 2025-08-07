#pragma once

#include "InputTypes.h"
#include "EventDispatcher.h"
#include "CoordinateTransformer.h"
#include "HitDetector.h"
#include <glm/glm.hpp>
#include <SDL.h>

namespace Input {

class DragManager {
private:
    enum State { Idle, Potential, Active };
    
    struct DragInfo {
        State state = Idle;
        glm::vec2 startScreenPos{0, 0};
        glm::vec2 currentScreenPos{0, 0};
        glm::vec2 lastScreenPos{0, 0};
        glm::vec2 startWorldPos{0, 0};
        glm::vec2 currentWorldPos{0, 0};
        glm::ivec2 startGridPos{0, 0};
        glm::ivec2 currentGridPos{0, 0};
        HitResult target;
        float accumDistance = 0;
        uint32_t startTime = 0;
        int button = -1;
    };
    
    DragInfo m_dragInfo;
    EventDispatcher* m_dispatcher = nullptr;
    CoordinateTransformer* m_transformer = nullptr;
    HitDetector* m_hitDetector = nullptr;
    
    float m_dragThreshold = 5.0f;  
    bool m_isDragging = false;
    
public:
    DragManager() = default;
    
    void setEventDispatcher(EventDispatcher* dispatcher) {
        m_dispatcher = dispatcher;
    }
    
    void setCoordinateTransformer(CoordinateTransformer* transformer) {
        m_transformer = transformer;
    }
    
    void setHitDetector(HitDetector* detector) {
        m_hitDetector = detector;
    }
    
    void setDragThreshold(float threshold) {
        m_dragThreshold = threshold;
    }
    
    void onMouseDown(const MouseEvent& event) {
        if (m_dragInfo.state != Idle) return;
        
        m_dragInfo.state = Potential;
        m_dragInfo.startScreenPos = event.screenPos;
        m_dragInfo.currentScreenPos = event.screenPos;
        m_dragInfo.lastScreenPos = event.screenPos;
        m_dragInfo.startWorldPos = event.worldPos;
        m_dragInfo.currentWorldPos = event.worldPos;
        m_dragInfo.startGridPos = event.gridPos;
        m_dragInfo.currentGridPos = event.gridPos;
        m_dragInfo.button = event.button;
        m_dragInfo.startTime = event.timestamp;
        m_dragInfo.accumDistance = 0;
        
        if (m_hitDetector) {
            m_dragInfo.target = m_hitDetector->detectHit(event.worldPos);
        }
    }
    
    void onMouseMove(const MouseEvent& event) {
        if (m_dragInfo.state == Idle) return;
        
        m_dragInfo.lastScreenPos = m_dragInfo.currentScreenPos;
        m_dragInfo.currentScreenPos = event.screenPos;
        m_dragInfo.currentWorldPos = event.worldPos;
        m_dragInfo.currentGridPos = event.gridPos;
        
        if (m_dragInfo.state == Potential) {
            float distance = glm::length(event.screenPos - m_dragInfo.startScreenPos);
            m_dragInfo.accumDistance = distance;
            
            if (distance > m_dragThreshold) {
                m_dragInfo.state = Active;
                m_isDragging = true;
                
                if (m_dispatcher) {
                    DragEvent dragEvent;
                    dragEvent.phase = DragPhase::Start;
                    dragEvent.startWorld = m_dragInfo.startWorldPos;
                    dragEvent.currentWorld = m_dragInfo.currentWorldPos;
                    dragEvent.deltaWorld = m_dragInfo.currentWorldPos - m_dragInfo.startWorldPos;
                    dragEvent.startGrid = m_dragInfo.startGridPos;
                    dragEvent.currentGrid = m_dragInfo.currentGridPos;
                    dragEvent.dragTarget = m_dragInfo.target;
                    dragEvent.distance = distance;
                    dragEvent.duration = (event.timestamp - m_dragInfo.startTime) / 1000.0f;
                    dragEvent.button = m_dragInfo.button;
                    
                    m_dispatcher->dispatch(dragEvent);
                }
            }
        } else if (m_dragInfo.state == Active) {
            if (m_dispatcher && m_transformer) {
                glm::vec2 lastWorldPos = m_transformer->screenToWorld(m_dragInfo.lastScreenPos);
                
                DragEvent dragEvent;
                dragEvent.phase = DragPhase::Move;
                dragEvent.startWorld = m_dragInfo.startWorldPos;
                dragEvent.currentWorld = m_dragInfo.currentWorldPos;
                dragEvent.deltaWorld = m_dragInfo.currentWorldPos - lastWorldPos;
                dragEvent.startGrid = m_dragInfo.startGridPos;
                dragEvent.currentGrid = m_dragInfo.currentGridPos;
                dragEvent.dragTarget = m_dragInfo.target;
                dragEvent.distance = glm::length(event.screenPos - m_dragInfo.startScreenPos);
                dragEvent.duration = (event.timestamp - m_dragInfo.startTime) / 1000.0f;
                dragEvent.button = m_dragInfo.button;
                
                m_dispatcher->dispatch(dragEvent);
            }
        }
    }
    
    void onMouseUp(const MouseEvent& event) {
        if (m_dragInfo.state == Idle) return;
        
        if (m_dragInfo.state == Potential) {
            if (m_dispatcher) {
                ClickEvent clickEvent;
                clickEvent.worldPos = event.worldPos;
                clickEvent.gridPos = event.gridPos;
                clickEvent.button = event.button;
                clickEvent.hit = m_dragInfo.target;
                clickEvent.doubleClick = false; 
                clickEvent.timestamp = event.timestamp;
                
                m_dispatcher->dispatch(clickEvent);
            }
        } else if (m_dragInfo.state == Active) {
            if (m_dispatcher) {
                DragEvent dragEvent;
                dragEvent.phase = DragPhase::End;
                dragEvent.startWorld = m_dragInfo.startWorldPos;
                dragEvent.currentWorld = event.worldPos;
                dragEvent.deltaWorld = event.worldPos - m_dragInfo.startWorldPos;
                dragEvent.startGrid = m_dragInfo.startGridPos;
                dragEvent.currentGrid = event.gridPos;
                dragEvent.dragTarget = m_dragInfo.target;
                dragEvent.distance = glm::length(event.screenPos - m_dragInfo.startScreenPos);
                dragEvent.duration = (event.timestamp - m_dragInfo.startTime) / 1000.0f;
                dragEvent.button = m_dragInfo.button;
                
                m_dispatcher->dispatch(dragEvent);
            }
            m_isDragging = false;
        }
        
        m_dragInfo.state = Idle;
    }
    
    void cancelDrag() {
        if (m_dragInfo.state == Active && m_dispatcher) {
            DragEvent dragEvent;
            dragEvent.phase = DragPhase::Cancel;
            dragEvent.startWorld = m_dragInfo.startWorldPos;
            dragEvent.currentWorld = m_dragInfo.currentWorldPos;
            dragEvent.startGrid = m_dragInfo.startGridPos;
            dragEvent.currentGrid = m_dragInfo.currentGridPos;
            dragEvent.dragTarget = m_dragInfo.target;
            dragEvent.button = m_dragInfo.button;
            
            m_dispatcher->dispatch(dragEvent);
        }
        
        m_dragInfo.state = Idle;
        m_isDragging = false;
    }
    
    bool isDragging() const { return m_isDragging; }
    
    const DragInfo& getDragInfo() const { return m_dragInfo; }
    
    glm::vec2 getDragDelta() const {
        if (m_dragInfo.state != Active) return glm::vec2(0);
        return m_dragInfo.currentWorldPos - m_dragInfo.startWorldPos;
    }
    
    float getDragDistance() const {
        if (m_dragInfo.state == Idle) return 0;
        return m_dragInfo.accumDistance;
    }
    
    HitResult getDragTarget() const {
        return m_dragInfo.target;
    }
};

} // namespace Input