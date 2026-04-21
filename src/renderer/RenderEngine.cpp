#include "renderer/RenderEngine.hpp"
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
    // Minecraft textures often have transparency, so we want 4 channels
    stbi_set_flip_vertically_on_load(false); 
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
    
    if (!data) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return -1;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    // Minecraft uses Nearest filtering for that pixelated look
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    // glGenerateMipmap(GL_TEXTURE_2D); // Usually not used in old MC, but can be added

    stbi_image_free(data);
    
    return (int)texture;
}
