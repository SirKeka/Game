#include "dynamic_allocator.hpp"
#include "core/logger.hpp"
#include "core/mmemory.hpp"

DynamicAllocator::~DynamicAllocator()
{
    //if (state) {
    //    MMemory::ZeroMem(state->MemoryBlock, state->TotalSize);
    //    state->TotalSize = 0;
    //    state = nullptr;
    //}
}

bool DynamicAllocator::MemoryRequirement(u64 TotalSize, u64 &MemoryRequirement)
{
    if (TotalSize < 1) {
        MERROR("DynamicAllocator::GetMemoryRequirement не может иметь значение TotalSize, равное 0.");
        return false;
    }

    MemoryRequirement = FreeList::GetMemoryRequirement(TotalSize) + sizeof(DynamicAllocatorState) + TotalSize;

    return true;
}

bool DynamicAllocator::GetMemoryRequirement(u64 TotalSize, u64 &MemoryRequirement)
{
    if (TotalSize < 1) {
        MERROR("DynamicAllocator::GetMemoryRequirement не может иметь значение TotalSize, равное 0.");
        return false;
    }
    if (state) {
        state->TotalSize = TotalSize;
    }
    
    MemoryRequirement = GetFreeListRequirement(TotalSize) + sizeof(DynamicAllocatorState) + TotalSize;

    return true;
}

bool DynamicAllocator::Create(u64 TotalSize, u64 &MemoryRequirement, void *memory)
{
    if (!memory) {
        return false;
    }
    
    if (GetMemoryRequirement(TotalSize, MemoryRequirement)) {
        Create(MemoryRequirement, memory);
        return true;
    }
    
    return false;
}

bool DynamicAllocator::Create(u64 MemoryRequirement, void *memory)
{
    if (memory) {
        // Memory layout:
        // state
        // freelist block
        // memory block
        state = reinterpret_cast<DynamicAllocatorState*>(memory);
        state->TotalSize = MemoryRequirement;
        state->FreelistBlock = reinterpret_cast<u8*>(memory) + sizeof(DynamicAllocatorState);
        state->MemoryBlock = reinterpret_cast<u8*>(state->FreelistBlock) + GetFreeListRequirement(MemoryRequirement);

        //MMemory::ZeroMem(state->MemoryBlock, state->TotalSize);

        // Собственно создайте свободный список
        state->list.Create(state->TotalSize, state->FreelistBlock);
        return true;
    }
    
    MERROR("DynamicAllocator::Create должен принимать не нулевой указатель. Не удалось создать.")
    return false;
}

bool DynamicAllocator::Destroy()
{
    if (state) {
        this->~DynamicAllocator();
        return true;
    }
    
    MWARN("DynamicAllocator::Destroy блок памяти не распределен. Уничтожить не удалось.");
    return false;
}

void *DynamicAllocator::Allocate(u64 size)
{
    if (state->MemoryBlock && size) {
        u64 offset = 0;
        // Попытайтесь выделить из свободного списка.
        if (state->list.AllocateBlock(size, offset)) {
            // Используйте это смещение относительно блока базовой памяти, чтобы получить блок.
            u8* block = (reinterpret_cast<u8*>(state->MemoryBlock) + offset);
            return block;
        } else {
            MERROR("DynamicAllocator::Allocate нет блоков памяти, достаточно больших для выделения.");
            u64 available = state->list.FreeSpace();
            MERROR("Запрошенный размер: %llu, общее доступное пространство: %llu", size, available);
            // ЗАДАЧА: Report fragmentation?
            return nullptr;
        }
    }
    MERROR("DynamicAllocator::Allocate требуется размер.");
    return nullptr;
}

bool DynamicAllocator::Free(void *block, u64 size)
{
    if (!block) {
        MERROR("DynamicAllocator::Free требует освобождения блока (0x%p).", block);
        return false;
    }

    // DynamicAllocatorState* state = allocator->memory;
    if (block < state->MemoryBlock || block > reinterpret_cast<u8*>(state->MemoryBlock) + state->TotalSize) {
        void* EndOfBlock = reinterpret_cast<u8*>(state->MemoryBlock) + state->TotalSize;
        MERROR("DynamicAllocator::Free попытка освободить блок (0x%p) за пределами диапазона распределителя (0x%p)-(0x%p).", block, state->MemoryBlock, EndOfBlock);
        return false;
    }
    u64 offset = (reinterpret_cast<u8*>(block) - reinterpret_cast<u8*>(state->MemoryBlock));
    if (!state->list.FreeBlock(size, offset)) {
        MERROR("DynamicAllocator::Free failed.");
        return false;
    }
    return true;
}

u64 DynamicAllocator::FreeSpace()
{
    return state->list.FreeSpace();
}

DynamicAllocator::operator bool() const
{
    if (state) { // TotalSize != 0 && (bool)state->list && FreelistBlock && MemoryBlock
        return true;
    }
    return false;
}

u64 DynamicAllocator::GetFreeListRequirement(u64 TotalSize)
{
    u64 FreelistRequirement = 0;
    // Сначала узнайте объем памяти, необходимый для списка свободных мест.
    state->list.GetMemoryRequirement(TotalSize, FreelistRequirement);
    return FreelistRequirement;
}
