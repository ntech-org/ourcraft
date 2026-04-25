#include "world/nbt/NBT.hpp"
#include <zlib.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>

namespace nbt {

// Helper to swap endianness (NBT is big-endian)
static uint16_t swap16(uint16_t val) { return (val << 8) | (val >> 8); }
static uint32_t swap32(uint32_t val) {
    return ((val >> 24) & 0xff) | ((val << 8) & 0xff0000) | ((val >> 8) & 0xff00) | ((val << 24) & 0xff000000);
}
static uint64_t swap64(uint64_t val) {
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL) | ((val >> 8) & 0x00FF00FF00FF00FFULL);
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL) | ((val >> 16) & 0x0000FFFF0000FFFFULL);
    return (val << 32) | (val >> 32);
}

template<typename T>
T readBinary(std::istream& in) {
    T val;
    in.read(reinterpret_cast<char*>(&val), sizeof(T));
    if constexpr (sizeof(T) == 2) { uint16_t v = swap16(*(uint16_t*)&val); return *(T*)&v; }
    if constexpr (sizeof(T) == 4) { uint32_t v = swap32(*(uint32_t*)&val); return *(T*)&v; }
    if constexpr (sizeof(T) == 8) { uint64_t v = swap64(*(uint64_t*)&val); return *(T*)&v; }
    return val;
}

std::string readString(std::istream& in) {
    uint16_t len = readBinary<uint16_t>(in);
    std::string s(len, '\0');
    in.read(&s[0], len);
    return s;
}

std::shared_ptr<Tag> readTagInternal(std::istream& in, TagType type, std::string name) {
    auto tag = std::make_shared<Tag>(type, name);
    switch (type) {
        case TagType::Byte: tag->value = readBinary<int8_t>(in); break;
        case TagType::Short: tag->value = readBinary<int16_t>(in); break;
        case TagType::Int: tag->value = readBinary<int32_t>(in); break;
        case TagType::Long: tag->value = readBinary<int64_t>(in); break;
        case TagType::Float: tag->value = readBinary<float>(in); break;
        case TagType::Double: tag->value = readBinary<double>(in); break;
        case TagType::ByteArray: {
            int32_t len = readBinary<int32_t>(in);
            std::vector<int8_t> data(len);
            in.read(reinterpret_cast<char*>(data.data()), len);
            tag->value = data;
            break;
        }
        case TagType::String: tag->value = readString(in); break;
        case TagType::List: {
            TagType listType = static_cast<TagType>(readBinary<uint8_t>(in));
            int32_t len = readBinary<int32_t>(in);
            List list; list.type = listType;
            for (int i = 0; i < len; ++i) list.elements.push_back(readTagInternal(in, listType, ""));
            tag->value = list;
            break;
        }
        case TagType::Compound: {
            Compound compound;
            while (true) {
                uint8_t t = readBinary<uint8_t>(in);
                if (t == 0) break;
                std::string n = readString(in);
                compound[n] = readTagInternal(in, static_cast<TagType>(t), n);
            }
            tag->value = compound;
            break;
        }
        case TagType::IntArray: {
            int32_t len = readBinary<int32_t>(in);
            std::vector<int32_t> data(len);
            for (int i = 0; i < len; ++i) data[i] = readBinary<int32_t>(in);
            tag->value = data;
            break;
        }
        default: break;
    }
    return tag;
}

std::shared_ptr<Tag> readTag(std::istream& in) {
    uint8_t type = readBinary<uint8_t>(in);
    if (type == 0) return nullptr;
    std::string name = readString(in);
    return readTagInternal(in, static_cast<TagType>(type), name);
}

// Implement compressed reading using zlib
std::shared_ptr<Tag> readCompressed(const std::string& filename) {
    gzFile file = gzopen(filename.c_str(), "rb");
    if (!file) return nullptr;

    std::vector<char> buffer;
    char tmp[4096];
    int read;
    while ((read = gzread(file, tmp, sizeof(tmp))) > 0) {
        buffer.insert(buffer.end(), tmp, tmp + read);
    }
    gzclose(file);

    std::string data(buffer.begin(), buffer.end());
    std::istringstream iss(data, std::ios::binary);
    return readTag(iss);
}

// Minimal writing implementation omitted for brevity in this step, but needed for conversion
void writeTag(std::ostream& out, const Tag& tag) {
    // TODO: Implement if needed for saving back to NBT
}

} // namespace nbt
