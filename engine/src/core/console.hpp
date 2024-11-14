#pragma once
#include "defines.hpp"
#include "logger.hpp"
#include "containers/darray.hpp"

class LinearAllocator;

using PFN_ConsoleConsumerWrite = bool(*)(void*, Log::Level, const char*);

struct ConsoleCommandArgument {
    const char* value;
};

struct ConsoleCommandContext {
    u8 ArgumentCount;
    ConsoleCommandArgument* arguments;
};

using PFN_ConsoleCommand = void(*)(ConsoleCommandContext);

namespace Console
{
    bool Initialize(u64& MemoryRequirement, void* memory, void* config); // LinearAllocator& SystemAllocator
    void Shutdown();

    MAPI void RegisterConsumer(void* inst, PFN_ConsoleConsumerWrite callback);

    void WriteLine(Log::Level level, const char* message);

    MAPI bool RegisterCommand(const char* command, u8 ArgCount, PFN_ConsoleCommand func);

    MAPI bool ExecuteCommand(const MString& command);
};

