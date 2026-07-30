#pragma once

#include "renderer/ChunkMesh.hpp"
#include <glad/glad.h>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <unordered_map>

class BatchedMesh {
public:
    struct DrawElementsIndirectCommand {
        std::uint32_t count;
        std::uint32_t instanceCount;
        std::uint32_t firstIndex;
        std::int32_t  baseVertex;
        std::uint32_t baseInstance;
    };

    struct Allocation {
        std::uint32_t vboOffset = 0;
        std::uint32_t vboSize = 0;
        std::uint32_t iboOffset = 0;
        std::uint32_t iboSize = 0;
        std::uint32_t indexCount = 0;
        bool valid = false;
    };

    BatchedMesh() = default;
    ~BatchedMesh();

    BatchedMesh(const BatchedMesh&) = delete;
    BatchedMesh& operator=(const BatchedMesh&) = delete;

    bool init(std::size_t vboCapacity = 256 * 1024 * 1024,
              std::size_t iboCapacity = 64 * 1024 * 1024);

    Allocation upload(const std::vector<TerrainVertex>& vertices,
                      const std::vector<std::uint32_t>& indices);
    void free(const Allocation& alloc);
    void remove(std::uint64_t key);
    void uploadToEntry(std::uint64_t key,
                       const std::vector<TerrainVertex>& vertices,
                       const std::vector<std::uint32_t>& indices);

    // One hard fence wait for a batch of upload/free (Wayland-safe).
    // Without a batch, each upload/free still waits individually.
    void beginMutationBatch();
    void endMutationBatch();

    void bind() const;
    void drawIndirect(const void* commands, std::size_t count);

    // Wait until GPU has finished reading VBO/IBO from the last draw.
    // Must be called before free/upload/grow that reuses buffer ranges.
    void waitGpuIdle();

    bool isInitialized() const { return m_vao != 0; }
    std::size_t getVBOUsage() const { return m_vboCapacity - totalFreeBytes(m_vboFree); }
    std::size_t getIBOUsage() const { return m_eboCapacity - totalFreeBytes(m_eboFree); }

private:
    void insertDrawFence();
    struct FreeBlock {
        std::size_t offset;
        std::size_t size;
    };

    static std::size_t totalFreeBytes(const std::vector<FreeBlock>& freeList);

    std::size_t allocateBlock(std::vector<FreeBlock>& freeList, std::size_t size);
    void freeBlock(std::vector<FreeBlock>& freeList, std::size_t offset, std::size_t size);
    bool growBuffer(GLenum target, GLuint& buffer, std::size_t& capacity,
                    std::vector<FreeBlock>& freeList, std::size_t needed);

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;
    mutable GLuint m_indirectBuffer = 0;
    mutable std::size_t m_indirectBufferCapacity = 0;
    GLsync m_drawFence = nullptr;
    bool m_mutationBatchActive = false;
    std::size_t m_vboCapacity = 0;
    std::size_t m_eboCapacity = 0;
    std::vector<FreeBlock> m_vboFree;
    std::vector<FreeBlock> m_eboFree;

    struct EntryAlloc {
        Allocation alloc;
    };
    std::unordered_map<std::uint64_t, EntryAlloc> m_entries;
};
