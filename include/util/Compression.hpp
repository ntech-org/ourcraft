#pragma once

#include <vector>
#include <cstdint>
#include <zstd.h>
#include <stdexcept>

class Compression {
public:
    static void compress(const std::vector<uint8_t>& input, std::vector<uint8_t>& output) {
        size_t const bound = ZSTD_compressBound(input.size());
        output.resize(bound);
        
        size_t const cSize = ZSTD_compress(output.data(), bound, input.data(), input.size(), 1);
        if (ZSTD_isError(cSize)) {
            throw std::runtime_error(std::string("ZSTD compression failed: ") + ZSTD_getErrorName(cSize));
        }
        output.resize(cSize);
    }

    static void decompress(const uint8_t* input, size_t inputSize, std::vector<uint8_t>& output, size_t expectedSize) {
        output.resize(expectedSize);
        size_t const dSize = ZSTD_decompress(output.data(), expectedSize, input, inputSize);
        
        if (ZSTD_isError(dSize)) {
            throw std::runtime_error(std::string("ZSTD decompression failed: ") + ZSTD_getErrorName(dSize));
        }
        if (dSize != expectedSize) {
            throw std::runtime_error("ZSTD decompression size mismatch");
        }
    }
};
