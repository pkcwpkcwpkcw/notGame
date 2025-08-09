#include "WireInputHandler.h"

namespace Input {

WireInputHandler::WireInputHandler(WireManager* wireManager)
    : m_wireManager(wireManager) {
}

void WireInputHandler::setEventDispatcher(EventDispatcher* dispatcher) {
    if (m_dispatcher) {
        disable();
    }
    
    m_dispatcher = dispatcher;
    
    if (m_dispatcher && m_enabled) {
        enable();
    }
}

void WireInputHandler::enable() {
    if (!m_dispatcher || m_enabled) return;
    
    m_dragEventId = m_dispatcher->subscribe<DragEvent>(
        [this](const DragEvent& event) { onDragEvent(event); }
    );
    
    m_clickEventId = m_dispatcher->subscribe<ClickEvent>(
        [this](const ClickEvent& event) { onClickEvent(event); }
    );
    
    m_hoverEventId = m_dispatcher->subscribe<HoverEvent>(
        [this](const HoverEvent& event) { onHoverEvent(event); }
    );
    
    m_enabled = true;
}

void WireInputHandler::disable() {
    if (!m_dispatcher || !m_enabled) return;
    
    if (m_dragEventId) {
        m_dispatcher->unsubscribe<DragEvent>(m_dragEventId);
        m_dragEventId = 0;
    }
    
    if (m_clickEventId) {
        m_dispatcher->unsubscribe<ClickEvent>(m_clickEventId);
        m_clickEventId = 0;
    }
    
    if (m_hoverEventId) {
        m_dispatcher->unsubscribe<HoverEvent>(m_hoverEventId);
        m_hoverEventId = 0;
    }
    
    m_enabled = false;
}

void WireInputHandler::onDragEvent(const DragEvent& event) {
    if (!m_wireManager || !m_enabled) return;
    
    switch (event.phase) {
        case DragPhase::Start:
            m_wireManager->onDragStart(event);
            break;
        case DragPhase::Move:
            m_wireManager->onDragMove(event);
            break;
        case DragPhase::End:
            m_wireManager->onDragEnd(event);
            break;
        case DragPhase::Cancel:
            m_wireManager->onDragCancel(event);
            break;
        default:
            break;
    }
}

void WireInputHandler::onClickEvent(const ClickEvent& event) {
    if (!m_wireManager || !m_enabled) return;
    
    m_wireManager->onClick(event);
}

void WireInputHandler::onHoverEvent(const HoverEvent& event) {
    if (!m_wireManager || !m_enabled) return;
    
    Vec2 worldPos{event.worldPos.x, event.worldPos.y};
    m_wireManager->onMouseMove(worldPos);
}

bool WireInputHandler::isConnecting() const noexcept {
    return m_wireManager && m_wireManager->isConnecting();
}

} // namespace Input