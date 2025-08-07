#version 330 core

in vec2 vTexCoord;
in vec4 vColor;

uniform sampler2D uTexture;
uniform vec4 uSpriteRect;  // x, y, width, height in texture coordinates
uniform vec4 uTintColor;
uniform float uSelected;
uniform float uActive;
uniform bool uUseTexture;

out vec4 fragColor;

void main() {
    vec4 texColor;
    
    if (uUseTexture) {
        // Sample from texture atlas
        vec2 atlasCoord = uSpriteRect.xy + vTexCoord * uSpriteRect.zw;
        texColor = texture(uTexture, atlasCoord);
    } else {
        // Solid color for debugging
        texColor = vec4(1.0);
    }
    
    // Apply tint and vertex color
    vec4 color = texColor * vColor * uTintColor;
    
    // Selection highlight (yellow outline effect)
    vec3 highlight = mix(color.rgb, vec3(1.0, 1.0, 0.0), uSelected * 0.3);
    
    // Active state glow (blue pulse)
    vec3 glow = highlight + vec3(0.0, 0.2, 0.5) * uActive;
    
    // Alpha test for sprites
    if (texColor.a < 0.01) {
        discard;
    }
    
    fragColor = vec4(glow, color.a);
}