#include "renderer/RenderEngine.hpp"
#include "renderer/TextureFX.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <iostream>
#include <filesystem>

RenderEngine::RenderEngine() {}

RenderEngine::~RenderEngine() {
    for (auto const& [name, id] : textureMap) {
        GLuint tex = (GLuint)id;
        glDeleteTextures(1, &tex);
    }
}

int RenderEngine::getTexture(const std::string& name) {
    if (textureMap.find(name) != textureMap.end()) {
        return textureMap[name];
    }
    
    int id = loadTexture(name);
    if (id >= 0) {
        textureMap[name] = id;
    }
    return id;
}

void RenderEngine::bindTexture(int textureID) {
    if (textureID >= 0) {
        glBindTexture(GL_TEXTURE_2D, textureID);
    }
}

void RenderEngine::registerTextureFX(std::unique_ptr<TextureFX> fx) {
    textureFXList.push_back(std::move(fx));
}

void RenderEngine::updateTextureFX() {
    if (textureFXList.empty()) return;

    int terrainID = getTexture("/terrain.png");
    if (terrainID < 0) return;
    
    glBindTexture(GL_TEXTURE_2D, (GLuint)terrainID);

    for (auto& fx : textureFXList) {
        fx->onTick();
        
        int tx = (fx->iconIndex % 16) * 16;
        int ty = (fx->iconIndex / 16) * 16;
        
        glTexSubImage2D(GL_TEXTURE_2D, 0, tx, ty, 16, 16, GL_RGBA, GL_UNSIGNED_BYTE, fx->imageData.data());
    }
}

int RenderEngine::loadTexture(const std::string& name) {
    // Search in assets/
    std::string path = "assets" + name;
    if (!std::filesystem::exists(path)) {
        // Try without leading slash
        if (name[0] == '/') {
            path = "assets" + name.substr(1);
        } else {
            path = "assets/" + name;
        }
    }
    
    if (!std::filesystem::exists(path)) {
        std::cerr << "Texture not found: " << path << std::endl;
        return -1;
    }

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(false); 
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
    
    if (!data) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return -1;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
    
    return (int)texture;
}
