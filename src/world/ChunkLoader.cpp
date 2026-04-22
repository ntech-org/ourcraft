#include "world/ChunkLoader.hpp"

ChunkLoader::ChunkLoader(WorldGenerator& generator) : m_generator(generator), m_running(true) {
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4; // Fallback

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

bool ChunkLoader::tryPopResult(std::shared_ptr<Chunk>& outChunk) {
    std::lock_guard<std::mutex> lock(m_resultMutex);
    if (m_resultQueue.empty()) return false;
    outChunk = std::move(m_resultQueue.front());
    m_resultQueue.pop();
    return true;
}

void ChunkLoader::workerLoop() {
    while (m_running) {
        ChunkTask task;
        {
            std::unique_lock<std::mutex> lock(m_requestMutex);
            m_cv.wait(lock, [this] { return !m_requestQueue.empty() || !m_running; });
            if (!m_running) break;
            task = std::move(m_requestQueue.front());
            m_requestQueue.pop();
        }

        if (task.type == ChunkTaskType::Generate) {
            auto chunk = std::make_shared<Chunk>(task.x, task.z);
            chunk->setState(ChunkState::Generating);
            m_generator.generateChunk(*chunk);
            chunk->setState(ChunkState::Generated);

            std::lock_guard<std::mutex> lock(m_resultMutex);
            m_resultQueue.push(std::move(chunk));
        } else if (task.type == ChunkTaskType::Decorate) {
            task.chunk->setState(ChunkState::Decorating);
            m_generator.decorateChunk(*task.chunk, task.chunkE.get(), task.chunkS.get(), task.chunkSE.get());
            task.chunk->setState(ChunkState::Decorated);

            std::lock_guard<std::mutex> lock(m_resultMutex);
            m_resultQueue.push(std::move(task.chunk));
        }
    }
}
