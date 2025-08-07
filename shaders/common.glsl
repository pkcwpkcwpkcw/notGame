// Common shader functions and definitions
// Include this file using: #include "common.glsl"

#ifndef COMMON_GLSL
#define COMMON_GLSL

// Constants
#define PI 3.14159265359
#define TWO_PI 6.28318530718
#define EPSILON 0.0001

// Color utilities
vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// Anti-aliasing
float smoothEdge(float edge, float width) {
    return smoothstep(edge - width, edge + width, 0.5);
}

// Grid functions
float gridLine(float pos, float gridSize, float lineWidth) {
    float grid = abs(fract(pos / gridSize - 0.5) - 0.5);
    float width = fwidth(pos / gridSize) * lineWidth;
    return 1.0 - smoothstep(0.0, width, grid);
}

// Distance functions
float distanceToLine(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a;
    vec2 ba = b - a;
    float t = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * t);
}

// Transformation matrices
mat2 rotate2D(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat2(c, -s, s, c);
}

mat3 translate2D(vec2 t) {
    return mat3(
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        t.x, t.y, 1.0
    );
}

mat3 scale2D(vec2 s) {
    return mat3(
        s.x, 0.0, 0.0,
        0.0, s.y, 0.0,
        0.0, 0.0, 1.0
    );
}

// Interpolation
float linearStep(float edge0, float edge1, float x) {
    return clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
}

float exponentialIn(float t) {
    return t == 0.0 ? 0.0 : pow(2.0, 10.0 * (t - 1.0));
}

float exponentialOut(float t) {
    return t == 1.0 ? 1.0 : 1.0 - pow(2.0, -10.0 * t);
}

// Pattern generation
float checkerboard(vec2 uv, float scale) {
    vec2 checker = floor(uv * scale);
    return mod(checker.x + checker.y, 2.0);
}

float dots(vec2 uv, float scale, float size) {
    vec2 pos = fract(uv * scale) - 0.5;
    return 1.0 - smoothstep(size - 0.01, size + 0.01, length(pos));
}

// Signal visualization
vec3 signalColor(float signal, float time) {
    vec3 offColor = vec3(0.2, 0.2, 0.2);
    vec3 onColor = vec3(0.0, 1.0, 0.0);
    
    // Pulse effect when on
    float pulse = sin(time * TWO_PI * 2.0) * 0.5 + 0.5;
    onColor *= 0.8 + pulse * 0.2;
    
    return mix(offColor, onColor, signal);
}

// Debugging utilities
vec3 debugUV(vec2 uv) {
    return vec3(uv, 0.0);
}

vec3 debugNormal(vec3 normal) {
    return normal * 0.5 + 0.5;
}

vec3 heatmap(float value) {
    vec3 cold = vec3(0.0, 0.0, 1.0);
    vec3 warm = vec3(1.0, 1.0, 0.0);
    vec3 hot = vec3(1.0, 0.0, 0.0);
    
    if (value < 0.5) {
        return mix(cold, warm, value * 2.0);
    } else {
        return mix(warm, hot, (value - 0.5) * 2.0);
    }
}

#endif // COMMON_GLSL