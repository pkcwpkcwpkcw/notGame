#pragma once
#include <cmath>
#include <algorithm>
#include <cstdint>

struct alignas(8) Vec2 {
    float x, y;
    
    constexpr Vec2() noexcept : x(0), y(0) {}
    constexpr Vec2(float x_, float y_) noexcept : x(x_), y(y_) {}
    
    Vec2 operator+(const Vec2& v) const noexcept { 
        return Vec2(x + v.x, y + v.y); 
    }
    Vec2 operator-(const Vec2& v) const noexcept { 
        return Vec2(x - v.x, y - v.y); 
    }
    Vec2 operator*(float s) const noexcept { 
        return Vec2(x * s, y * s); 
    }
    Vec2 operator/(float s) const noexcept { 
        return Vec2(x / s, y / s); 
    }
    
    Vec2& operator+=(const Vec2& v) noexcept {
        x += v.x; y += v.y; return *this;
    }
    Vec2& operator-=(const Vec2& v) noexcept {
        x -= v.x; y -= v.y; return *this;
    }
    Vec2& operator*=(float s) noexcept {
        x *= s; y *= s; return *this;
    }
    Vec2& operator/=(float s) noexcept {
        x /= s; y /= s; return *this;
    }
    
    [[nodiscard]] float length() const noexcept {
        return std::sqrt(x * x + y * y);
    }
    
    [[nodiscard]] float lengthSquared() const noexcept {
        return x * x + y * y;
    }
    
    [[nodiscard]] Vec2 normalized() const noexcept {
        float len = length();
        return len > 0 ? Vec2(x/len, y/len) : Vec2(0, 0);
    }
    
    [[nodiscard]] float dot(const Vec2& v) const noexcept {
        return x * v.x + y * v.y;
    }
    
    [[nodiscard]] float distance(const Vec2& v) const noexcept {
        return (*this - v).length();
    }
    
    [[nodiscard]] float distanceSquared(const Vec2& v) const noexcept {
        return (*this - v).lengthSquared();
    }
    
    [[nodiscard]] bool isZero() const noexcept {
        constexpr float EPSILON = 1e-6f;
        return std::abs(x) < EPSILON && std::abs(y) < EPSILON;
    }
    
    [[nodiscard]] bool equals(const Vec2& v, float epsilon = 1e-6f) const noexcept {
        return std::abs(x - v.x) < epsilon && std::abs(y - v.y) < epsilon;
    }
};

inline Vec2 operator*(float s, const Vec2& v) noexcept {
    return v * s;
}

struct Vec2i {
    int32_t x, y;
    
    constexpr Vec2i() noexcept : x(0), y(0) {}
    constexpr Vec2i(int32_t x_, int32_t y_) noexcept : x(x_), y(y_) {}
    
    Vec2i operator+(const Vec2i& v) const noexcept { 
        return Vec2i(x + v.x, y + v.y); 
    }
    Vec2i operator-(const Vec2i& v) const noexcept { 
        return Vec2i(x - v.x, y - v.y); 
    }
    
    bool operator==(const Vec2i& v) const noexcept {
        return x == v.x && y == v.y;
    }
    bool operator!=(const Vec2i& v) const noexcept {
        return !(*this == v);
    }
};