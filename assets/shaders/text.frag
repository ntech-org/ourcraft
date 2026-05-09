#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec4 Color;

uniform sampler2D texture1;
uniform bool hasTexture;

void main() {
    if (hasTexture) {
        vec4 texColor = texture(texture1, TexCoord);
        // Sharpen alpha slightly to combat blur
        float alpha = pow(texColor.a, 1.2);
        FragColor = vec4(texColor.rgb, alpha) * Color;
    } else {
        FragColor = Color;
    }
}
