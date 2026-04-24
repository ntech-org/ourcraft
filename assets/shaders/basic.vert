#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTileCoord;
layout (location = 2) in vec4 aColor;
layout (location = 3) in uint aTexIndex;
layout (location = 4) in uint aFaceId;
layout (location = 5) in float aFlowRotation;
layout (location = 6) in float aLiquidType;
layout (location = 7) in float aIsUnderwater;

out vec2 TileCoord;
out vec4 Color;
out vec3 WorldPos;
flat out uint TexIndex;
flat out uint FaceId;
flat out float FlowRotation;
flat out float LiquidType;
out float IsUnderwater;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TileCoord = aTileCoord;
    Color = aColor;
    WorldPos = aPos;
    TexIndex = aTexIndex;
    FaceId = aFaceId;
    FlowRotation = aFlowRotation;
    LiquidType = aLiquidType;
    IsUnderwater = aIsUnderwater;
}
