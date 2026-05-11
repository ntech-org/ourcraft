# OurCraft

A from-scratch C++20 recreation of Minecraft Infdev (2010 era), built with a focus on networking support and high-performance multi-threaded code.

## Features

- **OpenGL 3.3 Core** rendering via SDL3 — shaders, greedy chunk meshing, entity models, sky rendering
- **Client-server architecture** using ENet for UDP-based reliable transport
  - Integrated singleplayer mode (local server on `127.0.0.1:25565`)
  - Dedicated server build (`ourcraft-server`)
  - Custom binary protocol with 21 packet types
- **Infdev-style world generation** — terrain, caves, and features using Perlin/octave noise
- **Persistent storage** via RocksDB for chunks and player data
- **ZSTD compression** for network chunk payloads
- **Entity system** — players, zombies, dropped items with server-authoritative movement
- **Full GUI system** — main menu, inventory, crafting, multiplayer connection, loading, and error screens
- **Multi-threaded design** — dedicated network thread, server tick loop in a background thread, shared mutex for chunk access

## Project Structure

```
ourcraft-cpp/
├── assets/            # Textures, shaders, sounds, fonts
├── include/           # Header files (.hpp), organized by subsystem
├── src/               # Source files (.cpp), mirrors include/ structure
│   ├── main.cpp       # Client entry point
│   ├── server_main.cpp # Dedicated server entry point
│   ├── net/           # Networking (Client, Server, IntegratedServer, packets)
│   ├── renderer/      # OpenGL rendering (shaders, chunk meshing, models)
│   ├── world/         # World simulation (blocks, chunks, generation, storage)
│   ├── entities/      # Entity system (Player, Zombie, Item, Living)
│   ├── gui/           # GUI screens (MainMenu, Inventory, Crafting, etc.)
│   ├── physics/       # AABB collision detection
│   ├── inventory/     # Crafting manager
│   ├── items/         # Item registry
│   └── util/          # Utilities (Timer, Compression, ThreadSafeQueue)
├── world/             # Saved world data (RocksDB)
├── xmake.lua          # Build configuration
└── options.txt        # Game settings (sensitivity, FOV, render distance, etc.)
```

## Dependencies

| Library     | Purpose                                         |
|-------------|-------------------------------------------------|
| libsdl3     | Window creation, input, events, OpenGL context   |
| glad        | OpenGL 3.3 function loading                     |
| glm         | Math library (vectors, matrices)                |
| enet        | UDP networking (reliable & unreliable)          |
| rocksdb     | Persistent chunk and player data storage        |
| zstd        | ZSTD compression for network payloads           |
| stb         | Image loading (header-only)                     |
| freetype    | TrueType/OTF font rendering                     |

## Building

This project uses [XMake](https://xmake.io/) as its build system.

### Client

```bash
xmake build ourcraft
```

### Dedicated Server

```bash
xmake build ourcraft-server
```

Both targets require C++20. Release builds enable LTO and `fastest` optimization. System links on Linux: `pthread`, `dl`, `m`.

## Running

### Singleplayer

Build and run the `ourcraft` target. The game automatically starts an integrated server on `127.0.0.1:25565`.

### Multiplayer

1. Start a dedicated server: `./ourcraft-server`
2. Launch the client and use the Multiplayer screen to connect to the server address.

The server listens on port **25565** by default.

## Architecture

### Networking

- **Client** — manages an ENet host with a dedicated network thread for sending and polling events. Thread-safe queues bridge the network thread and the main game thread.
- **Server** — ENet-based server host handling up to 32 clients with connect, receive, and disconnect event processing.
- **IntegratedServer** — full server loop running at 20 TPS, handling player login, chunk loading/unloading, block interactions, entity updates, mob spawning, and periodic world saves (every 5 minutes).
- **NetworkHandler** — client-side bridge that processes 17+ packet types and manages the connection between the local integrated server and remote server.

### Threading Model

- **Main thread** — game loop (20 TPS tick + render with VSync)
- **Network thread** (Client) — polls ENet events and sends outgoing packets
- **Server thread** (IntegratedServer) — runs the server tick loop
- **Shared mutex** — protects chunk lookup in the World for concurrent read/write access

### World Storage

Chunks are stored in RocksDB with ZSTD compression. The save format is compatible with early Minecraft world layouts, using region-based chunk keys. Player data (position, rotation, health, inventory) is persisted alongside chunk data.
