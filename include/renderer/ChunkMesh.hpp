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
};

struct ChunkMeshData {
    std::vector<TerrainVertex> vertices;
    std::vector<std::uint32_t> indices;
    std::size_t quadCount = 0;
    AABB bounds {};

    bool empty() const { return indices.empty(); }
};

class ChunkMesh {
public:
    ChunkMesh() = default;
    ~ChunkMesh();

    ChunkMesh(const ChunkMesh&) = delete;
    ChunkMesh& operator=(const ChunkMesh&) = delete;

    ChunkMesh(ChunkMesh&& other) noexcept;
    ChunkMesh& operator=(ChunkMesh&& other) noexcept;

    void upload(const ChunkMeshData& meshData);
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
