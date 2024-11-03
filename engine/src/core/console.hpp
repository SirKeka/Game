#pragma once
#include "defines.hpp"
#include "logger.hpp"
#include "containers/darray.hpp"

class LinearAllocator;

using PFN_ConsoleConsumerWrite = bool(*)(void*, LogLevel, const char*);

struct ConsoleCommandArgument {
    const char* value;
};

struct ConsoleCommandContext {
    u8 ArgumentCount;
    ConsoleCommandArgument* arguments;
};

using PFN_ConsoleCommand = void(*)(ConsoleCommandContext);

class Console
{
    struct Consumer {
        PFN_ConsoleConsumerWrite callback;
        void* instance;
    };

    struct Command {
        MString name;
        u8 ArgCount;
        PFN_ConsoleCommand func;
    };

    u8 ConsumerCount;
    Consumer* consumers;

    DArray<Command> RegisteredCommands;

    static Console* pConsole;

    constexpr Console(Consumer* consumers);

public:
    ~Console();

    static void Initialize(LinearAllocator& SystemAllocator);
    static void Shutdown();

    MAPI static void RegisterConsumer(void* inst, PFN_ConsoleConsumerWrite callback);

    static void WriteLine(LogLevel level, const char* message);

    MAPI static bool RegisterCommand(const char* command, u8 ArgCount, PFN_ConsoleCommand func);

    MAPI static bool ExecuteCommand(const MString& command);
};

