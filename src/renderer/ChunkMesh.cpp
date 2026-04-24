#include "renderer/ChunkMesh.hpp"
#include <utility>

ChunkMesh::~ChunkMesh() {
    clear();
}

ChunkMesh::ChunkMesh(ChunkMesh&& other) noexcept {
    *this = std::move(other);
}

ChunkMesh& ChunkMesh::operator=(ChunkMesh&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    clear();

    m_vao = std::exchange(other.m_vao, 0);
    m_vbo = std::exchange(other.m_vbo, 0);
    m_ebo = std::exchange(other.m_ebo, 0);
    m_indexCount = std::exchange(other.m_indexCount, 0);
    m_triangleCount = std::exchange(other.m_triangleCount, 0);
    return *this;
}

void ChunkMesh::upload(const ChunkMeshData::Pass& pass) {
    ensureAllocated();

    m_indexCount = static_cast<GLsizei>(pass.indices.size());
    m_triangleCount = pass.indices.size() / 3;

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(pass.vertices.size() * sizeof(TerrainVertex)),
        pass.vertices.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(pass.indices.size() * sizeof(std::uint32_t)),
        pass.indices.data(),
        GL_STATIC_DRAW
    );

    glBindVertexArray(0);
}

void ChunkMesh::clear() {
    if (m_ebo != 0) {
        glDeleteBuffers(1, &m_ebo);
        m_ebo = 0;
    }
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }

    m_indexCount = 0;
    m_triangleCount = 0;
}

void ChunkMesh::draw() const {
    if (m_indexCount == 0) {
        return;
    }

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
}

void ChunkMesh::ensureAllocated() {
    if (m_vao != 0) {
        return;
    }

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), reinterpret_cast<void*>(offsetof(TerrainVertex, x)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), reinterpret_cast<void*>(offsetof(TerrainVertex, tileU)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(TerrainVertex), reinterpret_cast<void*>(offsetof(TerrainVertex, color)));

    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, sizeof(TerrainVertex), reinterpret_cast<void*>(offsetof(TerrainVertex, textureIndex)));

    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(4, 1, GL_UNSIGNED_INT, sizeof(TerrainVertex), reinterpret_cast<void*>(offsetof(TerrainVertex, faceId)));

    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), reinterpret_cast<void*>(offsetof(TerrainVertex, flowRotation)));

    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), reinterpret_cast<void*>(offsetof(TerrainVertex, liquidType)));

    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), reinterpret_cast<void*>(offsetof(TerrainVertex, isUnderwater)));

    glBindVertexArray(0);
}
