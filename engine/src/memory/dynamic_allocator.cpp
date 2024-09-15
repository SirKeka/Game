#include "dynamic_allocator.hpp"
#include "core/asserts.hpp"
#include "core/logger.hpp"
#include "core/mmemory.hpp"

struct AllocInfo {
    u8 alignment;
    u8 shift;
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
    if (state->MemoryBlock && size) {
        u64 BaseOffset = 0;
        if (state->list.AllocateBlock(size, BaseOffset)) {
            u8* ptr = reinterpret_cast<u8*>(state->MemoryBlock) + BaseOffset;
            return ptr;
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

void *DynamicAllocator::AllocateAligned(u64 size, u16 alignment)
{
    if (state->MemoryBlock && size) {
        // Требуемый размер рассчитывается на основе запрошенного размера, а также выравнивания, 
        // заголовка и u32 для хранения размера для быстрого/легкого поиска.
        // u64 AllocInfoSize = sizeof(AllocInfo);
        u64 StorageSize = SIZE_STORAGE;
        u64 RequiredSize = alignment + StorageSize + size + 1;
        // ПРИМЕЧАНИЕ: Это преобразование действительно будет проблемой только при выделениях более ~4ГиБ, так что... не делайте этого.
        MASSERT_MSG(RequiredSize < 4294967295U, "DynamicAllocator::AllocateAligned вызывается с требуемым размером > 4ГиБ. Не делайте этого.");

        u64 BaseOffset = 0;
        if (state->list.AllocateBlock(RequiredSize, BaseOffset)) {
            /*
            Схема памяти:
            4 байта/u32 Общий размер всего блока памяти.
            1 байт/u8 Размер выравнивания.
            x bytes/void заполнение
            1 байт/u8 смещение
            x bytes/void блок пользовательской памяти
            */

            // Получить базовый указатель или невыровненный блок памяти.
            u8* ptr = reinterpret_cast<u8*>(state->MemoryBlock) + BaseOffset;

            // Начните выравнивание после достаточного пространства для хранения размера всего
            // блока и размера выравнивания. Это позволяет хранить данные непосредственно перед блоком
            // пользователя, сохраняя при этом выравнивание на указанном блоке пользователя.
            u8* AlignedBlock = reinterpret_cast<u8*>(Range::GetAligned((u64)ptr + StorageSize + 1, alignment));

            // Если выравнивания не произошло, сдвигаем его на все байты,
            // чтобы всегда было место для хранения смещения.
            if (AlignedBlock == (ptr + StorageSize + 1)) {
                AlignedBlock += alignment;
            }

            // Определить сдвиг и сохранить его непосредственно перед блоком пользовательских данных.
            // (Это работает для выравнивания до 256 байт.)
            u64 shift = AlignedBlock - ptr - StorageSize - 1;
            MASSERT(shift > 0 && shift <= 256);
            AlignedBlock[-1] = static_cast<u8>(shift & 0xFF);

            // Общий размер выравненного блока памяти
            u32* BlockSize = reinterpret_cast<u32*>(ptr);
            *BlockSize = static_cast<u32>(RequiredSize);

            // Размер выравнивания
            ptr[SIZE_STORAGE] = alignment;

            return AlignedBlock;
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

bool DynamicAllocator::Free(void *block, u64 size, bool aligned)
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

    u8* MemBlock = reinterpret_cast<u8*>(block);
    u32 BlockSize = size;
    if (aligned) {
        // Извлекаем размер смещения
        u64 shift = MemBlock[-1] ? MemBlock[-1] : 256;
        // Получаем указатель на первую ячейку блока данных
        MemBlock = MemBlock - SIZE_STORAGE - 1 - shift;
        // Извлекаем общий размер блока
        BlockSize = *(reinterpret_cast<u32*>(MemBlock));
    }
    
    u64 offset = reinterpret_cast<u64>(MemBlock) - reinterpret_cast<u64>(state->MemoryBlock);
    if (!state->list.FreeBlock(BlockSize, offset)) {
        MERROR("DynamicAllocator::FreeAligned failed.");
        return false;
    }
    return true;
}

bool DynamicAllocator::GetSizeAlignment(void *block, u64 &OutSize, u16 &OutAlignment)
{
    u8* MemBlock = reinterpret_cast<u8*>(block);
    u16 shift = MemBlock[-1] ? MemBlock[-1] : 256;
    MemBlock = MemBlock - shift - SIZE_STORAGE - 1;
    OutSize = *reinterpret_cast<u32*>(MemBlock);
    OutAlignment = *reinterpret_cast<u8*>(MemBlock + SIZE_STORAGE);
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
