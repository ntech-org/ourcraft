#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <mutex>
#include <memory>

class RegionFile {
public:
    static constexpr int REGION_SIZE = 32;
    static constexpr int SECTOR_SIZE = 4096;

    RegionFile(const std::string& path);
    ~RegionFile();

    bool hasChunk(int x, int z) const;
    std::vector<uint8_t> readChunk(int x, int z);
    void writeChunk(int x, int z, const uint8_t* data, size_t length);

private:
    std::string m_path;
    mutable std::fstream m_file;
    mutable std::mutex m_mutex;

    uint32_t m_offsets[1024];
    uint32_t m_timestamps[1024];
    std::vector<bool> m_sectorFree;

    void loadHeader();
    int getOffset(int x, int z) const { return m_offsets[(x & 31) + (z & 31) * 32]; }
};
