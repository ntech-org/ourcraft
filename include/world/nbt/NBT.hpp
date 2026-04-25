#pragma once
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <memory>
#include <iostream>

namespace nbt {

enum class TagType : uint8_t {
    End = 0,
    Byte = 1,
    Short = 2,
    Int = 3,
    Long = 4,
    Float = 5,
    Double = 6,
    ByteArray = 7,
    String = 8,
    List = 9,
    Compound = 10,
    IntArray = 11,
    LongArray = 12
};

struct Tag;

using Compound = std::map<std::string, std::shared_ptr<Tag>>;
struct List {
    TagType type;
    std::vector<std::shared_ptr<Tag>> elements;
};

struct Tag {
    TagType type;
    std::string name;
    std::variant<int8_t, int16_t, int32_t, int64_t, float, double, std::vector<int8_t>, std::string, List, Compound, std::vector<int32_t>, std::vector<int64_t>> value;

    Tag(TagType t, std::string n) : type(t), name(n) {}
};

std::shared_ptr<Tag> readTag(std::istream& in);
void writeTag(std::ostream& out, const Tag& tag);

std::shared_ptr<Tag> readCompressed(const std::string& filename);
void writeCompressed(const std::string& filename, const Tag& tag);

} // namespace nbt
