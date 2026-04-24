#pragma once

#include <glad/glad.h>
#include <string>
#include <unordered_map>
#include <vector>

#include <memory>

class TextureFX;

class RenderEngine {
public:
    RenderEngine();
    ~RenderEngine();

    int getTexture(const std::string& name);
    void bindTexture(int textureID);
    
    void registerTextureFX(std::unique_ptr<TextureFX> fx);
    void updateTextureFX();
    
private:
    std::unordered_map<std::string, int> textureMap;
    std::vector<std::unique_ptr<TextureFX>> textureFXList;
    
    int loadTexture(const std::string& path);
};
