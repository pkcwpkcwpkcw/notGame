#pragma once

#include "InputTypes.h"
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <any>
#include <queue>
#include <mutex>
#include <memory>

namespace Input {

class EventDispatcher {
private:
    std::unordered_map<std::type_index, std::vector<std::any>> m_callbacks;
    std::queue<std::function<void()>> m_eventQueue;
    std::mutex m_queueMutex;
    bool m_processingEvents = false;
    
public:
    EventDispatcher() = default;
    
    template<typename EventType>
    void subscribe(EventCallback<EventType> callback) {
        auto& callbacks = m_callbacks[std::type_index(typeid(EventType))];
        callbacks.push_back(callback);
    }
    
    template<typename EventType>
    void unsubscribeAll() {
        m_callbacks.erase(std::type_index(typeid(EventType)));
    }
    
    template<typename EventType>
    void dispatch(const EventType& event) {
        if (m_processingEvents) {
            enqueue(event);
            return;
        }
        
        auto it = m_callbacks.find(std::type_index(typeid(EventType)));
        if (it != m_callbacks.end()) {
            for (const auto& anyCallback : it->second) {
                try {
                    auto callback = std::any_cast<EventCallback<EventType>>(anyCallback);
                    callback(event);
                } catch (const std::bad_any_cast& e) {
                    
                }
            }
        }
    }
    
    template<typename EventType>
    void enqueue(const EventType& event) {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        auto eventCopy = std::make_shared<EventType>(event);
        m_eventQueue.push([this, eventCopy]() { 
            dispatch(*eventCopy); 
        });
    }
    
    void processQueue() {
        std::queue<std::function<void()>> localQueue;
        
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            std::swap(localQueue, m_eventQueue);
        }
        
        m_processingEvents = true;
        
        while (!localQueue.empty()) {
            localQueue.front()();
            localQueue.pop();
        }
        
        m_processingEvents = false;
    }
    
    void clear() {
        m_callbacks.clear();
        std::queue<std::function<void()>> empty;
        std::swap(m_eventQueue, empty);
    }
    
    size_t getQueueSize() const {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_queueMutex));
        return m_eventQueue.size();
    }
    
    template<typename EventType>
    size_t getSubscriberCount() const {
        auto it = m_callbacks.find(std::type_index(typeid(EventType)));
        return (it != m_callbacks.end()) ? it->second.size() : 0;
    }
};

} // namespace Input