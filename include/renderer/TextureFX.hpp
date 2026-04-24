#pragma once

#include <vector>
#include <cstdint>

class TextureFX {
public:
    std::vector<uint8_t> imageData;
    int iconIndex;
    int tileSize = 16;

    explicit TextureFX(int iconIndex) : iconIndex(iconIndex) {
        imageData.resize(tileSize * tileSize * 4);
    }
    virtual ~TextureFX() = default;

    virtual void onTick() = 0;
};

class TextureWaterFX : public TextureFX {
public:
    TextureWaterFX();
    void onTick() override;

private:
    float red[256]{};
    float green[256]{};
    float blue[256]{};
    float alpha[256]{};
};

class TextureWaterFlowFX : public TextureFX {
public:
    TextureWaterFlowFX();
    void onTick() override;

private:
    float red[256]{};
    float green[256]{};
    float blue[256]{};
    float alpha[256]{};
    int tickCounter = 0;
};

class TextureLavaFX : public TextureFX {
public:
    TextureLavaFX();
    void onTick() override;

private:
    float red[256]{};
    float green[256]{};
    float blue[256]{};
    float alpha[256]{};
};

class TextureLavaFlowFX : public TextureFX {
public:
    TextureLavaFlowFX();
    void onTick() override;

private:
    float red[256]{};
    float green[256]{};
    float blue[256]{};
    float alpha[256]{};
    int tickCounter = 0;
};
