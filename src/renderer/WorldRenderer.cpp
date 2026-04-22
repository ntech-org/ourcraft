#include "renderer/WorldRenderer.hpp"
#include "renderer/ChunkMesher.hpp"
#include "renderer/Shader.hpp"
#include "world/Chunk.hpp"
#include "world/World.hpp"
#include <chrono>

WorldRenderer::WorldRenderer(World& world) : m_world(world) {
    rebuildSectionList();
}

void WorldRenderer::rebuildSectionList() {
    for (const auto& chunkPtr : m_world.getChunks()) {
        Chunk* chunk = chunkPtr.get();
        for (int sectionIndex = 0; sectionIndex < Chunk::SECTION_COUNT; ++sectionIndex) {
            std::uint64_t key = sectionKey(chunk->getX(), chunk->getZ(), sectionIndex);
            if (m_sections.find(key) == m_sections.end()) {
                SectionRenderEntry entry;
                entry.chunk = chunk;
                entry.sectionIndex = sectionIndex;
                const float baseX = static_cast<float>(chunk->getX() * Chunk::WIDTH);
                const float baseY = static_cast<float>(Chunk::getSectionMinY(sectionIndex));
                const float baseZ = static_cast<float>(chunk->getZ() * Chunk::DEPTH);
                entry.bounds.min = {baseX, baseY, baseZ};
                entry.bounds.max = {
                    baseX + static_cast<float>(Chunk::WIDTH),
                    baseY + static_cast<float>(Chunk::SECTION_HEIGHT),
                    baseZ + static_cast<float>(Chunk::DEPTH)
                };
                m_sections[key] = std::move(entry);
            }
        }
    }
    m_stats.sectionCount = m_sections.size();
}

void WorldRenderer::updateDirtyMeshes() {
    using clock = std::chrono::steady_clock;

    m_stats.sectionCount = m_sections.size();
    m_stats.meshBuilds = 0;
    m_stats.meshBuildMs = 0.0;

    int buildsThisFrame = 0;
    const int maxBuildsPerFrame = 4; // limit to 4 mesh builds per frame to avoid lag spikes

    for (auto& [key, entry] : m_sections) {
        if (!entry.chunk->isSectionDirty(entry.sectionIndex)) {
            continue;
        }

        const auto buildStart = clock::now();
        const ChunkMeshData meshData = ChunkMesher::buildSectionMesh(m_world, *entry.chunk, entry.sectionIndex);
        const auto buildEnd = clock::now();

        entry.bounds = meshData.bounds;
        if (meshData.empty()) {
            entry.mesh.clear();
        } else {
            entry.mesh.upload(meshData);
        }

        entry.uploadedVersion = entry.chunk->getSectionVersion(entry.sectionIndex);
        entry.chunk->clearSectionDirty(entry.sectionIndex);

        ++m_stats.meshBuilds;
        m_stats.meshBuildMs += std::chrono::duration<double, std::milli>(buildEnd - buildStart).count();
        
        if (++buildsThisFrame >= maxBuildsPerFrame) {
            break;
        }
    }
}

void WorldRenderer::render(const Frustum& frustum, Shader& shader) {
    m_stats.visibleSections = 0;
    m_stats.drawCalls = 0;
    m_stats.triangles = 0;

    shader.use();

    for (const auto& [key, entry] : m_sections) {
        if (!entry.mesh.hasGeometry()) {
            continue;
        }

        if (!frustum.intersects(entry.bounds)) {
            continue;
        }

        ++m_stats.visibleSections;
        ++m_stats.drawCalls;
        m_stats.triangles += entry.mesh.getTriangleCount();
        entry.mesh.draw();
    }
}

std::uint64_t WorldRenderer::sectionKey(int cx, int cz, int sectionIndex) {
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(cx)) << 40) |
           (static_cast<std::uint64_t>(static_cast<std::uint32_t>(cz)) << 8) |
           (static_cast<std::uint64_t>(sectionIndex) & 0xFFu);
}
