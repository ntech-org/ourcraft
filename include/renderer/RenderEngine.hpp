#pragma once

#include <glad/glad.h>
#include <string>
#include <unordered_map>
#include <vector>

class RenderEngine {
public:
    RenderEngine();
    ~RenderEngine();

    int getTexture(const std::string& name);
    void bindTexture(int textureID);
    
    // We can add dynamic texture support later if needed for Water/Lava
    
private:
    std::unordered_map<std::string, int> textureMap;
    
    int loadTexture(const std::string& path);
};
