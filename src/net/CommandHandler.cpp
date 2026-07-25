#include "net/CommandHandler.hpp"
#include "net/IntegratedServer.hpp"
#include "net/Packets.hpp"
#include "net/RegistrationManager.hpp"
#include "entities/EntityPlayer.hpp"
#include "world/World.hpp"
#include <sstream>
#include <algorithm>
#include <iostream>
#include <cstdlib>

CommandHandler::CommandHandler() {
    registerCommand("gamemode", [this](CommandContext& ctx) { return handleGamemode(ctx); }, true);
    registerCommand("gm", [this](CommandContext& ctx) { return handleGamemode(ctx); }, true);
    registerCommand("op", [this](CommandContext& ctx) { return handleOp(ctx); }, true);
    registerCommand("deop", [this](CommandContext& ctx) { return handleDeop(ctx); }, true);
    registerCommand("say", [this](CommandContext& ctx) { return handleSay(ctx); }, false);
    registerCommand("tp", [this](CommandContext& ctx) { return handleTp(ctx); }, true);
    registerCommand("time", [this](CommandContext& ctx) { return handleTime(ctx); }, true);
    registerCommand("help", [this](CommandContext& ctx) { return handleHelp(ctx); }, false);
    registerCommand("register", [this](CommandContext& ctx) { return handleRegister(ctx); }, false);
    registerCommand("login", [this](CommandContext& ctx) { return handleLogin(ctx); }, false);
}

void CommandHandler::registerCommand(const std::string& name, CommandFunc func, bool requiresOp) {
    m_commands.push_back({name, {std::move(func), requiresOp}});
}

bool CommandHandler::isCommand(const std::string& input) const {
    return !input.empty() && input[0] == '/';
}

std::string CommandHandler::execute(const std::string& input, CommandContext& ctx, bool senderIsOp) {
    if (input.empty() || input[0] != '/') return "";

    std::string trimmed = input.substr(1);
    std::istringstream iss(trimmed);
    std::string cmdName;
    iss >> cmdName;

    std::transform(cmdName.begin(), cmdName.end(), cmdName.begin(), ::tolower);

    ctx.args.clear();
    std::string arg;
    while (iss >> arg) {
        ctx.args.push_back(arg);
    }

    for (auto& [name, registered] : m_commands) {
        if (name == cmdName) {
            if (registered.requiresOp && !senderIsOp) {
                return "You don't have permission to use /" + cmdName;
            }
            return registered.func(ctx);
        }
    }

    return "Unknown command: /" + cmdName + ". Type /help for a list of commands.";
}

std::vector<std::string> CommandHandler::getCompletions(const std::string& prefix) const {
    std::vector<std::string> results;
    std::string lower = prefix;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    for (auto& [name, _] : m_commands) {
        if (name.size() >= lower.size() && name.compare(0, lower.size(), lower) == 0) {
            results.push_back("/" + name);
        }
    }
    return results;
}

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

std::string CommandHandler::handleGamemode(CommandContext& ctx) {
    if (ctx.args.empty()) return "Usage: /gamemode <survival|creative> [player]";

    std::string modeStr = toLower(ctx.args[0]);
    uint8_t mode;
    if (modeStr == "survival" || modeStr == "s" || modeStr == "0") {
        mode = 0;
    } else if (modeStr == "creative" || modeStr == "c" || modeStr == "1") {
        mode = 1;
    } else {
        return "Invalid gamemode: " + ctx.args[0] + ". Use survival or creative.";
    }

    std::string targetName = ctx.args.size() > 1 ? ctx.args[1] : ctx.senderName;

    ENetPeer* targetPeer = nullptr;
    for (auto& [peer, session] : ctx.server->getPlayers()) {
        if (session.username == targetName) {
            targetPeer = peer;
            break;
        }
    }

    if (!targetPeer) return "Player not found: " + targetName;

    auto& session = ctx.server->getPlayers()[targetPeer];
    session.gameMode = (mode == 0) ? GameMode::SURVIVAL : GameMode::CREATIVE;

    for (auto& entity : ctx.server->getWorld()->getEntities()) {
        if (entity->entityID == session.entityID) {
            if (auto* player = dynamic_cast<EntityPlayer*>(entity.get())) {
                player->gameMode = session.gameMode;
            }
            break;
        }
    }

    PacketGameModeChange pkt;
    pkt.gameMode = mode;
    ctx.server->getServer()->sendPacket(targetPeer, pkt, true);

    std::string modeName = (mode == 0) ? "Survival" : "Creative";
    if (targetName == ctx.senderName) {
        return "Set own gamemode to " + modeName;
    }
    return "Set " + targetName + "'s gamemode to " + modeName;
}

std::string CommandHandler::handleOp(CommandContext& ctx) {
    if (ctx.args.empty()) return "Usage: /op <player>";
    ctx.server->getPermissions().addOp(ctx.args[0]);
    return "Opped " + ctx.args[0];
}

std::string CommandHandler::handleDeop(CommandContext& ctx) {
    if (ctx.args.empty()) return "Usage: /deop <player>";
    ctx.server->getPermissions().removeOp(ctx.args[0]);
    return "De-opped " + ctx.args[0];
}

std::string CommandHandler::handleSay(CommandContext& ctx) {
    return "";
}

std::string CommandHandler::handleTp(CommandContext& ctx) {
    if (ctx.args.size() < 3) return "Usage: /tp <x> <y> <z>";

    double x, y, z;
    try {
        x = std::stod(ctx.args[0]);
        y = std::stod(ctx.args[1]);
        z = std::stod(ctx.args[2]);
    } catch (...) {
        return "Invalid coordinates";
    }

    for (auto& [peer, session] : ctx.server->getPlayers()) {
        if (session.username == ctx.senderName) {
            for (auto& entity : ctx.server->getWorld()->getEntities()) {
                if (entity->entityID == session.entityID) {
                    entity->setPosition(x, y, z);
                    PacketPlayerPosLook pkt;
                    pkt.x = x; pkt.y = y; pkt.z = z;
                    pkt.yaw = entity->rotationYaw;
                    pkt.pitch = entity->rotationPitch;
                    pkt.onGround = false;
                    ctx.server->getServer()->sendPacket(peer, pkt, true);
                    return "Teleported to " + ctx.args[0] + " " + ctx.args[1] + " " + ctx.args[2];
                }
            }
        }
    }
    return "Failed to teleport";
}

std::string CommandHandler::handleTime(CommandContext& ctx) {
    if (ctx.args.size() < 2) return "Usage: /time <set|add> <value>";

    std::string action = toLower(ctx.args[0]);
    long long value;
    try {
        value = std::stoll(ctx.args[1]);
    } catch (...) {
        return "Invalid time value";
    }

    World* world = ctx.server->getWorld();
    if (action == "set") {
        world->setWorldTime((double)value);
        return "Set time to " + std::to_string(value);
    } else if (action == "add") {
        world->setWorldTime(world->getWorldTime() + (double)value);
        return "Added " + std::to_string(value) + " to time";
    }
    return "Unknown action: " + action + ". Use set or add.";
}

std::string CommandHandler::handleHelp(CommandContext& ctx) {
    std::string result = "--- Commands ---";
    for (auto& [name, registered] : m_commands) {
        result += "\n/" + name;
    }
    return result;
}

std::string CommandHandler::handleRegister(CommandContext& ctx) {
    if (!ctx.server) return "Cannot register in this context";
    auto& regMgr = ctx.server->getRegistrationManager();
    if (regMgr.isRegistered(ctx.senderName)) {
        return "You are already registered!";
    }
    std::string key = regMgr.registerUser(ctx.senderName);
    return "Registered! Your key is: " + key + " (Save this!)";
}

std::string CommandHandler::handleLogin(CommandContext& ctx) {
    if (!ctx.server) return "Cannot login in this context";
    if (ctx.args.empty()) return "Usage: /login <key>";
    auto& regMgr = ctx.server->getRegistrationManager();
    if (!regMgr.isRegistered(ctx.senderName)) {
        return "You are not registered! Use /register first.";
    }
    if (regMgr.verifyKey(ctx.senderName, ctx.args[0])) {
        return "Login successful!";
    }
    return "Invalid key!";
}
