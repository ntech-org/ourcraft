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

    if (buffer.empty()) return nullptr;

    std::string data(buffer.begin(), buffer.end());
    std::istringstream iss(data, std::ios::binary);
    
    try {
        return readTag(iss);
    } catch (const std::exception& e) {
        std::cerr << "NBT Error: Failed to parse " << filename << ": " << e.what() << std::endl;
        return nullptr;
    }
}

template<typename T>
void writeBinary(std::ostream& out, T val) {
    if constexpr (sizeof(T) == 2) { uint16_t v = swap16(*(uint16_t*)&val); out.write(reinterpret_cast<const char*>(&v), 2); }
    else if constexpr (sizeof(T) == 4) { uint32_t v = swap32(*(uint32_t*)&val); out.write(reinterpret_cast<const char*>(&v), 4); }
    else if constexpr (sizeof(T) == 8) { uint64_t v = swap64(*(uint64_t*)&val); out.write(reinterpret_cast<const char*>(&v), 8); }
    else out.write(reinterpret_cast<const char*>(&val), sizeof(T));
}

void writeString(std::ostream& out, const std::string& s) {
    writeBinary<uint16_t>(out, static_cast<uint16_t>(s.length()));
    out.write(s.data(), s.length());
}

void writeTagInternal(std::ostream& out, const Tag& tag, bool writeName = true) {
    if (writeName) {
        writeBinary<uint8_t>(out, static_cast<uint8_t>(tag.type));
        writeString(out, tag.name);
    }

    switch (tag.type) {
        case TagType::Byte: writeBinary<int8_t>(out, std::get<int8_t>(tag.value)); break;
        case TagType::Short: writeBinary<int16_t>(out, std::get<int16_t>(tag.value)); break;
        case TagType::Int: writeBinary<int32_t>(out, std::get<int32_t>(tag.value)); break;
        case TagType::Long: writeBinary<int64_t>(out, std::get<int64_t>(tag.value)); break;
        case TagType::Float: writeBinary<float>(out, std::get<float>(tag.value)); break;
        case TagType::Double: writeBinary<double>(out, std::get<double>(tag.value)); break;
        case TagType::ByteArray: {
            const auto& data = std::get<std::vector<int8_t>>(tag.value);
            writeBinary<int32_t>(out, static_cast<int32_t>(data.size()));
            out.write(reinterpret_cast<const char*>(data.data()), data.size());
            break;
        }
        case TagType::String: writeString(out, std::get<std::string>(tag.value)); break;
        case TagType::List: {
            const auto& list = std::get<List>(tag.value);
            writeBinary<uint8_t>(out, static_cast<uint8_t>(list.type));
            writeBinary<int32_t>(out, static_cast<int32_t>(list.elements.size()));
            for (const auto& element : list.elements) writeTagInternal(out, *element, false);
            break;
        }
        case TagType::Compound: {
            const auto& compound = std::get<Compound>(tag.value);
            for (const auto& [name, subTag] : compound) {
                writeTagInternal(out, *subTag, true);
            }
            writeBinary<uint8_t>(out, 0); // Tag_End
            break;
        }
        case TagType::IntArray: {
            const auto& data = std::get<std::vector<int32_t>>(tag.value);
            writeBinary<int32_t>(out, static_cast<int32_t>(data.size()));
            for (auto v : data) writeBinary<int32_t>(out, v);
            break;
        }
        default: break;
    }
}

void writeTag(std::ostream& out, const Tag& tag) {
    writeTagInternal(out, tag, true);
}

void writeCompressed(const std::string& filename, const Tag& tag) {
    std::ostringstream oss(std::ios::binary);
    writeTag(oss, tag);
    std::string data = oss.str();

    gzFile file = gzopen(filename.c_str(), "wb");
    if (file) {
        gzwrite(file, data.data(), static_cast<unsigned int>(data.size()));
        gzclose(file);
    }
}

} // namespace nbt
