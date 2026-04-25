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

enum class ChunkTaskType {
    Generate,
    Decorate,
    Lighting
};

struct ChunkTask {
    ChunkTaskType type;
    int x, z;
    std::shared_ptr<Chunk> chunk;
    std::shared_ptr<Chunk> chunkE;
    std::shared_ptr<Chunk> chunkS;
    std::shared_ptr<Chunk> chunkSE;
};

class ChunkLoader {
public:
    ChunkLoader(WorldGenerator& generator, class World* world = nullptr);
    ~ChunkLoader();

    void requestChunk(int x, int z);
    void requestDecoration(std::shared_ptr<Chunk> chunk, 
                           std::shared_ptr<Chunk> chunkE,
                           std::shared_ptr<Chunk> chunkS,
                           std::shared_ptr<Chunk> chunkSE);
    void requestLighting(std::shared_ptr<Chunk> chunk);
    
    bool tryPopResult(std::shared_ptr<Chunk>& outChunk);

private:
    void workerLoop();

    WorldGenerator& m_generator;
    class World* m_world;
    std::queue<ChunkTask> m_requestQueue;
    std::queue<std::shared_ptr<Chunk>> m_resultQueue;
    
    std::mutex m_requestMutex;
    std::mutex m_resultMutex;
    std::condition_variable m_cv;
    
    std::vector<std::thread> m_workers;
    std::atomic<bool> m_running;
};
