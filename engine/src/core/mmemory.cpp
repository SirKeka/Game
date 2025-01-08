#include "mmemory.hpp"

#include "platform/platform.hpp"
#include "core/engine.hpp"

#include <cstring>
#include <stdio.h>
#include <new>

static const char* MemoryTagStrings[Memory::MaxTags] = {
    "UNKNOWN    ",
    "ARRAY      ",
    "STACK      ",
    "LINEAR ALLC",
    "DARRAY     ",
    "DICT       ",
    "RING QUEUE ",
    "BST        ",
    "STRING     ",
    "ENGINE     ",
    "JOB        ",
    "TEXTURE    ",
    "MAT INST   ",
    "RENDERER   ",
    "GAME       ",
    "TRANSFORM  ",
    "ENTITY     ",
    "ENTITY NODE",
    "SCENE      ",
    "HASHTABLE  ",
    "RESOURCE   ",
    "VULKAN     ",
    "VULKAN_EXT ",
    "DIRECT3D   ",
    "OPENGL     ",
    "GPU_LOCAL  ",
    "BITMAP_FONT",
    "SYSTEM_FONT",
    "KEYMAP     "
};

MemorySystem::MemorySystem(u64 TotalAllocSize, u64 AllocatorMemoryRequirement, void* AllocatorBlock) 
: 
TotalAllocSize(TotalAllocSize), 
TotalAllocated(), 
TaggedAllocations(), 
AllocCount(), 
AllocatorMemoryRequirement(AllocatorMemoryRequirement), 
allocator(), 
AllocatorBlock(AllocatorBlock), 
AllocationMutex()
{}

MemorySystem::~MemorySystem()
{
    if (state) {
        state->allocator.Destroy();
        // Освободите целый блок.
        //platform_free(state_ptr, state_ptr->allocator_memory_requirement + sizeof(memory_system_state));
        delete[] state;
    }
}

bool MemorySystem::Initialize(u64 TotalAllocSize)
{
    // Сумма, необходимая состоянию системы.
    u64 StateMemoryRequirement = sizeof(MemorySystem);

    // Выясните, сколько места нужно динамическому распределителю.
    u64 AllocRequirement = 0;
    DynamicAllocator::MemoryRequirement(TotalAllocSize, AllocRequirement);

    // Вызовите распределитель платформы, чтобы получить память для всей системы, включая состояние.
    // ЗАДАЧА: выравнивание памяти
    u8* block = new u8[StateMemoryRequirement + AllocRequirement](); // platform_allocate(StateMemoryRequirement + AllocRequirement, false);
    if (!block) {
        MFATAL("Не удалось выделить память в системе, и система не может продолжить работу.");
        return false;
    }

    // Состояние находится в первой части массивного блока памяти.
    state = new(block) MemorySystem(
        TotalAllocSize, 
        AllocRequirement, 
        block + StateMemoryRequirement // Блок распределителя находится в том же блоке памяти, но после состояния.
    );
    
    // state->AllocatorBlock = reinterpret_cast<void*>(block + StateMemoryRequirement);

    if (!state->allocator.Create(TotalAllocSize, state->AllocatorBlock)) {
        MFATAL("Система памяти не может настроить внутренний распределитель. Работа приложения не может быть продолжена.");
        return false;
    }

    // Создание мьютекса распредлителя
    if (!state->AllocationMutex) {
        MFATAL("Невозможно создать мьютекс выделения!");
        return false;
    }

    MDEBUG("Система памяти успешно выделила %llu байт.", TotalAllocSize);
    return true;
}

MINLINE void MemorySystem::Shutdown()
{
    if (state) {
        //state->allocator.Destroy();
        // Освободите целый блок.
        //platform_free(state_ptr, state_ptr->allocator_memory_requirement + sizeof(memory_system_state));
        u8* block = reinterpret_cast<u8*>(state);
        state = nullptr;
        delete[] block;
    }
}

void *MemorySystem::Allocate(u64 bytes, Memory::Tag tag, bool nullify, bool def)
{
    return AllocateAligned(bytes, 1, tag, nullify, def);
}

void *MemorySystem::AllocateAligned(u64 bytes, u16 alignment, Memory::Tag tag, bool nullify, bool def)
{
    if (tag == Memory::Unknown) {
        MWARN("MMemory::AllocateAligned вызывается с использованием MemoryTag::Unknown. Переклассифицировать это распределение.");
    }

    // Либо выделяйте из системного распределителя, либо из ОС. Последнее никогда не должно произойти.
    u8* block = nullptr;

    if (state && !def) {
        // Убедитесь, что многопоточные запросы не мешают друг другу.
        if (!state->AllocationMutex.Lock()) {
            MFATAL("Ошибка получения блокировки мьютекса во время выделения.");
            return nullptr;
        }

        if (alignment > 1) {
            block = reinterpret_cast<u8*>(state->allocator.AllocateAligned(bytes, alignment));
        } else {
            block = reinterpret_cast<u8*>(state->allocator.Allocate(bytes));
        }
        
        state->AllocationMutex.Unlock();
    } else {
        if (!state) {
            // Если система еще не запустилась, предупредите об этом, но дайте пока память.
            MWARN("Memory::AllocateAligned вызывается перед инициализацией системы памяти.");
        }
        // ЗАДАЧА: Memory alignment
        block = new u8[bytes];
    }

    if (block) {
        state->TotalAllocated += bytes;
        state->TaggedAllocations[tag] += bytes;
        state->AllocCount++;
        if (nullify) {
            MemorySystem::ZeroMem(block, alignment > 2 ? bytes - alignment - 5 : bytes);
        }
        return block;
    }
    
    MFATAL("MMemory::AllocateAligned не удалось успешно распределить.");
    return nullptr;
}

void *MemorySystem::Realloc(void *ptr, u64 size, u64 NewSize, Memory::Tag tag, bool copy)
{
    if (tag == Memory::Unknown) {
        MWARN("MMemory::AllocateAligned вызывается с использованием MemoryTag::Unknown. Переклассифицировать это распределение.");
    }

    // Либо выделяйте из системного распределителя, либо из ОС. Последнее никогда не должно произойти.
    u8* block = nullptr;
    u64 DeltaSize = 0;
    if (NewSize > size) {
        DeltaSize = NewSize - size;
    } else {
        Free((u8*)ptr + NewSize, size - NewSize, tag);
        return ptr;
    }

    if (state) {
        // Убедитесь, что многопоточные запросы не мешают друг другу.
        if (!state->AllocationMutex.Lock()) {
            MFATAL("Ошибка получения блокировки мьютекса во время выделения.");
            return nullptr;
        }

        if (ptr) {
            block = reinterpret_cast<u8*>(state->allocator.Realloc(ptr, size, NewSize));

            if (ptr != block && copy) {
                CopyMem(block, ptr, size);
            }
        } else {
            block = reinterpret_cast<u8*>(state->allocator.Allocate(NewSize));
        }
        
        state->AllocationMutex.Unlock();
    }

    if (block) {
        state->TotalAllocated += DeltaSize;
        state->TaggedAllocations[tag] += DeltaSize;
        return block;
    }
    
    MFATAL("MMemory::Realloc не удалось успешно распределить.");
    return nullptr;
}

void MemorySystem::AllocateReport(u64 size, Memory::Tag tag)
{
    // Убедитесь, что многопоточные запросы не подавляют друг друга.
    if (!state->AllocationMutex.Lock()) {
        MFATAL("Ошибка получения блокировки мьютекса во время отчета о распределении.");
        return;
    }
    state->TotalAllocated += size;
    state->TaggedAllocations[tag] += size;
    state->AllocCount++;
    state->AllocationMutex.Unlock();
}

void MemorySystem::Free(void *block, u64 bytes, Memory::Tag tag, bool def)
{
    FreeAligned(block, bytes, false, tag, def);
}

void MemorySystem::FreeAligned(void *block, u64 size, u16 alignment, Memory::Tag tag, bool def)
{
    if (block) {
        if (tag == Memory::Unknown) {
            MWARN("free вызывается с использованием MemoryTag::Unknown. Переклассифицировать это распределение.");
        }
        if (state && !def) {
            if (!state->AllocationMutex.Lock()) {
                MFATAL("Невозможно получить блокировку мьютекса для операции освобождения. Вероятно повреждение кучи.");
                return;
            }
            state->TotalAllocated -= size;
            state->TaggedAllocations[tag] -= size;
            state->AllocCount--;
            [[maybe_unused]]bool result = state->allocator.Free(block, size, alignment);
            // Если освобождение не удалось, возможно, это связано с тем, что выделение было выполнено до запуска этой системы. 
            // Поскольку это абсолютно должно быть исключением из правил, попробуйте освободить его на уровне платформы. 
            // Если это не удастся, значит, начнется какой-то другой вид мошенничества, и у нас возникнут более серьезные проблемы.
            /*if (!result) {
                // ЗАДАЧА: Выравнивание памяти
                u8* ptrRawMem = reinterpret_cast<u8*>(block);
                delete[] ptrRawMem;
            }*/
            state->AllocationMutex.Unlock();
        } else {
            u8* ptrRawMem = reinterpret_cast<u8*>(block);
            delete[] ptrRawMem;
        }
    }
}

void MemorySystem::FreeReport(u64 size, Memory::Tag tag)
{
    // Убедитесь, что многопоточные запросы не подавляют друг друга.
    if (!state->AllocationMutex.Lock()) {
        MFATAL("Ошибка получения блокировки мьютекса во время отчета о распределении.");
        return;
    }
    state->TotalAllocated -= size;
    state->TaggedAllocations[tag] -= size;
    state->AllocCount--;
    state->AllocationMutex.Unlock();
}

bool MemorySystem::GetSizeAlignment(void *block, u64 &OutSize, u16 &OutAlignment)
{
    return DynamicAllocator::GetSizeAlignment(block, OutSize, OutAlignment);
}

void *MemorySystem::ZeroMem(void *block, u64 bytes)
{
    return memset(block, 0, bytes);
}

void* MemorySystem::CopyMem(void *dest, const void *source, u64 bytes)
{
    return std::memmove(dest, source, bytes);
}

void *MemorySystem::SetMemory(void *dest, i32 value, u64 bytes)
{
    return std::memset(dest, value, bytes);
}

u64 MemorySystem::GetMemoryAllocCount()
{
    return state->AllocCount;
}

template <class U, class... Args>
inline void MemorySystem::Construct(U *ptr, Args &&...args)
{
    //new(start) U(std::forward<Args>(args)...);
}

MString MemorySystem::GetMemoryUsageStr()
{
    char buffer[8000] = "Использование системной памяти (с тегами):\n";
    u64 offset = MString::Length(buffer);
    for (u32 i = 0; i < Memory::MaxTags; ++i) {
        f32 amount = 1.F;
        const char* unit = GetUnitForSize(state->TaggedAllocations[i], amount);

        i32 length = snprintf(buffer + offset, 8000, "  %s: %.2f%s\n", MemoryTagStrings[i], amount, unit);
        offset += length;
    }
    {
        // Рассчет общего использования памяти
        u64 TotalSpace = state->allocator.TotalSpace();
        u64 FreeSpace = state->allocator.FreeSpace();
        u64 UsedSpace = TotalSpace - FreeSpace;

        f32 UsedAmount = 1.F;
        const char* UsedUnit = GetUnitForSize(UsedSpace, UsedAmount);

        f32 TotalAmount = 1.F;
        const char* TotalUnit = GetUnitForSize(TotalSpace, TotalAmount);

        f64 PercentUsed = (f64)(UsedSpace) / TotalSpace;

        i32 length = snprintf(buffer + offset, 8000, " Общее использование памяти: %.2f%s of %.2f%s (%.2f%%)\n", UsedAmount, UsedUnit, TotalAmount, TotalUnit, PercentUsed);
        offset += length;
    }
    
    return buffer;
}

const char* MemorySystem::GetUnitForSize(u64 SizeBytes, f32 &OutAmount)
{
    if (SizeBytes >= GIBIBYTES(1)) {
        OutAmount = (f64)SizeBytes / GIBIBYTES(1);
        return "GiB";
    } else if (SizeBytes >= MEBIBYTES(1)) {
        OutAmount = (f64)SizeBytes / MEBIBYTES(1);
        return "MiB";
    } else if (SizeBytes >= KIBIBYTES(1)) {
        OutAmount = (f64)SizeBytes / KIBIBYTES(1);
        return "KiB";
    } else {
        OutAmount = (f32)SizeBytes;
        return "B";
    }
}

/*void * MMemory::operator new(u64 size)
{
    return LinearAllocator::Instance().Allocate(size);
}*/