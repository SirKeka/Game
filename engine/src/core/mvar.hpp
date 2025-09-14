#pragma once

#include "defines.h"

namespace MVar
{
    bool Initialize(u64& MemoryRequirement, void* memory, void* config);
    void Shutdown();

    MAPI bool CreateInt(const char* name, i32 value);
    MAPI bool GetInt(const char* name, i32& OutValue);
    MAPI bool SetInt(const char* name, i32 value);
} // namespace MVar

