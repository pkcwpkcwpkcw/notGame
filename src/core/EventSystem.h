#pragma once

#include <SDL.h>
#include <functional>
#include <unordered_map>
#include <vector>
#include <queue>

struct KeyboardState {
    bool keys[SDL_NUM_SCANCODES];
    bool prevKeys[SDL_NUM_SCANCODES];
    
    KeyboardState();
    void update();
    bool isPressed(SDL_Scancode key) const;
    bool isJustPressed(SDL_Scancode key) const;
    bool isJustReleased(SDL_Scancode key) const;
    
    bool isCtrlPressed() const;
    bool isShiftPressed() const;
    bool isAltPressed() const;
};

struct MouseState {
    int x, y;
    int prevX, prevY;
    int deltaX, deltaY;
    bool buttons[3];
    bool prevButtons[3];
    int wheelDelta;
    
    MouseState();
    void update();
    bool isButtonPressed(int button) const;
    bool isButtonJustPressed(int button) const;
    bool isButtonJustReleased(int button) const;
    bool isDragging(int button) const;
};

class IEventListener {
public:
    virtual ~IEventListener() = default;
    
    virtual void onKeyPress(SDL_Scancode key) {}
    virtual void onKeyRelease(SDL_Scancode key) {}
    virtual void onMousePress(int button, int x, int y) {}
    virtual void onMouseRelease(int button, int x, int y) {}
    virtual void onMouseMove(int x, int y, int dx, int dy) {}
    virtual void onMouseWheel(int delta) {}
    virtual void onWindowResize(int width, int height) {}
    virtual void onWindowEvent(SDL_WindowEventID event) {}
};

class EventSystem {
public:
    EventSystem();
    ~EventSystem();
    
    void processEvent(const SDL_Event& event);
    void update();
    
    void addEventListener(IEventListener* listener);
    void removeEventListener(IEventListener* listener);
    
    const KeyboardState& getKeyboard() const { return m_keyboard; }
    const MouseState& getMouse() const { return m_mouse; }
    
    void clearState();
    void setEventCapture(bool capture) { m_captureEvents = capture; }
    
private:
    void handleKeyboardEvent(const SDL_KeyboardEvent& e);
    void handleMouseButtonEvent(const SDL_MouseButtonEvent& e);
    void handleMouseMotionEvent(const SDL_MouseMotionEvent& e);
    void handleMouseWheelEvent(const SDL_MouseWheelEvent& e);
    void handleWindowEvent(const SDL_WindowEvent& e);
    
    void notifyKeyPress(SDL_Scancode key);
    void notifyKeyRelease(SDL_Scancode key);
    void notifyMousePress(int button, int x, int y);
    void notifyMouseRelease(int button, int x, int y);
    void notifyMouseMove(int x, int y, int dx, int dy);
    void notifyMouseWheel(int delta);
    void notifyWindowResize(int width, int height);
    void notifyWindowEvent(SDL_WindowEventID event);
    
private:
    KeyboardState m_keyboard;
    MouseState m_mouse;
    std::vector<IEventListener*> m_listeners;
    bool m_captureEvents;
    
    std::queue<SDL_Event> m_eventQueue;
    static const size_t MAX_EVENT_QUEUE_SIZE = 100;
};