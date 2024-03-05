#pragma once

#include "defines.hpp"

class MAPI LinearAllocator
{
public:
    u64 TotalSize{0};
    u64 allocated{0};
    void* memory{nullptr};
    bool OwnsMemory{false};
public:
    LinearAllocator() : TotalSize(0), allocated(0), memory(nullptr), OwnsMemory(false) {}
    LinearAllocator(u64 TotalSize, void* memory = nullptr);
    ~LinearAllocator();

    void* Allocate(u64 size);
    void FreeAll();
};

template <typename T>
class WrapLinearAllocator
{

    static LinearAllocator la () {
        static LinearAllocator linearallocator(sizeof(T));
        return linearallocator;
    }
public:
    static void* Allocate(u64 size) { return la().Allocate(size); }
    static void  Free(void* ptr) { la().FreeAll; }

};
