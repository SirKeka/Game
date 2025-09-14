#include "linear_allocator.hpp"

#include "core/memory_system.h"

constexpr LinearAllocator::LinearAllocator(u64 TotalSize, void *memory)
:
    TotalSize(TotalSize),
    allocated(),
    memory(memory ? memory : MemorySystem::Allocate(TotalSize, Memory::LinearAllocator, true)),
    OwnsMemory(memory == nullptr)
{}

LinearAllocator::~LinearAllocator()
{
    if (TotalSize != 0) {
        allocated = 0;
        OwnsMemory = memory == nullptr;

        if (OwnsMemory && memory) MemorySystem::Free(memory, TotalSize, Memory::LinearAllocator);

        memory = nullptr;
        TotalSize = 0;
        OwnsMemory = false;
    }
}

void LinearAllocator::Initialize(u64 TotalSize, void *memory)
{
    this->TotalSize = TotalSize;
    this->memory = memory ? memory : MemorySystem::Allocate(TotalSize, Memory::LinearAllocator, true);
    OwnsMemory = memory == nullptr;
}

void *LinearAllocator::Allocate(u64 size)
{
    if (memory) {
        if (allocated + size > TotalSize) {
            u64 remaining = TotalSize - allocated;
            MERROR("LinearAllocator::AllocateConstruct - Попытка выделить %llu байт, только %llu байт осталось.", size, remaining);
            return nullptr;
        }
        void* block = reinterpret_cast<u8*>(memory) + allocated;
        allocated += size;
        return block;
    }
    MERROR("Linear Allocator::Allocate - предоставленный распределитель не инициализирован.");
    return nullptr;
}

void LinearAllocator::FreeAll()
{
    if (memory) {
       allocated = 0;
        MemorySystem::ZeroMem(memory, TotalSize);
    }
}
