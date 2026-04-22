#pragma once

#include "world/Chunk.hpp"
#include "world/WorldGenerator.hpp"
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <vector>

class ChunkLoader {
public:
    ChunkLoader(WorldGenerator& generator);
    ~ChunkLoader();

    void requestChunk(int x, int z);
    bool tryPopResult(std::shared_ptr<Chunk>& outChunk);

private:
    void workerLoop();

    WorldGenerator& m_generator;
    std::queue<std::pair<int, int>> m_requestQueue;
    std::queue<std::shared_ptr<Chunk>> m_resultQueue;
    
    std::mutex m_requestMutex;
    std::mutex m_resultMutex;
    std::condition_variable m_cv;
    
    std::vector<std::thread> m_workers;
    std::atomic<bool> m_running;
};
