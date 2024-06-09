#pragma once

#include "defines.hpp"
#include "containers/freelist.hpp"

class MAPI DynamicAllocator
{
private:
    struct DynamicAllocatorState {
        u64 TotalSize{};
        FreeList list{};
        void* FreelistBlock{nullptr};
        void* MemoryBlock{nullptr};
    }* state;
public:
    DynamicAllocator() : state(nullptr) {} // TotalSize(), list(), FreelistBlock(nullptr), MemoryBlock(nullptr) {}
    ~DynamicAllocator();

    //bool GetMemoryRequirement(u64 TotalSize, u64 &MemoryRequirement);
    static bool MemoryRequirement(u64 TotalSize, u64 &MemoryRequirement);
    bool GetMemoryRequirement(u64 TotalSize, u64 &MemoryRequirement);
    bool Create(u64 TotalSize, u64 &MemoryRequirement, void *memory);
    /// @brief Создает динамический распредлитель
    /// @param MemoryRequirement размер указателя на область памяти которым будет управлять данный распределитель
    /// @param memory указатель на область памяти где которой будет управлять данный распределитель
    /// @return true если создан успешно, иначе false
    bool Create(u64 MemoryRequirement, void *memory);
    bool Destroy();
    void *Allocate(u64 size);
    bool Free(void *block, u64 size);
    u64 FreeSpace();

    operator bool() const;
private:
    u64 GetFreeListRequirement(u64 TotalSize);
};
