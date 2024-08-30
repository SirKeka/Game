#include "dynamic_allocator.hpp"
#include "core/asserts.hpp"
#include "core/logger.hpp"
#include "core/mmemory.hpp"

struct AllocHeader {
    void* start;
    u16 alignment;
};

// Размер хранилища в байтах размера блока пользовательской памяти узла
constexpr auto SIZE_STORAGE = sizeof(u32);

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
        // Требуемый размер рассчитывается на основе запрошенного размера, а также выравнивания, 
        // заголовка и u32 для хранения размера для быстрого/легкого поиска.
        u64 RequiredSize = alignment + sizeof(AllocHeader) + SIZE_STORAGE + size;
        // ПРИМЕЧАНИЕ: Это преобразование действительно будет проблемой только при выделениях более ~4ГиБ, так что... не делайте этого.
        MASSERT_MSG(RequiredSize < 4294967295U, "DynamicAllocator::AllocateAligned вызывается с требуемым размером > 4ГиБ. Не делайте этого.");

        u64 BaseOffset = 0;
        if (state->list.AllocateBlock(RequiredSize, BaseOffset)) {
            /*
            Схема памяти:
            x bytes/void заполнение
            4 bytes/u32 размер пользовательского блока
            x bytes/void блок пользовательской памяти
            alloc_header

            */
            // Получить базовый указатель или невыровненный блок памяти.
            auto ptr = reinterpret_cast<void*>((u64)state->MemoryBlock + BaseOffset);
            // Начните выравнивание после достаточного пространства 
            // для хранения u32. Это позволяет хранить u32 непосредственно 
            // перед блоком пользователя, сохраняя при этом выравнивание на указанном блоке пользователя.
            u64 AlignedBlockOffset = Range::GetAligned((u64)ptr + BaseOffset + SIZE_STORAGE, alignment);
            // Сохраните размер непосредственно перед блоком пользовательских данных
            u32* BlockSize = reinterpret_cast<u32*>(AlignedBlockOffset - SIZE_STORAGE);
            *BlockSize = (u32)size;
            // Сохраните заголовок непосредственно после пользовательского блока.
            auto header = reinterpret_cast<AllocHeader*>(AlignedBlockOffset + size);
            header->start = ptr;
            header->alignment = alignment;

            return reinterpret_cast<void*>(AlignedBlockOffset);

        } else {
            MERROR("DynamicAllocator::AllocateBlockAligned нет блоков памяти, достаточно больших для выделения.");
            u64 available = state->list.FreeSpace();
            MERROR("Запрошенный размер: %llu, общее доступное пространство: %llu", size, available);
            // ЗАДАЧА: Report fragmentation?
            return nullptr;
        }
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

    u32* BlockSize = reinterpret_cast<u32*>((u64)block - SIZE_STORAGE);
    auto header = reinterpret_cast<AllocHeader*>((u64)block + *BlockSize);
    u64 required_size = header->alignment + sizeof(AllocHeader) + SIZE_STORAGE + *BlockSize;
    u64 offset = (u64)header->start - (u64)state->MemoryBlock;
    if (!state->list.FreeBlock(required_size, offset)) {
        MERROR("DynamicAllocator::FreeAligned failed.");
        return false;
    }
    return true;
}

bool DynamicAllocator::GetSizeAlignment(void *block, u64 &OutSize, u16 &OutAlignment)
{
    // Получите заголовок.
    OutSize = *reinterpret_cast<u32*>((u64)block - SIZE_STORAGE);
    auto header = reinterpret_cast<AllocHeader*>((u64)block + OutSize);
    OutAlignment = header->alignment;
    return true;
}

u64 DynamicAllocator::FreeSpace()
{
    return state->list.FreeSpace();
}

u64 DynamicAllocator::TotalSpace()
{
    return state->TotalSize;
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
