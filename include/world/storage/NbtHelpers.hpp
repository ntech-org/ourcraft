#pragma once

#include "world/nbt/NBT.hpp"
#include <memory>
#include <string>
#include <sstream>

inline std::shared_ptr<nbt::Tag> makeNbtCompound(const std::string& name = "") {
    auto tag = std::make_shared<nbt::Tag>(nbt::TagType::Compound, name);
    tag->value = nbt::Compound();
    return tag;
}

inline std::shared_ptr<nbt::Tag> makeNbtList(const std::string& name, nbt::TagType elementType) {
    auto tag = std::make_shared<nbt::Tag>(nbt::TagType::List, name);
    nbt::List list;
    list.type = elementType;
    tag->value = list;
    return tag;
}

inline void setNbtLong(nbt::Compound& map, const std::string& key, int64_t value) {
    auto tag = std::make_shared<nbt::Tag>(nbt::TagType::Long, key);
    tag->value = value;
    map[key] = tag;
}

inline void setNbtInt(nbt::Compound& map, const std::string& key, int32_t value) {
    auto tag = std::make_shared<nbt::Tag>(nbt::TagType::Int, key);
    tag->value = value;
    map[key] = tag;
}

inline void setNbtFloat(nbt::Compound& map, const std::string& key, float value) {
    auto tag = std::make_shared<nbt::Tag>(nbt::TagType::Float, key);
    tag->value = value;
    map[key] = tag;
}

inline void setNbtDouble(nbt::Compound& map, const std::string& key, double value) {
    auto tag = std::make_shared<nbt::Tag>(nbt::TagType::Double, key);
    tag->value = value;
    map[key] = tag;
}

inline void setNbtShort(nbt::Compound& map, const std::string& key, int16_t value) {
    auto tag = std::make_shared<nbt::Tag>(nbt::TagType::Short, key);
    tag->value = value;
    map[key] = tag;
}

inline void setNbtByte(nbt::Compound& map, const std::string& key, int8_t value) {
    auto tag = std::make_shared<nbt::Tag>(nbt::TagType::Byte, key);
    tag->value = value;
    map[key] = tag;
}

inline void setNbtString(nbt::Compound& map, const std::string& key, const std::string& value) {
    auto tag = std::make_shared<nbt::Tag>(nbt::TagType::String, key);
    tag->value = value;
    map[key] = tag;
}

inline void addNbtDouble(nbt::List& list, double value) {
    auto t = std::make_shared<nbt::Tag>(nbt::TagType::Double, "");
    t->value = value;
    list.elements.push_back(t);
}

inline void addNbtFloat(nbt::List& list, float value) {
    auto t = std::make_shared<nbt::Tag>(nbt::TagType::Float, "");
    t->value = value;
    list.elements.push_back(t);
}

inline bool getNbtLong(const nbt::Compound& map, const std::string& key, int64_t& val) {
    auto it = map.find(key);
    if (it != map.end() && it->second->type == nbt::TagType::Long) {
        val = std::get<int64_t>(it->second->value);
        return true;
    }
    return false;
}

inline bool getNbtInt(const nbt::Compound& map, const std::string& key, int32_t& val) {
    auto it = map.find(key);
    if (it != map.end() && it->second->type == nbt::TagType::Int) {
        val = std::get<int32_t>(it->second->value);
        return true;
    }
    return false;
}

inline bool getNbtFloat(const nbt::Compound& map, const std::string& key, float& val) {
    auto it = map.find(key);
    if (it != map.end() && it->second->type == nbt::TagType::Float) {
        val = std::get<float>(it->second->value);
        return true;
    }
    return false;
}

inline bool getNbtDouble(const nbt::Compound& map, const std::string& key, double& val) {
    auto it = map.find(key);
    if (it != map.end() && it->second->type == nbt::TagType::Double) {
        val = std::get<double>(it->second->value);
        return true;
    }
    return false;
}

inline bool getNbtShort(const nbt::Compound& map, const std::string& key, int16_t& val) {
    auto it = map.find(key);
    if (it != map.end() && it->second->type == nbt::TagType::Short) {
        val = std::get<int16_t>(it->second->value);
        return true;
    }
    return false;
}

inline bool getNbtByte(const nbt::Compound& map, const std::string& key, int8_t& val) {
    auto it = map.find(key);
    if (it != map.end() && it->second->type == nbt::TagType::Byte) {
        val = std::get<int8_t>(it->second->value);
        return true;
    }
    return false;
}

inline std::string nbtToString(const nbt::Tag& tag) {
    std::ostringstream oss(std::ios::binary);
    nbt::writeTag(oss, tag);
    return oss.str();
}

inline std::shared_ptr<nbt::Tag> stringToNbt(const std::string& str) {
    std::istringstream iss(str, std::ios::binary);
    return nbt::readTag(iss);
}
