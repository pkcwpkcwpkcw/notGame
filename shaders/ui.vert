#version 330 core

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;

uniform vec2 uScreenSize;
uniform mat4 uProjection;  // For ImGui compatibility

out vec2 vTexCoord;
out vec4 vColor;

void main() {
    vTexCoord = aTexCoord;
    vColor = aColor;
    
    // Convert screen coordinates to NDC
    vec2 ndc = (aPosition / uScreenSize) * 2.0 - 1.0;
    
    // Flip Y coordinate for correct orientation
    ndc.y = -ndc.y;
    
    // Pixel-perfect alignment
    vec2 pixelPos = round(aPosition);
    ndc = (pixelPos / uScreenSize) * 2.0 - 1.0;
    ndc.y = -ndc.y;
    
    gl_Position = vec4(ndc, 0.0, 1.0);
}