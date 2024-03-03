#pragma once

#include "core/mmemory.hpp"

class MAPI LinearAllocator
{
public:
    u64 TotalSize;
    u64 allocated;
    void* memory;
    bool OwnsMemory;
public:
    LinearAllocator() = default;
    LinearAllocator(u64 TotalSize, void* memory = nullptr);
    ~LinearAllocator();

    void* Allocate(u64 size);
    void FreeAll();
};
