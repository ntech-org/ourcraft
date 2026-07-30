#include "world/ChunkLoader.hpp"
#include "world/World.hpp"
#include "util/Profiler.hpp"
#include <algorithm>

ChunkLoader::ChunkLoader(WorldGenerator& generator, World* world, SaveHandler* saveHandler) 
    : m_generator(generator), m_world(world), m_saveHandler(saveHandler), m_running(true) 
{
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;
    numThreads = std::min(numThreads > 4 ? numThreads - 4 : 1u, 4u);

    for (unsigned int i = 0; i < numThreads; ++i) {
        m_workers.emplace_back(&ChunkLoader::workerLoop, this);
    }
    const unsigned int decorationThreads = std::thread::hardware_concurrency() >= 8 ? 2u : 1u;
    for (unsigned int i = 0; i < decorationThreads; ++i) {
        m_workers.emplace_back(&ChunkLoader::decorationLoop, this);
    }
    m_workers.emplace_back(&ChunkLoader::lightingLoop, this);
}

ChunkLoader::~ChunkLoader() {
    m_running = false;
    m_cv.notify_all();
    m_decorationCv.notify_all();
    m_lightingCv.notify_all();
    for (auto& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ChunkLoader::requestChunk(int x, int z) {
    m_pendingWork.fetch_add(1, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lock(m_requestMutex);
        m_generateQueue.push({ChunkTaskType::Generate, x, z, nullptr, nullptr, nullptr, nullptr});
    }
    m_cv.notify_one();
}

void ChunkLoader::requestDecoration(std::shared_ptr<Chunk> chunk, 
                                   std::shared_ptr<Chunk> chunkE,
                                   std::shared_ptr<Chunk> chunkS,
                                   std::shared_ptr<Chunk> chunkSE) 
{
    m_pendingWork.fetch_add(1, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lock(m_requestMutex);
        m_decorationQueue.push({ChunkTaskType::Decorate, chunk->getX(), chunk->getZ(), chunk, chunkE, chunkS, chunkSE});
    }
    m_decorationCv.notify_one();
}

void ChunkLoader::requestLighting(std::shared_ptr<Chunk> chunk) {
    m_pendingWork.fetch_add(1, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lock(m_requestMutex);
        m_lightingQueue.push({ChunkTaskType::Lighting, chunk->getX(), chunk->getZ(), chunk, nullptr, nullptr, nullptr});
    }
    m_lightingCv.notify_one();
}

void ChunkLoader::requestSave(std::shared_ptr<Chunk> chunk) {
    m_pendingWork.fetch_add(1, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lock(m_requestMutex);
        m_saveQueue.push({ChunkTaskType::Save, chunk->getX(), chunk->getZ(), chunk, nullptr, nullptr, nullptr});
    }
    m_cv.notify_one();
}

bool ChunkLoader::tryPopResult(std::shared_ptr<Chunk>& outChunk) {
    std::lock_guard<std::mutex> lock(m_resultMutex);
    if (m_resultQueue.empty()) return false;
    outChunk = std::move(m_resultQueue.front());
    m_resultQueue.pop();
    m_pendingWork.fetch_sub(1, std::memory_order_release);
    return true;
}

void ChunkLoader::workerLoop() {
    OC_THREAD_NAME("ChunkLoader");
    while (true) {
        ChunkTask task;
        {
            std::unique_lock<std::mutex> lock(m_requestMutex);
            m_cv.wait(lock, [this] { return !m_generateQueue.empty() || !m_saveQueue.empty() || !m_running; });
            if (!m_running && m_generateQueue.empty() && m_saveQueue.empty()) break;
            if (!m_generateQueue.empty()) {
                task = std::move(m_generateQueue.front());
                m_generateQueue.pop();
            } else {
                task = std::move(m_saveQueue.front());
                m_saveQueue.pop();
            }
        }

        if (task.type == ChunkTaskType::Generate) {
            OC_ZONE_SCOPED_N("ChunkGenerate");
            auto chunk = std::make_shared<Chunk>(task.x, task.z);

            bool loaded = false;
            {
                OC_ZONE_SCOPED_N("ChunkLoadDisk");
                loaded = m_saveHandler->loadChunk(*chunk);
            }
            if (!loaded) {
                chunk->setState(ChunkState::Generating);
                {
                    OC_ZONE_SCOPED_N("WorldGen");
                    m_generator.generateChunk(*chunk);
                }
                chunk->setState(ChunkState::Generated);
            }

            std::lock_guard<std::mutex> lock(m_resultMutex);
            m_resultQueue.push(std::move(chunk));
        } else {
            OC_ZONE_SCOPED_N("ChunkSave");
            m_saveHandler->saveChunk(*task.chunk);
            m_pendingWork.fetch_sub(1, std::memory_order_release);
        }
    }
}

void ChunkLoader::decorationLoop() {
    OC_THREAD_NAME("ChunkDecoration");
    while (true) {
        ChunkTask task;
        {
            std::unique_lock<std::mutex> lock(m_requestMutex);
            m_decorationCv.wait(lock, [this] { return !m_decorationQueue.empty() || !m_running; });
            if (!m_running && m_decorationQueue.empty()) break;
            task = std::move(m_decorationQueue.front());
            m_decorationQueue.pop();
        }

        {
            OC_ZONE_SCOPED_N("ChunkDecorate");
            std::array<Chunk*, 4> chunks {
                task.chunk.get(), task.chunkE.get(), task.chunkS.get(), task.chunkSE.get()
            };
            std::sort(chunks.begin(), chunks.end(), [](const Chunk* a, const Chunk* b) {
                if (!a || !b) return a < b;
                return a->getX() != b->getX() ? a->getX() < b->getX() : a->getZ() < b->getZ();
            });
            std::vector<std::unique_lock<std::mutex>> blockLocks;
            blockLocks.reserve(chunks.size());
            Chunk* previous = nullptr;
            for (Chunk* chunk : chunks) {
                if (chunk && chunk != previous) blockLocks.emplace_back(chunk->getBlockMutex());
                previous = chunk;
            }
            task.chunk->setState(ChunkState::Decorating);
            m_generator.decorateChunk(*task.chunk, task.chunkE.get(), task.chunkS.get(), task.chunkSE.get());
            task.chunk->setState(ChunkState::Decorated);

            std::lock_guard<std::mutex> lock(m_resultMutex);
            m_resultQueue.push(std::move(task.chunk));
        }
    }
}

void ChunkLoader::lightingLoop() {
    OC_THREAD_NAME("ChunkLighting");
    while (true) {
        ChunkTask task;
        {
            std::unique_lock<std::mutex> lock(m_requestMutex);
            m_lightingCv.wait(lock, [this] { return !m_lightingQueue.empty() || !m_running; });
            if (!m_running && m_lightingQueue.empty()) break;
            task = std::move(m_lightingQueue.front());
            m_lightingQueue.pop();
        }

        {
            OC_ZONE_SCOPED_N("ChunkLighting");
            // Keep final-lighting chunks at their monotonic state while the
            // worker runs so they remain valid dependencies for other chunks.
            ChunkState oldState = task.chunk->getState();
            if (oldState < ChunkState::Decorated) {
                task.chunk->setState(ChunkState::Lighting);
            }
            if (m_world) {
                m_world->calculateInitialSkylight(*task.chunk);
            }
            // Final lighting must converge with neighboring final-light passes
            // before this chunk can be published to clients.
            task.chunk->setState((oldState == ChunkState::Decorated || oldState == ChunkState::LightingFinal)
                ? ChunkState::LightingReady : ChunkState::Lighted);

            std::lock_guard<std::mutex> lock(m_resultMutex);
            m_resultQueue.push(std::move(task.chunk));
        }
    }
}
