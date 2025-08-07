#version 330 core

in vec2 vWorldPos;

uniform float uGridSize;
uniform vec4 uGridColor;
uniform vec4 uSubGridColor;
uniform vec2 uCameraPos;
uniform float uZoom;

out vec4 fragColor;

float grid(vec2 pos, float size) {
    vec2 grid = abs(fract(pos / size - 0.5) - 0.5) / fwidth(pos / size);
    return 1.0 - min(min(grid.x, grid.y), 1.0);
}

void main() {
    // Calculate major (10x) and minor grid lines
    float majorGrid = grid(vWorldPos, uGridSize * 10.0);
    float minorGrid = grid(vWorldPos, uGridSize);
    
    // Mix colors based on grid type
    vec4 color = mix(uSubGridColor, uGridColor, majorGrid);
    float alpha = max(majorGrid, minorGrid * 0.5);
    
    // Distance-based fade
    float dist = length(vWorldPos - uCameraPos);
    float fadeStart = 100.0 / max(uZoom, 0.1);
    float fadeEnd = 200.0 / max(uZoom, 0.1);
    alpha *= 1.0 - smoothstep(fadeStart, fadeEnd, dist);
    
    // Axis highlighting
    float axisThickness = 2.0 / uZoom;
    float xAxis = 1.0 - smoothstep(0.0, axisThickness, abs(vWorldPos.y));
    float yAxis = 1.0 - smoothstep(0.0, axisThickness, abs(vWorldPos.x));
    
    if (xAxis > 0.0) {
        color = mix(color, vec4(1.0, 0.0, 0.0, 1.0), xAxis);
        alpha = max(alpha, xAxis);
    }
    if (yAxis > 0.0) {
        color = mix(color, vec4(0.0, 1.0, 0.0, 1.0), yAxis);
        alpha = max(alpha, yAxis);
    }
    
    fragColor = vec4(color.rgb, color.a * alpha);
}