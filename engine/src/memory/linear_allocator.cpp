#include "linear_allocator.hpp"

LinearAllocator::LinearAllocator(u64 TotalSize, void * memory)
{
    if (this->TotalSize != 0) {
        this->TotalSize = TotalSize;
        this->allocated = 0;
        this->OwnsMemory = memory == nullptr;
        if (memory) {
            this->memory = memory;
        } else {
            this->memory = MMemory::Allocate(TotalSize, MEMORY_TAG_LINEAR_ALLOCATOR);
        }
    }
}

LinearAllocator::~LinearAllocator()
{
    if (TotalSize != 0) {
        allocated = 0;
        OwnsMemory = memory == nullptr;

        if (OwnsMemory && memory) MMemory::Free(memory, TotalSize, MEMORY_TAG_LINEAR_ALLOCATOR);

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
            MERROR("Linear Allocator::Allocate - Попытка выделить %lluB, только %lluB осталось.", size, remaining);
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
