#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace UTF8 {
    inline std::u32string toUTF32(const std::string& str) {
        std::u32string result;
        for (size_t i = 0; i < str.length(); ++i) {
            uint32_t cp = 0;
            unsigned char c = (unsigned char)str[i];

            if (c <= 0x7F) {
                cp = c;
            } else if ((c & 0xE0) == 0xC0) {
                cp = (c & 0x1F) << 6;
                cp |= (str[++i] & 0x3F);
            } else if ((c & 0xF0) == 0xE0) {
                cp = (c & 0x0F) << 12;
                cp |= (str[++i] & 0x3F) << 6;
                cp |= (str[++i] & 0x3F);
            } else if ((c & 0xF8) == 0xF0) {
                cp = (c & 0x07) << 18;
                cp |= (str[++i] & 0x3F) << 12;
                cp |= (str[++i] & 0x3F) << 6;
                cp |= (str[++i] & 0x3F);
            }
            result.push_back(cp);
        }
        return result;
    }
}
