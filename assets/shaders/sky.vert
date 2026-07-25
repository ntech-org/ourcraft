#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aColor;

out vec2 TexCoord;
out vec4 Color;
out float LocalY;
out float Dist;
out vec3 WorldPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 uCameraPos;
uniform float uTime;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    WorldPos = worldPos.xyz;
    vec4 viewPos = view * worldPos;
    gl_Position = projection * viewPos;
    TexCoord = aTexCoord;
    Color = aColor;
    LocalY = aPos.y;
    Dist = length(viewPos.xyz);
}
