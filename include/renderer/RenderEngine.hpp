#pragma once

#include <glad/glad.h>
#include <string>

#include <unordered_map>
#include <vector>

#include <memory>

class TextureFX;

constexpr const char* TEX_TERRAIN  = "/terrain.png";
constexpr const char* TEX_ITEMS    = "/gui/items.png";
constexpr const char* TEX_GUI      = "/gui/gui.png";
constexpr const char* TEX_ICONS    = "/gui/icons.png";
constexpr const char* TEX_CHAR     = "/char.png";
constexpr const char* TEX_WATER    = "/water.png";

class RenderEngine {
public:
    RenderEngine();
    ~RenderEngine();

    int getTexture(const std::string& name);
    int createTexture(int width, int height, const unsigned char* data);
    void bindTexture(int textureID);
    
    void registerTextureFX(std::unique_ptr<TextureFX> fx);
    void updateTextureFX();
    
private:
    std::unordered_map<std::string, int> textureMap;
    std::vector<std::unique_ptr<TextureFX>> textureFXList;
    
    int loadTexture(const std::string& path);
};
