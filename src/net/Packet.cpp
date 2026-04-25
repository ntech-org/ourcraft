#include "net/Packet.hpp"
#include <cstring>

void Packet::writeInt(std::vector<uint8_t>& buffer, int32_t value) {
    uint8_t bytes[4];
    std::memcpy(bytes, &value, 4);
    for (int i = 0; i < 4; ++i) buffer.push_back(bytes[i]);
}

void Packet::writeFloat(std::vector<uint8_t>& buffer, float value) {
    uint8_t bytes[4];
    std::memcpy(bytes, &value, 4);
    for (int i = 0; i < 4; ++i) buffer.push_back(bytes[i]);
}

void Packet::writeDouble(std::vector<uint8_t>& buffer, double value) {
    uint8_t bytes[8];
    std::memcpy(bytes, &value, 8);
    for (int i = 0; i < 8; ++i) buffer.push_back(bytes[i]);
}

void Packet::writeString(std::vector<uint8_t>& buffer, const std::string& value) {
    writeInt(buffer, (int32_t)value.size());
    for (char c : value) buffer.push_back((uint8_t)c);
}

void Packet::writeByte(std::vector<uint8_t>& buffer, uint8_t value) {
    buffer.push_back(value);
}

void Packet::writeBytes(std::vector<uint8_t>& buffer, const uint8_t* data, size_t size) {
    size_t currentSize = buffer.size();
    buffer.resize(currentSize + size);
    std::memcpy(buffer.data() + currentSize, data, size);
}

int32_t Packet::readInt(const uint8_t*& data) {
    int32_t value;
    std::memcpy(&value, data, 4);
    data += 4;
    return value;
}

float Packet::readFloat(const uint8_t*& data) {
    float value;
    std::memcpy(&value, data, 4);
    data += 4;
    return value;
}

double Packet::readDouble(const uint8_t*& data) {
    double value;
    std::memcpy(&value, data, 8);
    data += 8;
    return value;
}

std::string Packet::readString(const uint8_t*& data) {
    int32_t size = readInt(data);
    std::string value((const char*)data, size);
    data += size;
    return value;
}

uint8_t Packet::readByte(const uint8_t*& data) {
    uint8_t value = *data;
    data += 1;
    return value;
}

void Packet::readBytes(const uint8_t*& data, uint8_t* target, size_t size) {
    std::memcpy(target, data, size);
    data += size;
}
