#pragma once

#include "renderer/Bounds.hpp"
#include <cstddef>
#include <cstdint>
#include <glad/glad.h>
#include <vector>

struct TerrainVertex {
    float x, y, z;
    float tileU, tileV;
    std::uint32_t color;
    std::uint32_t textureIndex;
    std::uint32_t faceId;
    float flowRotation;
    float liquidType; // 0: None, 1: Water, 2: Lava
    float isUnderwater;
};

struct ChunkMeshData {
    struct Pass {
        std::vector<TerrainVertex> vertices;
        std::vector<std::uint32_t> indices;
        std::size_t quadCount = 0;
    };

    Pass opaque;
    Pass translucent;
    AABB bounds {};

    bool empty() const { return opaque.indices.empty() && translucent.indices.empty(); }
};

class ChunkMesh {
public:
    ChunkMesh() = default;
    ~ChunkMesh();

    ChunkMesh(const ChunkMesh&) = delete;
    ChunkMesh& operator=(const ChunkMesh&) = delete;

    ChunkMesh(ChunkMesh&& other) noexcept;
    ChunkMesh& operator=(ChunkMesh&& other) noexcept;

    void upload(const ChunkMeshData::Pass& pass);
    void clear();
    void draw() const;

    bool hasGeometry() const { return m_indexCount > 0; }
    std::size_t getTriangleCount() const { return m_triangleCount; }

private:
    void ensureAllocated();

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;
    GLsizei m_indexCount = 0;
    std::size_t m_triangleCount = 0;
};
