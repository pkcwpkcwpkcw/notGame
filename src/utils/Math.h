#pragma once

#include <cmath>
#include <algorithm>

namespace notgame {
namespace math {

// Constants
constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;
constexpr float HALF_PI = 0.5f * PI;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

// Basic math functions
template<typename T>
inline T min(T a, T b) {
    return (a < b) ? a : b;
}

template<typename T>
inline T max(T a, T b) {
    return (a > b) ? a : b;
}

template<typename T>
inline T clamp(T value, T minVal, T maxVal) {
    return min(max(value, minVal), maxVal);
}

template<typename T>
inline T lerp(T a, T b, float t) {
    return a + (b - a) * t;
}

// Float comparison with epsilon
inline bool nearlyEqual(float a, float b, float epsilon = 0.0001f) {
    return std::abs(a - b) < epsilon;
}

// Angle functions
inline float normalizeAngle(float angle) {
    while (angle > PI) angle -= TWO_PI;
    while (angle < -PI) angle += TWO_PI;
    return angle;
}

inline float degreesToRadians(float degrees) {
    return degrees * DEG_TO_RAD;
}

inline float radiansToDegrees(float radians) {
    return radians * RAD_TO_DEG;
}

// Power of 2 utilities
inline bool isPowerOfTwo(int n) {
    return n > 0 && (n & (n - 1)) == 0;
}

inline int nextPowerOfTwo(int n) {
    if (isPowerOfTwo(n)) return n;
    
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    
    return n;
}

// Vector math
inline float distance(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return std::sqrt(dx * dx + dy * dy);
}

inline float distanceSquared(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return dx * dx + dy * dy;
}

} // namespace math
} // namespace notgame