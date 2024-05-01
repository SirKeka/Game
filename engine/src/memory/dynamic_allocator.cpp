#include "dynamic_allocator.hpp"
#include "core/logger.hpp"
#include "core/mmemory.hpp"

DynamicAllocator::~DynamicAllocator()
{
    //DynamicAllocatorState* state = allocator->memory;
    list.~FreeList();
    MMemory::ZeroMem(this->MemoryBlock, this->TotalSize);
    this->TotalSize = 0;
}

bool DynamicAllocator::GetMemoryRequirement(u64 TotalSize, u64 &MemoryRequirement)
{
    if (TotalSize < 1) {
        MERROR("DynamicAllocator::GetMemoryRequirement не может иметь значение TotalSize, равное 0. Создание не удалось.");
        return false;
    }

    this->TotalSize = TotalSize;
    MemoryRequirement = GetFreeListRequirement() + sizeof(DynamicAllocator) + TotalSize;

    return true;
}

bool DynamicAllocator::Create(u64 TotalSize, u64 &MemoryRequirement, void *memory)
{
    if (GetMemoryRequirement(TotalSize, MemoryRequirement)) {
        Create(memory);
        return true;
    }
    
    return false;
}

bool DynamicAllocator::Create(void *memory)
{
    if (memory) {
        // Memory layout:
        // state
        // freelist block
        // memory block
        this->FreelistBlock = reinterpret_cast<u8*>(memory) + sizeof(DynamicAllocator);
        this->MemoryBlock = reinterpret_cast<u8*>(this->FreelistBlock) + GetFreeListRequirement();

        MMemory::ZeroMem(this->MemoryBlock, TotalSize);

        // Собственно создайте свободный список
        list.Create(TotalSize, this->FreelistBlock);
        return true;
    }
    
    MERROR("DynamicAllocator::Create должен принимать не нулевой указатель. Не удалось создать.")
    return false;
}

bool DynamicAllocator::Destroy()
{
    if (this->MemoryBlock) {
        this->~DynamicAllocator();
        return true;
    }
    
    MWARN("DynamicAllocator::Destroy блок памяти не распределен. Уничтожить не удалось.");
    return false;
}

void *DynamicAllocator::Allocate(u64 size)
{
    if (MemoryBlock && size) {
        // DynamicAllocatorState* state = allocator->memory;
        u64 offset = 0;
        // Попытайтесь выделить из свободного списка.
        if (list.AllocateBlock(size, offset)) {
            // Используйте это смещение относительно блока базовой памяти, чтобы получить блок.
            void* block = reinterpret_cast<u8*>(this->MemoryBlock) + offset;
            return block;
        } else {
            MERROR("DynamicAllocator::Allocate нет блоков памяти, достаточно больших для выделения.");
            u64 available = list.FreeSpace();
            MERROR("Запрошенный размер: %llu, общее доступное пространство: %llu", size, available);
            // СДЕЛАТЬ: Report fragmentation?
            return nullptr;
        }
    }
    MERROR("DynamicAllocator::Allocate требуется размер.");
    return nullptr;
}

void DynamicAllocator::Free(void *block, u64 size)
{
    if (!block) {
        MERROR("DynamicAllocator::Free требует освобождения блока (0x%p).", block);
        return;
    }

    // DynamicAllocatorState* state = allocator->memory;
    if (block < this->MemoryBlock || block > reinterpret_cast<u8*>(this->MemoryBlock) + this->TotalSize) {
        void* EndOfBlock = reinterpret_cast<u8*>(this->MemoryBlock) + this->TotalSize;
        MERROR("DynamicAllocator::Free попытка освободить блок (0x%p) за пределами диапазона распределителя (0x%p)-(0x%p).", block, this->MemoryBlock, EndOfBlock);
        return;
    }
    u64 offset = reinterpret_cast<u8*>(block) - reinterpret_cast<u8*>(this->MemoryBlock);
    if (!list.FreeBlock(size, offset)) {
        MERROR("DynamicAllocator::Free failed.");
        return;
    }
}

u64 DynamicAllocator::FreeSpace()
{
    return list.FreeSpace();
}

DynamicAllocator::operator bool() const
{
    if (TotalSize != 0 && (bool)list && FreelistBlock && MemoryBlock) {
        return true;
    }
    return false;
}

u64 DynamicAllocator::GetFreeListRequirement()
{
    u64 FreelistRequirement = 0;
    // Сначала узнайте объем памяти, необходимый для списка свободных мест.
    list.GetMemoryRequirement(this->TotalSize, FreelistRequirement);
    return FreelistRequirement;
}
