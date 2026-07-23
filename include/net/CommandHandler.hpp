#pragma once

#include <string>
#include <functional>
#include <vector>

struct _ENetPeer;
typedef struct _ENetPeer ENetPeer;

class IntegratedServer;

class CommandHandler {
public:
    struct CommandContext {
        IntegratedServer* server = nullptr;
        ENetPeer* senderPeer = nullptr;
        std::string senderName;
        std::vector<std::string> args;
    };

    using CommandFunc = std::function<std::string(CommandContext&)>;

    CommandHandler();

    void registerCommand(const std::string& name, CommandFunc func, bool requiresOp = true);
    std::string execute(const std::string& input, CommandContext& ctx, bool senderIsOp);

    bool isCommand(const std::string& input) const;
    std::vector<std::string> getCompletions(const std::string& prefix) const;

private:
    std::string handleGamemode(CommandContext& ctx);
    std::string handleOp(CommandContext& ctx);
    std::string handleDeop(CommandContext& ctx);
    std::string handleSay(CommandContext& ctx);
    std::string handleTp(CommandContext& ctx);
    std::string handleTime(CommandContext& ctx);
    std::string handleHelp(CommandContext& ctx);

    struct RegisteredCommand {
        CommandFunc func;
        bool requiresOp;
    };
    std::vector<std::pair<std::string, RegisteredCommand>> m_commands;
};
