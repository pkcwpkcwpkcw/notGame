#version 330 core

layout(location = 0) in vec2 aPosition;

uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uInvViewProj;

out vec2 vWorldPos;

void main() {
    // Full screen quad in clip space
    gl_Position = vec4(aPosition * 2.0 - 1.0, 0.0, 1.0);
    
    // Calculate world position for fragment shader
    vec4 worldPos = uInvViewProj * gl_Position;
    vWorldPos = worldPos.xy / worldPos.w;
}