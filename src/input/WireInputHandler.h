#pragma once

#include "input/InputTypes.h"
#include "input/EventDispatcher.h"
#include "core/WireManager.h"
#include <memory>

namespace Input {

class WireInputHandler {
public:
    explicit WireInputHandler(WireManager* wireManager);
    ~WireInputHandler() = default;
    
    void setEventDispatcher(EventDispatcher* dispatcher);
    void enable();
    void disable();
    
    void onDragEvent(const DragEvent& event);
    void onClickEvent(const ClickEvent& event);
    void onHoverEvent(const HoverEvent& event);
    
    [[nodiscard]] bool isEnabled() const noexcept { return m_enabled; }
    [[nodiscard]] bool isConnecting() const noexcept;
    
private:
    WireManager* m_wireManager;
    EventDispatcher* m_dispatcher;
    bool m_enabled{true};
    
    uint32_t m_dragEventId{0};
    uint32_t m_clickEventId{0};
    uint32_t m_hoverEventId{0};
};

} // namespace Input