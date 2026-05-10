#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 WorldPos;
in vec4 VertexColor;
in vec3 Normal;

uniform sampler2D texture1;
uniform vec3 colorTint;
uniform vec3 fogColor;
uniform vec3 cameraPos;
uniform float fogNear;
uniform float fogFar;
uniform float fogDensity;
uniform int fogMode;
uniform float daylightFactor;
uniform vec3 sunDirection;

void main() {
    vec4 texColor = texture(texture1, TexCoord);
    if(texColor.a < 0.1)
        discard;
    
    vec3 color = texColor.rgb * colorTint * VertexColor.rgb;
    
    vec3 normal = normalize(Normal);
    vec3 sunDir = normalize(sunDirection);
    float sunDiffuse = max(dot(normal, sunDir), 0.0);
    float directionalLight = sunDiffuse * 0.35 * daylightFactor;
    color *= (1.0 + directionalLight);
    
    float fogFactor = 1.0;
    float dist = length(WorldPos);
    if (fogMode == 1) {
        fogFactor = exp(-fogDensity * dist);
    } else {
        fogFactor = (fogFar - dist) / max(fogFar - fogNear, 0.001);
    }
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    FragColor = vec4(mix(fogColor, color, fogFactor), texColor.a);
}

