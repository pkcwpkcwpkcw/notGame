#version 330 core

in vec2 vTexCoord;
in vec4 vColor;

uniform sampler2D uTexture;
uniform vec4 uClipRect;  // x, y, z=right, w=bottom
uniform bool uUseTexture;

out vec4 fragColor;

void main() {
    // Scissor test for clipping
    vec2 fragPos = gl_FragCoord.xy;
    if (fragPos.x < uClipRect.x || fragPos.y < uClipRect.y ||
        fragPos.x > uClipRect.z || fragPos.y > uClipRect.w) {
        discard;
    }
    
    vec4 color;
    if (uUseTexture) {
        // Sample texture (for fonts and images)
        color = texture(uTexture, vTexCoord);
        
        // For font rendering, texture contains alpha only
        if (color.r == color.g && color.g == color.b) {
            color = vec4(vColor.rgb, color.a * vColor.a);
        } else {
            color *= vColor;
        }
    } else {
        // Solid color
        color = vColor;
    }
    
    fragColor = color;
}