#include "renderer/TextureFX.hpp"
#include <cmath>
#include <cstdlib>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- Water ---

TextureWaterFX::TextureWaterFX() : TextureFX(205) {
}

void TextureWaterFX::onTick() {
    for (int i = 0; i < 16; ++i) {
        for (int j = 0; j < 16; ++j) {
            float var3 = 0.0f;
            for (int k = i - 1; k <= i + 1; ++k) {
                int var5 = k & 15;
                int var6 = j & 15;
                var3 += red[var5 + var6 * 16];
            }
            green[i + j * 16] = var3 / 3.3f + blue[i + j * 16] * 0.8f;
        }
    }

    for (int i = 0; i < 16; ++i) {
        for (int j = 0; j < 16; ++j) {
            blue[i + j * 16] += alpha[i + j * 16] * 0.05f;
            if (blue[i + j * 16] < 0.0f) blue[i + j * 16] = 0.0f;
            alpha[i + j * 16] -= 0.1f;
            if ((float)rand() / (float)RAND_MAX < 0.05f) {
                alpha[i + j * 16] = 0.5f;
            }
        }
    }

    for (int i = 0; i < 256; ++i) red[i] = green[i];

    for (int i = 0; i < 256; ++i) {
        float var3 = red[i];
        if (var3 > 1.0f) var3 = 1.0f;
        if (var3 < 0.0f) var3 = 0.0f;
        float var13 = var3 * var3;
        int r = (int)(32.0f + var13 * 32.0f);
        int g = (int)(50.0f + var13 * 64.0f);
        int b = 255;
        int a = (int)(146.0f + var13 * 50.0f);

        imageData[i * 4 + 0] = (uint8_t)r;
        imageData[i * 4 + 1] = (uint8_t)g;
        imageData[i * 4 + 2] = (uint8_t)b;
        imageData[i * 4 + 3] = (uint8_t)a;
    }
}

TextureWaterFlowFX::TextureWaterFlowFX() : TextureFX(206) {
}

void TextureWaterFlowFX::onTick() {
    tickCounter++;
    for (int i = 0; i < 16; ++i) {
        for (int j = 0; j < 16; ++j) {
            float var3 = 0.0f;
            for (int k = j - 2; k <= j; ++k) {
                int var5 = i & 15;
                int var6 = k & 15;
                var3 += red[var5 + var6 * 16];
            }
            green[i + j * 16] = var3 / 3.2f + blue[i + j * 16] * 0.8f;
        }
    }

    for (int i = 0; i < 16; ++i) {
        for (int j = 0; j < 16; ++j) {
            blue[i + j * 16] += alpha[i + j * 16] * 0.05f;
            if (blue[i + j * 16] < 0.0f) blue[i + j * 16] = 0.0f;
            alpha[i + j * 16] -= 0.3f;
            if ((float)rand() / (float)RAND_MAX < 0.2f) {
                alpha[i + j * 16] = 0.5f;
            }
        }
    }

    for (int i = 0; i < 256; ++i) red[i] = green[i];

    for (int i = 0; i < 256; ++i) {
        float var3 = red[(i - tickCounter * 16) & 255];
        if (var3 > 1.0f) var3 = 1.0f;
        if (var3 < 0.0f) var3 = 0.0f;
        float var13 = var3 * var3;
        int r = (int)(32.0f + var13 * 32.0f);
        int g = (int)(50.0f + var13 * 64.0f);
        int b = 255;
        int a = (int)(146.0f + var13 * 50.0f);

        imageData[i * 4 + 0] = (uint8_t)r;
        imageData[i * 4 + 1] = (uint8_t)g;
        imageData[i * 4 + 2] = (uint8_t)b;
        imageData[i * 4 + 3] = (uint8_t)a;
    }
}

// --- Lava ---

TextureLavaFX::TextureLavaFX() : TextureFX(237) {
}

void TextureLavaFX::onTick() {
    for (int i = 0; i < 16; ++i) {
        for (int j = 0; j < 16; ++j) {
            float var3 = 0.0f;
            int var4 = (int)(std::sin((float)j * (float)M_PI * 2.0f / 16.0f) * 1.2f);
            int var5 = (int)(std::sin((float)i * (float)M_PI * 2.0f / 16.0f) * 1.2f);

            for (int k = i - 1; k <= i + 1; ++k) {
                for (int l = j - 1; l <= j + 1; ++l) {
                    int var8 = (k + var4) & 15;
                    int var9 = (l + var5) & 15;
                    var3 += red[var8 + var9 * 16];
                }
            }

            green[i + j * 16] = var3 / 10.0f + (blue[((i + 0) & 15) + ((j + 0) & 15) * 16] + blue[((i + 1) & 15) + ((j + 0) & 15) * 16] + blue[((i + 1) & 15) + ((j + 1) & 15) * 16] + blue[((i + 0) & 15) + ((j + 1) & 15) * 16]) / 4.0f * 0.8f;
            blue[i + j * 16] += alpha[i + j * 16] * 0.01f;
            if (blue[i + j * 16] < 0.0f) blue[i + j * 16] = 0.0f;
            alpha[i + j * 16] -= 0.06f;
            if ((float)rand() / (float)RAND_MAX < 0.005f) {
                alpha[i + j * 16] = 1.5f;
            }
        }
    }

    for (int i = 0; i < 256; ++i) red[i] = green[i];

    for (int i = 0; i < 256; ++i) {
        float var3 = red[i] * 2.0f;
        if (var3 > 1.0f) var3 = 1.0f;
        if (var3 < 0.0f) var3 = 0.0f;
        int r = (int)(var3 * 100.0f + 155.0f);
        int g = (int)(var3 * var3 * 255.0f);
        int b = (int)(var3 * var3 * var3 * var3 * 128.0f);
        
        imageData[i * 4 + 0] = (uint8_t)r;
        imageData[i * 4 + 1] = (uint8_t)g;
        imageData[i * 4 + 2] = (uint8_t)b;
        imageData[i * 4 + 3] = 255;
    }
}

TextureLavaFlowFX::TextureLavaFlowFX() : TextureFX(238) {
}

void TextureLavaFlowFX::onTick() {
    tickCounter++;
    for (int i = 0; i < 16; ++i) {
        for (int j = 0; j < 16; ++j) {
            float var3 = 0.0f;
            int var4 = (int)(std::sin((float)j * (float)M_PI * 2.0f / 16.0f) * 1.2f);
            int var5 = (int)(std::sin((float)i * (float)M_PI * 2.0f / 16.0f) * 1.2f);

            for (int k = i - 1; k <= i + 1; ++k) {
                for (int l = j - 1; l <= j + 1; ++l) {
                    int var8 = (k + var4) & 15;
                    int var9 = (l + var5) & 15;
                    var3 += red[var8 + var9 * 16];
                }
            }

            green[i + j * 16] = var3 / 10.0f + (blue[((i + 0) & 15) + ((j + 0) & 15) * 16] + blue[((i + 1) & 15) + ((j + 0) & 15) * 16] + blue[((i + 1) & 15) + ((j + 1) & 15) * 16] + blue[((i + 0) & 15) + ((j + 1) & 15) * 16]) / 4.0f * 0.8f;
            blue[i + j * 16] += alpha[i + j * 16] * 0.01f;
            if (blue[i + j * 16] < 0.0f) blue[i + j * 16] = 0.0f;
            alpha[i + j * 16] -= 0.06f;
            if ((float)rand() / (float)RAND_MAX < 0.005f) {
                alpha[i + j * 16] = 1.5f;
            }
        }
    }

    for (int i = 0; i < 256; ++i) red[i] = green[i];

    for (int i = 0; i < 256; ++i) {
        float var3 = red[(i - (tickCounter / 3) * 16) & 255] * 2.0f;
        if (var3 > 1.0f) var3 = 1.0f;
        if (var3 < 0.0f) var3 = 0.0f;
        int r = (int)(var3 * 100.0f + 155.0f);
        int g = (int)(var3 * var3 * 255.0f);
        int b = (int)(var3 * var3 * var3 * var3 * 128.0f);
        
        imageData[i * 4 + 0] = (uint8_t)r;
        imageData[i * 4 + 1] = (uint8_t)g;
        imageData[i * 4 + 2] = (uint8_t)b;
        imageData[i * 4 + 3] = 255;
    }
}