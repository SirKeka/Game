#pragma once

#include "defines.hpp"
#include "containers/freelist.hpp"

class MAPI DynamicAllocator
{
private:
    u64 TotalSize;
    FreeList list;
    void* FreelistBlock;
    void* MemoryBlock;
public:
    DynamicAllocator() : TotalSize(), list(), FreelistBlock(nullptr), MemoryBlock(nullptr) {}
    ~DynamicAllocator();

    bool GetMemoryRequirement(u64 TotalSize, u64 &MemoryRequirement);
    bool Create(u64 TotalSize, u64 &MemoryRequirement, void *memory);
    bool Create(void *memory);
    bool Destroy();
    void *Allocate(u64 size);
    void Free(void *block, u64 size);
    u64 FreeSpace();

    operator bool() const;
private:
    u64 GetFreeListRequirement();
};
