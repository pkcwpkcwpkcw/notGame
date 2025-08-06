#include "EventSystem.h"
#include <algorithm>
#include <cstring>

KeyboardState::KeyboardState() {
    std::memset(keys, 0, sizeof(keys));
    std::memset(prevKeys, 0, sizeof(prevKeys));
}

void KeyboardState::update() {
    std::memcpy(prevKeys, keys, sizeof(keys));
}

bool KeyboardState::isPressed(SDL_Scancode key) const {
    return keys[key];
}

bool KeyboardState::isJustPressed(SDL_Scancode key) const {
    return keys[key] && !prevKeys[key];
}

bool KeyboardState::isJustReleased(SDL_Scancode key) const {
    return !keys[key] && prevKeys[key];
}

bool KeyboardState::isCtrlPressed() const {
    return keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL];
}

bool KeyboardState::isShiftPressed() const {
    return keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
}

bool KeyboardState::isAltPressed() const {
    return keys[SDL_SCANCODE_LALT] || keys[SDL_SCANCODE_RALT];
}

MouseState::MouseState() 
    : x(0), y(0)
    , prevX(0), prevY(0)
    , deltaX(0), deltaY(0)
    , wheelDelta(0) {
    std::memset(buttons, 0, sizeof(buttons));
    std::memset(prevButtons, 0, sizeof(prevButtons));
}

void MouseState::update() {
    std::memcpy(prevButtons, buttons, sizeof(buttons));
    prevX = x;
    prevY = y;
    deltaX = 0;
    deltaY = 0;
    wheelDelta = 0;
}

bool MouseState::isButtonPressed(int button) const {
    if (button < 0 || button >= 3) return false;
    return buttons[button];
}

bool MouseState::isButtonJustPressed(int button) const {
    if (button < 0 || button >= 3) return false;
    return buttons[button] && !prevButtons[button];
}

bool MouseState::isButtonJustReleased(int button) const {
    if (button < 0 || button >= 3) return false;
    return !buttons[button] && prevButtons[button];
}

bool MouseState::isDragging(int button) const {
    if (button < 0 || button >= 3) return false;
    return buttons[button] && (deltaX != 0 || deltaY != 0);
}

EventSystem::EventSystem()
    : m_captureEvents(false) {
}

EventSystem::~EventSystem() {
    m_listeners.clear();
}

void EventSystem::processEvent(const SDL_Event& event) {
    if (m_eventQueue.size() >= MAX_EVENT_QUEUE_SIZE) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, 
                   "Event queue overflow, dropping oldest event");
        m_eventQueue.pop();
    }
    
    switch (event.type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            handleKeyboardEvent(event.key);
            break;
            
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            handleMouseButtonEvent(event.button);
            break;
            
        case SDL_MOUSEMOTION:
            handleMouseMotionEvent(event.motion);
            break;
            
        case SDL_MOUSEWHEEL:
            handleMouseWheelEvent(event.wheel);
            break;
            
        case SDL_WINDOWEVENT:
            handleWindowEvent(event.window);
            break;
    }
}

void EventSystem::update() {
    m_keyboard.update();
    m_mouse.update();
}

void EventSystem::addEventListener(IEventListener* listener) {
    if (listener) {
        m_listeners.push_back(listener);
    }
}

void EventSystem::removeEventListener(IEventListener* listener) {
    m_listeners.erase(
        std::remove(m_listeners.begin(), m_listeners.end(), listener),
        m_listeners.end()
    );
}

void EventSystem::clearState() {
    std::memset(m_keyboard.keys, 0, sizeof(m_keyboard.keys));
    std::memset(m_keyboard.prevKeys, 0, sizeof(m_keyboard.prevKeys));
    
    m_mouse = MouseState();
    
    while (!m_eventQueue.empty()) {
        m_eventQueue.pop();
    }
}

void EventSystem::handleKeyboardEvent(const SDL_KeyboardEvent& e) {
    SDL_Scancode scancode = e.keysym.scancode;
    
    if (e.type == SDL_KEYDOWN) {
        m_keyboard.keys[scancode] = true;
        notifyKeyPress(scancode);
    } else if (e.type == SDL_KEYUP) {
        m_keyboard.keys[scancode] = false;
        notifyKeyRelease(scancode);
    }
}

void EventSystem::handleMouseButtonEvent(const SDL_MouseButtonEvent& e) {
    int buttonIndex = -1;
    
    switch (e.button) {
        case SDL_BUTTON_LEFT:   buttonIndex = 0; break;
        case SDL_BUTTON_RIGHT:  buttonIndex = 1; break;
        case SDL_BUTTON_MIDDLE: buttonIndex = 2; break;
        default: return;
    }
    
    if (e.type == SDL_MOUSEBUTTONDOWN) {
        m_mouse.buttons[buttonIndex] = true;
        notifyMousePress(buttonIndex, e.x, e.y);
    } else if (e.type == SDL_MOUSEBUTTONUP) {
        m_mouse.buttons[buttonIndex] = false;
        notifyMouseRelease(buttonIndex, e.x, e.y);
    }
    
    m_mouse.x = e.x;
    m_mouse.y = e.y;
}

void EventSystem::handleMouseMotionEvent(const SDL_MouseMotionEvent& e) {
    m_mouse.x = e.x;
    m_mouse.y = e.y;
    m_mouse.deltaX = e.xrel;
    m_mouse.deltaY = e.yrel;
    
    notifyMouseMove(e.x, e.y, e.xrel, e.yrel);
}

void EventSystem::handleMouseWheelEvent(const SDL_MouseWheelEvent& e) {
    m_mouse.wheelDelta = e.y;
    notifyMouseWheel(e.y);
}

void EventSystem::handleWindowEvent(const SDL_WindowEvent& e) {
    notifyWindowEvent(static_cast<SDL_WindowEventID>(e.event));
    
    if (e.event == SDL_WINDOWEVENT_RESIZED) {
        notifyWindowResize(e.data1, e.data2);
    }
}

void EventSystem::notifyKeyPress(SDL_Scancode key) {
    for (auto listener : m_listeners) {
        listener->onKeyPress(key);
    }
}

void EventSystem::notifyKeyRelease(SDL_Scancode key) {
    for (auto listener : m_listeners) {
        listener->onKeyRelease(key);
    }
}

void EventSystem::notifyMousePress(int button, int x, int y) {
    for (auto listener : m_listeners) {
        listener->onMousePress(button, x, y);
    }
}

void EventSystem::notifyMouseRelease(int button, int x, int y) {
    for (auto listener : m_listeners) {
        listener->onMouseRelease(button, x, y);
    }
}

void EventSystem::notifyMouseMove(int x, int y, int dx, int dy) {
    for (auto listener : m_listeners) {
        listener->onMouseMove(x, y, dx, dy);
    }
}

void EventSystem::notifyMouseWheel(int delta) {
    for (auto listener : m_listeners) {
        listener->onMouseWheel(delta);
    }
}

void EventSystem::notifyWindowResize(int width, int height) {
    for (auto listener : m_listeners) {
        listener->onWindowResize(width, height);
    }
}

void EventSystem::notifyWindowEvent(SDL_WindowEventID event) {
    for (auto listener : m_listeners) {
        listener->onWindowEvent(event);
    }
}