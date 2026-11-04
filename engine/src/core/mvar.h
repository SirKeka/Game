#pragma once

#include "containers/mstring.h"

struct ConsoleCommandContext;

class MVar
{
    struct IntEntry
    {
        MString name;
        i32 value;
    };

    static constexpr u32 MVAR_INT_MAX_COUNT = 200;
    IntEntry ints[MVAR_INT_MAX_COUNT];

    static inline MVar* state;

    constexpr MVar() : ints() {}
    static void ConsoleCommandCreateInt(ConsoleCommandContext& context);
    static void ConsoleCommandPrintInt(ConsoleCommandContext& context);
    static void ConsoleCommandSetInt(ConsoleCommandContext& context);
    static void ConsoleCommandPrintAll(ConsoleCommandContext& context);
public:
    MVar(const MVar&) = delete;
    MVar& operator=(MVar&) = delete;

    static MVar* Initialize(u64& MemoryRequirement, void* memory);
    void Shutdown();

    MAPI static bool CreateInt(const char* name, i32 value);
    MAPI static bool GetInt(const char* name, i32& OutValue);
    MAPI static bool SetInt(const char* name, i32 value);
}; // class MVar

