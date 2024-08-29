#include "dynamic_allocator.hpp"
#include "core/logger.hpp"
#include "core/mmemory.hpp"

struct AllocHeader {
    u64 size;
    u16 alignment;
    u16 AlignmentOffset;
};

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
    return AllocateAligned(size, 1);
}

void *DynamicAllocator::AllocateAligned(u64 size, u16 alignment)
{
    if (state->MemoryBlock && size) {
        u64 offset = 0;

        // Учитывайте место для заголовка.
        u64 ActualSize = size + sizeof(AllocHeader);
        u16 AlignmentOffset = 0;

        // Попытайтесь выделить из свободного списка.
        void* block = nullptr;
        if (state->list.AllocateBlockAligned(ActualSize, alignment, offset, AlignmentOffset)) {
            // Установите информацию заголовка.
            auto header = reinterpret_cast<AllocHeader*>(((u8*)state->MemoryBlock) + offset);
            header->alignment = alignment;
            header->AlignmentOffset = AlignmentOffset;
            header->size = size;  // Сохраните фактический размер здесь.
            // Блок — это state->memoryblock, затем offset, затем после заголовка.
            block = reinterpret_cast<void*>(((u8*)state->MemoryBlock) + offset + sizeof(AllocHeader));
        } else {
            MERROR("DynamicAllocator::AllocateBlockAligned нет блоков памяти, достаточно больших для выделения.");
            u64 available = state->list.FreeSpace();
            MERROR("Запрошенный размер: %llu, общее доступное пространство: %llu", size, available);
            // ЗАДАЧА: Report fragmentation?
            block = nullptr;
        }
        return block;
    }
    MERROR("DynamicAllocator::AllocateBlockAligned требуется размер.");
    return nullptr;
}

bool DynamicAllocator::Free(void *block, u64 size)
{
    return FreeAligned(block);
}

bool DynamicAllocator::FreeAligned(void *block)
{
    if (!block) {
        MERROR("DynamicAllocator::FreeAligned требует освобождения блока (0x%p).", block);
        return false;
    }

    // DynamicAllocatorState* state = allocator->memory;
    if (block < state->MemoryBlock || block > reinterpret_cast<u8*>(state->MemoryBlock) + state->TotalSize) {
        void* EndOfBlock = reinterpret_cast<u8*>(state->MemoryBlock) + state->TotalSize;
        MERROR("DynamicAllocator::FreeAligned попытка освободить блок (0x%p) за пределами диапазона распределителя (0x%p)-(0x%p).", block, state->MemoryBlock, EndOfBlock);
        return false;
    }
    // Получите заголовок.
    auto header = reinterpret_cast<AllocHeader*>(((u8*)block) - sizeof(AllocHeader));
    u64 ActualSize = header->size + sizeof(AllocHeader);
    u64 offset = (reinterpret_cast<u8*>(block) - reinterpret_cast<u8*>(state->MemoryBlock));
    if (!state->list.FreeBlockAligned(ActualSize, offset - sizeof(AllocHeader), header->AlignmentOffset)) {
        MERROR("DynamicAllocator::FreeAligned failed.");
        return false;
    }
    return true;
}

bool DynamicAllocator::GetSizeAlignment(void *block, u64 &OutSize, u16 &OutAlignment)
{
    // Получите заголовок.
    auto header = reinterpret_cast<AllocHeader*>((u8*)block - sizeof(AllocHeader));
    OutSize = header->size;
    OutAlignment = header->alignment;
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
