#include "renderer/BatchedMesh.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>

BatchedMesh::~BatchedMesh() {
    if (m_ebo) glDeleteBuffers(1, &m_ebo);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_indirectBuffer) glDeleteBuffers(1, &m_indirectBuffer);
}

bool BatchedMesh::init(std::size_t vboCapacity, std::size_t iboCapacity) {
    if (m_vao) return true;

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vboCapacity), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(iboCapacity), nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex),
                          reinterpret_cast<void*>(offsetof(TerrainVertex, x)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex),
                          reinterpret_cast<void*>(offsetof(TerrainVertex, tileU)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(TerrainVertex),
                          reinterpret_cast<void*>(offsetof(TerrainVertex, color)));
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, sizeof(TerrainVertex),
                           reinterpret_cast<void*>(offsetof(TerrainVertex, textureIndex)));
    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(4, 1, GL_UNSIGNED_INT, sizeof(TerrainVertex),
                           reinterpret_cast<void*>(offsetof(TerrainVertex, faceId)));
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex),
                          reinterpret_cast<void*>(offsetof(TerrainVertex, flowRotation)));
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex),
                          reinterpret_cast<void*>(offsetof(TerrainVertex, liquidType)));
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex),
                          reinterpret_cast<void*>(offsetof(TerrainVertex, isUnderwater)));
    glEnableVertexAttribArray(8);
    glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex),
                          reinterpret_cast<void*>(offsetof(TerrainVertex, skyLight)));
    glEnableVertexAttribArray(9);
    glVertexAttribPointer(9, 1, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex),
                          reinterpret_cast<void*>(offsetof(TerrainVertex, blockLight)));

    glBindVertexArray(0);

    m_vboCapacity = vboCapacity;
    m_eboCapacity = iboCapacity;
    m_vboFree.push_back({0, vboCapacity});
    m_eboFree.push_back({0, iboCapacity});

    return true;
}

std::size_t BatchedMesh::totalFreeBytes(const std::vector<FreeBlock>& freeList) {
    std::size_t total = 0;
    for (const auto& b : freeList) total += b.size;
    return total;
}

std::size_t BatchedMesh::allocateBlock(std::vector<FreeBlock>& freeList, std::size_t size) {
    auto best = freeList.end();
    std::size_t bestSize = SIZE_MAX;
    for (auto it = freeList.begin(); it != freeList.end(); ++it) {
        if (it->size >= size && it->size < bestSize) {
            best = it;
            bestSize = it->size;
            if (bestSize == size) break;
        }
    }
    if (best == freeList.end()) return SIZE_MAX;
    std::size_t offset = best->offset;
    if (best->size == size) {
        freeList.erase(best);
    } else {
        best->offset += size;
        best->size -= size;
    }
    return offset;
}

void BatchedMesh::freeBlock(std::vector<FreeBlock>& freeList, std::size_t offset, std::size_t size) {
    auto it = std::lower_bound(freeList.begin(), freeList.end(), offset,
                               [](const FreeBlock& b, std::size_t off) { return b.offset < off; });

    bool mergedForward = false;

    // Try to merge with the next block
    if (it != freeList.end() && it->offset == offset + size) {
        it->offset = offset;
        it->size += size;
        mergedForward = true;
    }

    // Try to merge with the previous block
    if (it != freeList.begin()) {
        auto prev = it - 1;
        if (prev->offset + prev->size == offset) {
            if (mergedForward) {
                // 'it' already has size + next_block; merge all of it into prev
                prev->size += it->size;
                freeList.erase(it);
            } else {
                prev->size += size;
            }
            return;
        }
    }

    // No backward merge; if forward merge happened, 'it' already covers the region correctly
    if (!mergedForward) {
        freeList.insert(it, {offset, size});
    }
}

bool BatchedMesh::growBuffer(GLenum target, GLuint& buffer, std::size_t& capacity,
                               std::vector<FreeBlock>& freeList, std::size_t needed) {
    std::size_t newCapacity = capacity * 2;
    while (newCapacity - capacity < needed) newCapacity *= 2;

    GLuint newBuffer;
    glGenBuffers(1, &newBuffer);
    glBindBuffer(GL_COPY_READ_BUFFER, buffer);
    glBindBuffer(target, newBuffer);
    glBufferData(target, static_cast<GLsizeiptr>(newCapacity), nullptr, GL_DYNAMIC_DRAW);
    glCopyBufferSubData(GL_COPY_READ_BUFFER, target, 0, 0, static_cast<GLsizeiptr>(capacity));

    GLuint oldBuffer = buffer;
    std::size_t oldCapacity = capacity;
    buffer = newBuffer;
    capacity = newCapacity;

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex),
                          reinterpret_cast<void*>(offsetof(TerrainVertex, x)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex),
                          reinterpret_cast<void*>(offsetof(TerrainVertex, tileU)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(TerrainVertex),
                          reinterpret_cast<void*>(offsetof(TerrainVertex, color)));
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, sizeof(TerrainVertex),
                           reinterpret_cast<void*>(offsetof(TerrainVertex, textureIndex)));
    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(4, 1, GL_UNSIGNED_INT, sizeof(TerrainVertex),
                           reinterpret_cast<void*>(offsetof(TerrainVertex, faceId)));
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex),
                          reinterpret_cast<void*>(offsetof(TerrainVertex, flowRotation)));
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex),
                          reinterpret_cast<void*>(offsetof(TerrainVertex, liquidType)));
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex),
                          reinterpret_cast<void*>(offsetof(TerrainVertex, isUnderwater)));
    glEnableVertexAttribArray(8);
    glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex),
                          reinterpret_cast<void*>(offsetof(TerrainVertex, skyLight)));
    glEnableVertexAttribArray(9);
    glVertexAttribPointer(9, 1, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex),
                          reinterpret_cast<void*>(offsetof(TerrainVertex, blockLight)));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBindVertexArray(0);

    glDeleteBuffers(1, &oldBuffer);
    freeBlock(freeList, oldCapacity, newCapacity - oldCapacity);
    return true;
}

BatchedMesh::Allocation BatchedMesh::upload(const std::vector<TerrainVertex>& vertices,
                                            const std::vector<std::uint32_t>& indices) {
    Allocation result;
    if (vertices.empty() || indices.empty()) return result;

    std::size_t vboSize = vertices.size() * sizeof(TerrainVertex);
    std::size_t iboSize = indices.size() * sizeof(std::uint32_t);

    // Align IBO offset to 4 bytes
    std::size_t iboAlign = (4 - (iboSize % 4)) % 4;
    std::size_t alignedIboSize = iboSize + iboAlign;

    std::size_t vboOff = allocateBlock(m_vboFree, vboSize);
    if (vboOff == SIZE_MAX) {
        if (!growBuffer(GL_ARRAY_BUFFER, m_vbo, m_vboCapacity, m_vboFree, vboSize))
            return result;
        vboOff = allocateBlock(m_vboFree, vboSize);
        if (vboOff == SIZE_MAX) return result;
    }

    std::size_t iboOff = allocateBlock(m_eboFree, alignedIboSize);
    if (iboOff == SIZE_MAX) {
        // Rollback VBO allocation
        freeBlock(m_vboFree, vboOff, vboSize);
        if (!growBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo, m_eboCapacity, m_eboFree, alignedIboSize))
            return result;
        iboOff = allocateBlock(m_eboFree, alignedIboSize);
        if (iboOff == SIZE_MAX) {
            freeBlock(m_vboFree, vboOff, vboSize);
            return result;
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vboOff),
                    static_cast<GLsizeiptr>(vboSize), vertices.data());

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(iboOff),
                    static_cast<GLsizeiptr>(iboSize), indices.data());

    result.vboOffset = static_cast<std::uint32_t>(vboOff);
    result.vboSize = static_cast<std::uint32_t>(vboSize);
    result.iboOffset = static_cast<std::uint32_t>(iboOff);
    result.iboSize = static_cast<std::uint32_t>(alignedIboSize);
    result.indexCount = static_cast<std::uint32_t>(indices.size());
    result.valid = true;
    return result;
}

void BatchedMesh::free(const Allocation& alloc) {
    if (!alloc.valid) return;
    freeBlock(m_vboFree, alloc.vboOffset, alloc.vboSize);
    freeBlock(m_eboFree, alloc.iboOffset, alloc.iboSize);
}

void BatchedMesh::remove(std::uint64_t key) {
    auto it = m_entries.find(key);
    if (it != m_entries.end()) {
        free(it->second.alloc);
        m_entries.erase(it);
    }
}

void BatchedMesh::uploadToEntry(std::uint64_t key,
                                const std::vector<TerrainVertex>& vertices,
                                const std::vector<std::uint32_t>& indices) {
    auto it = m_entries.find(key);
    if (it != m_entries.end()) {
        free(it->second.alloc);
    }
    Allocation alloc = upload(vertices, indices);
    m_entries[key] = {alloc};
}

void BatchedMesh::bind() const {
    glBindVertexArray(m_vao);
}

void BatchedMesh::drawIndirect(const void* commands, std::size_t count) const {
    if (count == 0) return;

    std::size_t neededSize = count * sizeof(DrawElementsIndirectCommand);
    if (m_indirectBuffer == 0) {
        m_indirectBufferCapacity = std::max(neededSize * 2, std::size_t(64 * 1024));
        glGenBuffers(1, &m_indirectBuffer);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirectBuffer);
        glBufferData(GL_DRAW_INDIRECT_BUFFER, static_cast<GLsizeiptr>(m_indirectBufferCapacity),
                     nullptr, GL_DYNAMIC_DRAW);
    } else if (neededSize > m_indirectBufferCapacity) {
        m_indirectBufferCapacity = neededSize * 2;
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirectBuffer);
        glBufferData(GL_DRAW_INDIRECT_BUFFER, static_cast<GLsizeiptr>(m_indirectBufferCapacity),
                     nullptr, GL_DYNAMIC_DRAW);
    } else {
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirectBuffer);
    }

    glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, static_cast<GLsizeiptr>(neededSize), commands);

    glBindVertexArray(m_vao);
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
                                static_cast<GLsizei>(count),
                                sizeof(DrawElementsIndirectCommand));
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
}
