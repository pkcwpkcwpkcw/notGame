#include "Timer.h"
#include <SDL.h>
#include <algorithm>
#include <thread>

Timer::Timer(uint32_t targetFPS)
    : m_targetFPS(targetFPS)
    , m_targetFrameTime(1.0f / targetFPS)
    , m_deltaTime(0.0f)
    , m_frameCount(0)
    , m_fpsAccumulator(0.0f)
    , m_currentFPS(0.0f)
    , m_avgFrameTime(0.0f)
    , m_maxFrameTime(0.0f)
    , m_minFrameTime(1000.0f)
    , m_frameTimeAccumulator(0.0f)
    , m_statFrameCount(0) {
    m_startTime = Clock::now();
    m_lastFrameTime = m_startTime;
}

Timer::~Timer() = default;

void Timer::beginFrame() {
    m_frameStartTime = Clock::now();
    
    Duration delta = m_frameStartTime - m_lastFrameTime;
    m_deltaTime = delta.count();
    
    m_deltaTime = std::min(m_deltaTime, 0.1f);
    
    m_lastFrameTime = m_frameStartTime;
}

void Timer::endFrame() {
    TimePoint frameEndTime = Clock::now();
    Duration frameTime = frameEndTime - m_frameStartTime;
    float frameDuration = frameTime.count();
    
    m_frameCount++;
    m_fpsAccumulator += m_deltaTime;
    
    if (m_fpsAccumulator >= 1.0f) {
        m_currentFPS = m_frameCount / m_fpsAccumulator;
        m_frameCount = 0;
        m_fpsAccumulator = 0.0f;
    }
    
    m_frameTimeAccumulator += frameDuration;
    m_statFrameCount++;
    m_maxFrameTime = std::max(m_maxFrameTime, frameDuration);
    m_minFrameTime = std::min(m_minFrameTime, frameDuration);
    
    if (m_statFrameCount >= 100) {
        m_avgFrameTime = m_frameTimeAccumulator / m_statFrameCount;
        m_frameTimeAccumulator = 0.0f;
        m_statFrameCount = 0;
    }
}

void Timer::waitForTargetFPS() {
    TimePoint currentTime = Clock::now();
    Duration elapsedTime = currentTime - m_frameStartTime;
    float elapsed = elapsedTime.count();
    
    if (elapsed < m_targetFrameTime) {
        float sleepTime = m_targetFrameTime - elapsed;
        
        if (sleepTime > 0.002f) {
            SDL_Delay(static_cast<uint32_t>((sleepTime - 0.001f) * 1000));
        }
        
        while (true) {
            currentTime = Clock::now();
            elapsedTime = currentTime - m_frameStartTime;
            if (elapsedTime.count() >= m_targetFrameTime) {
                break;
            }
        }
    }
}

void Timer::setTargetFPS(uint32_t fps) {
    m_targetFPS = fps;
    m_targetFrameTime = 1.0f / fps;
}

float Timer::getTimeSinceStart() const {
    TimePoint currentTime = Clock::now();
    Duration elapsed = currentTime - m_startTime;
    return elapsed.count();
}

uint32_t Timer::getCurrentTicks() const {
    return SDL_GetTicks();
}

void Timer::resetStats() {
    m_avgFrameTime = 0.0f;
    m_maxFrameTime = 0.0f;
    m_minFrameTime = 1000.0f;
    m_frameTimeAccumulator = 0.0f;
    m_statFrameCount = 0;
}