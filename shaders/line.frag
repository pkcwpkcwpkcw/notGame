#version 330 core

in float vTexCoord;
in float vSide;

uniform vec4 uSignalOnColor;
uniform vec4 uSignalOffColor;
uniform float uSignalState;    // 0.0 = off, 1.0 = on
uniform float uTime;
uniform float uFlowSpeed;
uniform bool uSelected;

out vec4 fragColor;

void main() {
    // Signal flow animation
    float flow = fract(vTexCoord * 5.0 - uTime * uFlowSpeed);
    float pulse = smoothstep(0.4, 0.6, flow) * uSignalState;
    
    // Base color based on signal state
    vec4 baseColor = mix(uSignalOffColor, uSignalOnColor, uSignalState);
    
    // Add pulse effect when signal is on
    vec3 pulseColor = baseColor.rgb * (1.0 + pulse * 0.3);
    
    // Selection highlight
    if (uSelected) {
        baseColor = mix(baseColor, vec4(1.0, 1.0, 0.0, 1.0), 0.5);
    }
    
    // Anti-aliasing at edges
    float edgeDist = abs(vSide);
    float edgeAlpha = 1.0 - smoothstep(0.8, 1.0, edgeDist);
    
    // Dotted line pattern for inactive wires
    float pattern = 1.0;
    if (uSignalState < 0.5) {
        pattern = step(0.5, fract(vTexCoord * 10.0));
    }
    
    fragColor = vec4(pulseColor, baseColor.a * edgeAlpha * pattern);
}