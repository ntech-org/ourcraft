#include "world/ChunkLoader.hpp"
#include "world/World.hpp"
#include "util/Profiler.hpp"

ChunkLoader::ChunkLoader(WorldGenerator& generator, World* world, SaveHandler* saveHandler) 
    : m_generator(generator), m_world(world), m_saveHandler(saveHandler), m_running(true) 
{
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 8; // Better fallback

    for (unsigned int i = 0; i < numThreads; ++i) {
        m_workers.emplace_back(&ChunkLoader::workerLoop, this);
    }
}

ChunkLoader::~ChunkLoader() {
    m_running = false;
    m_cv.notify_all();
    for (auto& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ChunkLoader::requestChunk(int x, int z) {
    {
        std::lock_guard<std::mutex> lock(m_requestMutex);
        m_requestQueue.push({ChunkTaskType::Generate, x, z, nullptr, nullptr, nullptr, nullptr});
    }
    m_cv.notify_one();
}

void ChunkLoader::requestDecoration(std::shared_ptr<Chunk> chunk, 
                                   std::shared_ptr<Chunk> chunkE,
                                   std::shared_ptr<Chunk> chunkS,
                                   std::shared_ptr<Chunk> chunkSE) 
{
    {
        std::lock_guard<std::mutex> lock(m_requestMutex);
        m_requestQueue.push({ChunkTaskType::Decorate, chunk->getX(), chunk->getZ(), chunk, chunkE, chunkS, chunkSE});
    }
    m_cv.notify_one();
}

void ChunkLoader::requestLighting(std::shared_ptr<Chunk> chunk) {
    {
        std::lock_guard<std::mutex> lock(m_requestMutex);
        m_requestQueue.push({ChunkTaskType::Lighting, chunk->getX(), chunk->getZ(), chunk, nullptr, nullptr, nullptr});
    }
    m_cv.notify_one();
}

void ChunkLoader::requestSave(std::shared_ptr<Chunk> chunk) {
    {
        std::lock_guard<std::mutex> lock(m_requestMutex);
        m_requestQueue.push({ChunkTaskType::Save, chunk->getX(), chunk->getZ(), chunk, nullptr, nullptr, nullptr});
    }
    m_cv.notify_one();
}

bool ChunkLoader::tryPopResult(std::shared_ptr<Chunk>& outChunk) {
    std::lock_guard<std::mutex> lock(m_resultMutex);
    if (m_resultQueue.empty()) return false;
    outChunk = std::move(m_resultQueue.front());
    m_resultQueue.pop();
    return true;
}

void ChunkLoader::workerLoop() {
    OC_THREAD_NAME("ChunkLoader");
    while (true) {
        ChunkTask task;
        {
            std::unique_lock<std::mutex> lock(m_requestMutex);
            m_cv.wait(lock, [this] { return !m_requestQueue.empty() || !m_running; });
            if (!m_running && m_requestQueue.empty()) break;
            task = std::move(m_requestQueue.top());
            m_requestQueue.pop();
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
        } else if (task.type == ChunkTaskType::Decorate) {
            OC_ZONE_SCOPED_N("ChunkDecorate");
            task.chunk->setState(ChunkState::Decorating);
            m_generator.decorateChunk(*task.chunk, task.chunkE.get(), task.chunkS.get(), task.chunkSE.get());
            task.chunk->setState(ChunkState::Decorated);

            std::lock_guard<std::mutex> lock(m_resultMutex);
            m_resultQueue.push(std::move(task.chunk));
        } else if (task.type == ChunkTaskType::Lighting) {
            OC_ZONE_SCOPED_N("ChunkLighting");
            // Lighting can happen after Generated or after Decorated
            ChunkState oldState = task.chunk->getState();
            task.chunk->setState(ChunkState::Lighting);
            if (m_world) {
                m_world->calculateInitialSkylight(*task.chunk);
            }
            // If we were Decorated or already in FinalLighting, we are now Complete.
            // Otherwise we are Lighted (waiting for decoration).
            task.chunk->setState((oldState == ChunkState::Decorated || oldState == ChunkState::LightingFinal) ? ChunkState::Complete : ChunkState::Lighted);

            std::lock_guard<std::mutex> lock(m_resultMutex);
            m_resultQueue.push(std::move(task.chunk));
        } else if (task.type == ChunkTaskType::Save) {
            OC_ZONE_SCOPED_N("ChunkSave");
            m_saveHandler->saveChunk(*task.chunk);
        }
    }
}
