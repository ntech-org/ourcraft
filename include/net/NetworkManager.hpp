#pragma once

#include <enet/enet.h>
#include <stdexcept>

class NetworkManager {
public:
    static void init() {
        if (enet_initialize() != 0) {
            throw std::runtime_error("An error occurred while initializing ENet.");
        }
    }

    static void deinit() {
        enet_deinitialize();
    }
};
