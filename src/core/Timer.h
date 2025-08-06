#pragma once

#include <cstdint>
#include <chrono>

class Timer {
public:
    explicit Timer(uint32_t targetFPS = 60);
    ~Timer();
    
    void beginFrame();
    void endFrame();
    void waitForTargetFPS();
    
    float getDeltaTime() const { return m_deltaTime; }
    float getTimeSinceStart() const;
    uint32_t getCurrentTicks() const;
    
    float getCurrentFPS() const { return m_currentFPS; }
    uint32_t getTargetFPS() const { return m_targetFPS; }
    void setTargetFPS(uint32_t fps);
    
    float getAverageFrameTime() const { return m_avgFrameTime; }
    float getMaxFrameTime() const { return m_maxFrameTime; }
    float getMinFrameTime() const { return m_minFrameTime; }
    void resetStats();
    
private:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using Duration = std::chrono::duration<float>;
    
    TimePoint m_startTime;
    TimePoint m_frameStartTime;
    TimePoint m_lastFrameTime;
    float m_deltaTime;
    
    uint32_t m_targetFPS;
    float m_targetFrameTime;
    
    uint32_t m_frameCount;
    float m_fpsAccumulator;
    float m_currentFPS;
    
    float m_avgFrameTime;
    float m_maxFrameTime;
    float m_minFrameTime;
    float m_frameTimeAccumulator;
    uint32_t m_statFrameCount;
};