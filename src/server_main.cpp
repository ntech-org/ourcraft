#include "net/IntegratedServer.hpp"
#include "net/CommandHandler.hpp"
#include "net/NetworkManager.hpp"
#include "world/Block.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <string>

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
        
        server->setDedicated(true);
        server->start();
        
        std::cout << "[Server] Server is now running. Type 'help' for commands. Press Ctrl+C to stop." << std::endl;

        std::string line;
        while (server->isRunning() && std::getline(std::cin, line)) {
            if (line.empty()) continue;
            if (line == "stop") {
                server->stop();
                break;
            }

            CommandHandler::CommandContext ctx;
            ctx.server = server.get();
            ctx.senderName = "CONSOLE";

            std::string result = server->getCommandHandler().execute(line, ctx, true);
            if (!result.empty()) {
                std::cout << result << std::endl;
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "[Server] Fatal error: " << e.what() << std::endl;
        return 1;
    }

    NetworkManager::deinit();
    std::cout << "[Server] Server stopped." << std::endl;

    return 0;
}
