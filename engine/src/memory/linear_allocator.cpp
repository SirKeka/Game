#include "linear_allocator.hpp"

#include "core/mmemory.hpp"

LinearAllocator::LinearAllocator(u64 TotalSize, void *memory)
{
    if (this->TotalSize == 0) {
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

void LinearAllocator::FreeAll()
{
    if (memory) {
       allocated = 0;
        MMemory::ZeroMem(memory, TotalSize);
    }
}
