#include "net/IntegratedServer.hpp"
#include "net/NetworkManager.hpp"
#include "world/Block.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>

IntegratedServer* g_server = nullptr;

void signalHandler(int signum) {
    std::cout << "\n[Server] Interrupt signal (" << signum << ") received. Shutting down..." << std::endl;
    if (g_server) {
        g_server->stop();
    }
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "OurCraft Dedicated Server starting..." << std::endl;

    try {
        NetworkManager::init();
        Block::init();
        
        auto server = std::make_unique<IntegratedServer>();
        g_server = server.get();
        
        // Use dedicated mode to run IntegratedServer logic in the main thread
        server->setDedicated(true);
        
        std::cout << "[Server] Server is now running. Press Ctrl+C to stop." << std::endl;
        
        server->start();

    } catch (const std::exception& e) {
        std::cerr << "[Server] Fatal error: " << e.what() << std::endl;
        return 1;
    }

    NetworkManager::deinit();
    std::cout << "[Server] Server stopped." << std::endl;

    return 0;
}
