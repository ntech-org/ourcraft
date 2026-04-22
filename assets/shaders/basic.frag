#version 330 core
out vec4 FragColor;

in vec2 TileCoord;
in vec4 Color;
in vec3 WorldPos;
flat in uint TexIndex;
flat in uint FaceId;

uniform sampler2D texture1;
uniform bool hasTexture;
uniform vec3 fogColor;
uniform vec3 cameraPos;
uniform float fogNear;
uniform float fogFar;
uniform float daylightFactor;
uniform vec3 sunDirection;

vec2 atlasUV(uint texIndex, vec2 tileCoord) {
    vec2 atlasTiles = vec2(16.0, 16.0);
    vec2 tileSize = 1.0 / atlasTiles;
    vec2 texel = 1.0 / vec2(textureSize(texture1, 0));
    vec2 localUV = fract(tileCoord);
    vec2 tileOffset = vec2(float(texIndex % 16u), float(texIndex / 16u)) * tileSize;
    // Match the old atlas convention: no half-texel shift at the start of the tile,
    // only a tiny inset on the far edge to avoid bleeding.
    vec2 minUV = tileOffset;
    vec2 maxUV = tileOffset + tileSize - texel * 0.001;
    return mix(minUV, maxUV, localUV);
}

float faceShade(uint faceId) {
    if (faceId == 0u) return 0.5;
    if (faceId == 1u) return 1.0;
    if (faceId == 2u || faceId == 3u) return 0.8;
    return 0.6;
}

vec3 faceNormal(uint faceId) {
    if (faceId == 0u) return vec3(0.0, -1.0, 0.0);
    if (faceId == 1u) return vec3(0.0, 1.0, 0.0);
    if (faceId == 2u) return vec3(0.0, 0.0, -1.0);
    if (faceId == 3u) return vec3(0.0, 0.0, 1.0);
    if (faceId == 4u) return vec3(-1.0, 0.0, 0.0);
    return vec3(1.0, 0.0, 0.0);
}

void main() {
    vec4 outColor = Color;
    if (hasTexture) {
        vec4 texColor = texture(texture1, atlasUV(TexIndex, TileCoord)) * Color;
        if (texColor.a < 0.1) discard;
        outColor = texColor;
    }

    float classicShade = faceShade(FaceId);
    float sunDiffuse = max(dot(faceNormal(FaceId), normalize(sunDirection)), 0.0);
    float lighting = classicShade * mix(0.35, 1.0, daylightFactor) + sunDiffuse * 0.25 * daylightFactor;
    outColor.rgb *= clamp(lighting, 0.10, 1.0);

    float fogFactor = clamp((fogFar - distance(WorldPos, cameraPos)) / max(fogFar - fogNear, 0.001), 0.0, 1.0);
    FragColor = vec4(mix(fogColor, outColor.rgb, fogFactor), outColor.a);
}
