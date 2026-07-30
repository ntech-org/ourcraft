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

#include "world/storage/SaveHandler.hpp"

enum class ChunkTaskType {
    Generate,
    Decorate,
    Lighting,
    Save
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
    ChunkLoader(WorldGenerator& generator, class World* world = nullptr, SaveHandler* saveHandler = nullptr);
    ~ChunkLoader();

    void requestChunk(int x, int z);
    void requestDecoration(std::shared_ptr<Chunk> chunk, 
                           std::shared_ptr<Chunk> chunkE,
                           std::shared_ptr<Chunk> chunkS,
                           std::shared_ptr<Chunk> chunkSE);
    void requestLighting(std::shared_ptr<Chunk> chunk);
    void requestSave(std::shared_ptr<Chunk> chunk);
    void stopWorldAccess() { m_world = nullptr; }
    
    bool tryPopResult(std::shared_ptr<Chunk>& outChunk);
    bool hasPendingWork() const { return m_pendingWork.load(std::memory_order_acquire) > 0; }

    SaveHandler* getSaveHandler() { return m_saveHandler; }

private:
    void workerLoop();
    void decorationLoop();
    void lightingLoop();

    WorldGenerator& m_generator;
    class World* m_world;
    SaveHandler* m_saveHandler;
    std::queue<ChunkTask> m_generateQueue;
    std::queue<ChunkTask> m_decorationQueue;
    std::queue<ChunkTask> m_lightingQueue;
    std::queue<ChunkTask> m_saveQueue;
    std::queue<std::shared_ptr<Chunk>> m_resultQueue;
    
    std::mutex m_requestMutex;
    std::mutex m_resultMutex;
    std::condition_variable m_cv;
    std::condition_variable m_decorationCv;
    std::condition_variable m_lightingCv;
    
    std::vector<std::thread> m_workers;
    std::atomic<bool> m_running;
    std::atomic<std::size_t> m_pendingWork {0};
};
