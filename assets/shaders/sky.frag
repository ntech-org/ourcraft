#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec4 Color;
in float LocalY;
in float Dist;

uniform sampler2D texture1;
uniform bool hasTexture;
uniform vec4 tint;
uniform bool useGradient;
uniform vec3 gradientTopColor;
uniform vec3 gradientBottomColor;
uniform float gradientMinY;
uniform float gradientMaxY;

uniform bool useFog;
uniform vec4 fogColor;
uniform float fogStart;
uniform float fogEnd;

void main() {
    vec4 outColor = Color * tint;
    if (useGradient) {
        float t = clamp((LocalY - gradientMinY) / max(gradientMaxY - gradientMinY, 0.001), 0.0, 1.0);
        outColor.rgb *= mix(gradientBottomColor, gradientTopColor, t);
    }
    if (hasTexture) {
        vec4 texColor = texture(texture1, TexCoord) * outColor;
        if (texColor.a < 0.1) discard;
        outColor = texColor;
    }

    if (useFog) {
        float fogFactor = clamp((fogEnd - Dist) / (fogEnd - fogStart), 0.0, 1.0);
        outColor.rgb = mix(fogColor.rgb, outColor.rgb, fogFactor);
    }

    FragColor = outColor;
}
