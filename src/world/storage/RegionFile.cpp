#include "world/storage/RegionFile.hpp"
#include <zlib.h>
#include <iostream>
#include <ctime>
#include <algorithm>

RegionFile::RegionFile(const std::string& path) : m_path(path) {
    m_file.open(path, std::ios::in | std::ios::out | std::ios::binary);
    if (!m_file.is_open()) {
        m_file.clear();
        m_file.open(path, std::ios::out | std::ios::binary);
        if (m_file.is_open()) {
            std::vector<uint8_t> empty(SECTOR_SIZE * 2, 0);
            m_file.write((char*)empty.data(), empty.size());
            m_file.flush();
            m_file.close();
            m_file.open(path, std::ios::in | std::ios::out | std::ios::binary);
        }
    }
    
    if (m_file.is_open()) {
        m_file.seekg(0, std::ios::end);
        size_t size = m_file.tellg();
        if (size < SECTOR_SIZE * 2) {
            m_file.seekg(0);
            std::vector<uint8_t> empty(SECTOR_SIZE * 2, 0);
            m_file.write((char*)empty.data(), empty.size());
            m_file.flush();
            size = SECTOR_SIZE * 2;
        }
        
        m_sectorFree.clear();
        m_sectorFree.resize(size / SECTOR_SIZE, true);
        if (m_sectorFree.size() >= 2) {
            m_sectorFree[0] = false; // Offsets
            m_sectorFree[1] = false; // Timestamps
        }
        
        loadHeader();
    } else {
        std::cerr << "RegionFile: Failed to open " << path << std::endl;
    }
}

RegionFile::~RegionFile() {
    if (m_file.is_open()) m_file.close();
}

void RegionFile::loadHeader() {
    m_file.seekg(0);
    for (int i = 0; i < 1024; ++i) {
        uint32_t val;
        m_file.read((char*)&val, 4);
        // Big endian to host
        uint32_t hostVal = ((val >> 24) & 0xff) | ((val << 8) & 0xff0000) | ((val >> 8) & 0xff00) | ((val << 24) & 0xff000000);
        m_offsets[i] = hostVal;
        
        int offset = hostVal >> 8;
        int sectors = hostVal & 0xff;
        if (offset > 0 && offset + sectors <= (int)m_sectorFree.size()) {
            for (int j = 0; j < sectors; ++j) m_sectorFree[offset + j] = false;
        }
    }
    for (int i = 0; i < 1024; ++i) {
        uint32_t val;
        m_file.read((char*)&val, 4);
        m_timestamps[i] = ((val >> 24) & 0xff) | ((val << 8) & 0xff0000) | ((val >> 8) & 0xff00) | ((val << 24) & 0xff000000);
    }
}

bool RegionFile::hasChunk(int x, int z) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return getOffset(x, z) != 0;
}

std::vector<uint8_t> RegionFile::readChunk(int x, int z) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_file.is_open()) return {};
    
    uint32_t offsetVal = getOffset(x, z);
    if (offsetVal == 0) return {};
    
    int offset = offsetVal >> 8;
    int sectors = offsetVal & 0xff;
    
    if (offset + sectors > (int)m_sectorFree.size()) return {};
    
    m_file.seekg(offset * SECTOR_SIZE);
    uint32_t length;
    m_file.read((char*)&length, 4);
    length = ((length >> 24) & 0xff) | ((length << 8) & 0xff0000) | ((length >> 8) & 0xff00) | ((length << 24) & 0xff000000);
    
    if (length > (uint32_t)sectors * SECTOR_SIZE) return {};
    if (length == 0) return {};
    
    uint8_t version;
    m_file.read((char*)&version, 1);
    
    std::vector<uint8_t> compressed(length - 1);
    m_file.read((char*)compressed.data(), length - 1);
    
    // Decompress (zlib)
    std::vector<uint8_t> decompressed;
    z_stream strm;
    strm.zalloc = Z_NULL; strm.zfree = Z_NULL; strm.opaque = Z_NULL;
    strm.avail_in = compressed.size();
    strm.next_in = (Bytef*)compressed.data();
    inflateInit(&strm);
    
    uint8_t out[4096];
    do {
        strm.avail_out = sizeof(out);
        strm.next_out = out;
        if (inflate(&strm, Z_NO_FLUSH) < 0) break;
        decompressed.insert(decompressed.end(), out, out + (sizeof(out) - strm.avail_out));
    } while (strm.avail_out == 0);
    inflateEnd(&strm);
    
    return decompressed;
}

void RegionFile::writeChunk(int x, int z, const uint8_t* data, size_t length) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_file.is_open() || m_sectorFree.empty()) return;
    
    // Compress (zlib)
    std::vector<uint8_t> compressed;
    z_stream strm;
    strm.zalloc = Z_NULL; strm.zfree = Z_NULL; strm.opaque = Z_NULL;
    deflateInit(&strm, Z_DEFAULT_COMPRESSION);
    strm.avail_in = length;
    strm.next_in = (Bytef*)data;
    
    uint8_t out[4096];
    do {
        strm.avail_out = sizeof(out);
        strm.next_out = out;
        deflate(&strm, Z_FINISH);
        compressed.insert(compressed.end(), out, out + (sizeof(out) - strm.avail_out));
    } while (strm.avail_out == 0);
    deflateEnd(&strm);
    
    uint32_t fullLength = compressed.size() + 1;
    int sectorsNeeded = (fullLength + 4 + SECTOR_SIZE - 1) / SECTOR_SIZE;
    
    // Find space (very simple allocation for now)
    int offset = -1;
    uint32_t oldOffsetVal = getOffset(x, z);
    if (oldOffsetVal != 0) {
        int oldOffset = oldOffsetVal >> 8;
        int oldSectors = oldOffsetVal & 0xff;
        if (sectorsNeeded <= oldSectors) {
            offset = oldOffset;
        } else {
            for (int i = 0; i < oldSectors; ++i) {
                if (oldOffset + i < (int)m_sectorFree.size()) m_sectorFree[oldOffset + i] = true;
            }
        }
    }
    
    if (offset == -1) {
        for (int i = 0; i <= (int)m_sectorFree.size() - sectorsNeeded; ++i) {
            bool free = true;
            for (int j = 0; j < sectorsNeeded; ++j) if (!m_sectorFree[i + j]) { free = false; break; }
            if (free) { offset = i; break; }
        }
        if (offset == -1) {
            offset = m_sectorFree.size();
            m_sectorFree.resize(offset + sectorsNeeded, false);
        }
    }
    
    for (int i = 0; i < sectorsNeeded; ++i) {
        if (offset + i < (int)m_sectorFree.size()) m_sectorFree[offset + i] = false;
    }
    
    m_file.seekp(offset * SECTOR_SIZE);
    uint32_t lenOut = ((fullLength >> 24) & 0xff) | ((fullLength << 8) & 0xff0000) | ((fullLength >> 8) & 0xff00) | ((fullLength << 24) & 0xff000000);
    m_file.write((char*)&lenOut, 4);
    uint8_t version = 2; // zlib
    m_file.write((char*)&version, 1);
    m_file.write((char*)compressed.data(), compressed.size());
    
    // Update header
    uint32_t newOffsetVal = (offset << 8) | (sectorsNeeded & 0xff);
    m_offsets[(x & 31) + (z & 31) * 32] = newOffsetVal;
    m_timestamps[(x & 31) + (z & 31) * 32] = (uint32_t)std::time(nullptr);
    
    m_file.seekp(((x & 31) + (z & 31) * 32) * 4);
    uint32_t offsetOut = ((newOffsetVal >> 24) & 0xff) | ((newOffsetVal << 8) & 0xff0000) | ((newOffsetVal >> 8) & 0xff00) | ((newOffsetVal << 24) & 0xff000000);
    m_file.write((char*)&offsetOut, 4);
    
    m_file.seekp(SECTOR_SIZE + ((x & 31) + (z & 31) * 32) * 4);
    uint32_t ts = m_timestamps[(x & 31) + (z & 31) * 32];
    uint32_t tsOut = ((ts >> 24) & 0xff) | ((ts << 8) & 0xff0000) | ((ts >> 8) & 0xff00) | ((ts << 24) & 0xff000000);
    m_file.write((char*)&tsOut, 4);
    
    m_file.flush();
}
