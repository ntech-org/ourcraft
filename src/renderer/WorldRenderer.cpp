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
    m_sections.clear();
    m_sections.reserve(m_world.getChunks().size() * Chunk::SECTION_COUNT);

    for (const auto& chunkPtr : m_world.getChunks()) {
        Chunk* chunk = chunkPtr.get();
        for (int sectionIndex = 0; sectionIndex < Chunk::SECTION_COUNT; ++sectionIndex) {
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
            m_sections.push_back(std::move(entry));
        }
    }

    m_stats.sectionCount = m_sections.size();
}

void WorldRenderer::updateDirtyMeshes() {
    using clock = std::chrono::steady_clock;

    m_stats.sectionCount = m_sections.size();
    m_stats.meshBuilds = 0;
    m_stats.meshBuildMs = 0.0;

    for (SectionRenderEntry& entry : m_sections) {
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
    }
}

void WorldRenderer::render(const Frustum& frustum, Shader& shader) {
    m_stats.visibleSections = 0;
    m_stats.drawCalls = 0;
    m_stats.triangles = 0;

    shader.use();

    for (const SectionRenderEntry& entry : m_sections) {
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
