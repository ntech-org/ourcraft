#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec4 Color;

uniform sampler2D texture1;
uniform bool hasTexture;

void main() {
    if (hasTexture) {
        vec4 texColor = texture(texture1, TexCoord);
        if(texColor.a < 0.1) discard;
        FragColor = texColor * Color;
    } else {
        FragColor = Color;
    }
}
