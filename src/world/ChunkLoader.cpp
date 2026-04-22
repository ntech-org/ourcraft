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
        m_requestQueue.push({x, z});
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
        std::pair<int, int> coords;
        {
            std::unique_lock<std::mutex> lock(m_requestMutex);
            m_cv.wait(lock, [this] { return !m_requestQueue.empty() || !m_running; });
            if (!m_running) break;
            coords = m_requestQueue.front();
            m_requestQueue.pop();
        }

        auto chunk = std::make_shared<Chunk>(coords.first, coords.second);
        m_generator.generateChunk(*chunk);

        {
            std::lock_guard<std::mutex> lock(m_resultMutex);
            m_resultQueue.push(std::move(chunk));
        }
    }
}
