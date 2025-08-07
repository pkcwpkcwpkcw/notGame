#version 330 core

layout(location = 0) in vec2 aPosition;
layout(location = 1) in float aSide;      // -1 or 1 for line thickness
layout(location = 2) in float aTexCoord;  // 0 to 1 along line

uniform mat4 uProjection;
uniform mat4 uView;
uniform vec2 uStartPos;
uniform vec2 uEndPos;
uniform float uLineThickness;

out float vTexCoord;
out float vSide;

void main() {
    vTexCoord = aTexCoord;
    vSide = aSide;
    
    // Calculate line direction and normal
    vec2 lineDir = normalize(uEndPos - uStartPos);
    vec2 lineNormal = vec2(-lineDir.y, lineDir.x);
    
    // Interpolate position along line
    vec2 linePos = mix(uStartPos, uEndPos, aTexCoord);
    
    // Apply thickness perpendicular to line
    vec2 offset = lineNormal * aSide * uLineThickness * 0.5;
    vec2 worldPos = linePos + offset;
    
    // Transform to clip space
    gl_Position = uProjection * uView * vec4(worldPos, 0.0, 1.0);
}