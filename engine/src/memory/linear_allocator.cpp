#include "linear_allocator.hpp"

#include "core/mmemory.hpp"

LinearAllocator LinearAllocator::state;

LinearAllocator::LinearAllocator(u64 TotalSize, void *memory)
:
    TotalSize(TotalSize),
    allocated(),
    memory(memory ? memory : MMemory::Allocate(TotalSize, MemoryTag::LinearAllocator)),
    OwnsMemory(memory == nullptr)
{
    /*this->TotalSize = TotalSize;
    this->allocated = 0;
    this->OwnsMemory = memory == nullptr;
    if (memory) {
        this->memory = memory;
    } else {
        this->memory = MMemory::Allocate(TotalSize, MemoryTag::LinearAllocator);
    }*/
}

LinearAllocator::~LinearAllocator()
{
    if (TotalSize != 0) {
        allocated = 0;
        OwnsMemory = memory == nullptr;

        if (OwnsMemory && memory) MMemory::Free(memory, TotalSize, MemoryTag::LinearAllocator);

        memory = nullptr;
        TotalSize = 0;
        OwnsMemory = false;
    }
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
        MMemory::ZeroMem(memory, TotalSize);
    }
}

void LinearAllocator::Initialize(u64 TotalSize, void *memory)
{
    state = LinearAllocator(TotalSize, memory);
}