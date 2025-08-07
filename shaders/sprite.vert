#version 330 core

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexCoord;

// Per-instance attributes for instancing
layout(location = 3) in vec2 aInstancePos;
layout(location = 4) in float aInstanceRotation;
layout(location = 5) in vec4 aInstanceColor;

uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel;
uniform vec2 uPosition;
uniform float uRotation;
uniform vec2 uScale;
uniform bool uUseInstancing;

out vec2 vTexCoord;
out vec4 vColor;

void main() {
    vTexCoord = aTexCoord;
    
    vec2 position;
    float rotation;
    
    if (uUseInstancing) {
        position = aInstancePos;
        rotation = aInstanceRotation;
        vColor = aInstanceColor;
    } else {
        position = uPosition;
        rotation = uRotation;
        vColor = vec4(1.0);
    }
    
    // Apply rotation
    float cosR = cos(rotation);
    float sinR = sin(rotation);
    mat2 rotMatrix = mat2(cosR, -sinR, sinR, cosR);
    
    // Transform vertex
    vec2 scaledPos = aPosition * uScale;
    vec2 rotatedPos = rotMatrix * scaledPos;
    vec2 worldPos = position + rotatedPos;
    
    // Apply view-projection
    gl_Position = uProjection * uView * vec4(worldPos, 0.0, 1.0);
}