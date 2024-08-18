#include "mmemory.hpp"

#include "platform/platform.hpp"
#include "core/application.hpp"

#include <cstring>
#include <stdio.h>

struct MemoryState 
{
    u64 TotalAllocSize{};                       // Общий размер памяти в байтах, используемый внутренним распределителем для этой системы.
    u64 TotalAllocated{};
    u64 TaggedAllocations[Memory::MaxTags]{};
    u64 AllocCount{};
    u64 AllocatorMemoryRequirement{};
    DynamicAllocator allocator{};
    void* AllocatorBlock{nullptr};
    MMutex AllocationMutex{};                   // Мьютекс для выделений/освобождений

    MemoryState(u64 TotalAllocSize, u64 AllocatorMemoryRequirement, void* AllocatorBlock) : 
    TotalAllocSize(TotalAllocSize), 
    TotalAllocated(), 
    TaggedAllocations(), 
    AllocCount(),
    AllocatorMemoryRequirement(AllocatorMemoryRequirement),
    allocator(),
    AllocatorBlock(AllocatorBlock),
    AllocationMutex() {}
};

static const char* MemoryTagStrings[Memory::MaxTags] = {
    "UNKNOWN    ",
    "ARRAY      ",
    "LINEAR ALLC",
    "DARRAY     ",
    "DICT       ",
    "RING QUEUE ",
    "BST        ",
    "STRING     ",
    "APPLICATION",
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
    "RESOURCE   "};

MemoryState* MMemory::state = nullptr;

MMemory::~MMemory()
{
    if (state) {
        state->allocator.Destroy();
        // Освободите целый блок.
        //platform_free(state_ptr, state_ptr->allocator_memory_requirement + sizeof(memory_system_state));
        delete[] state;
    }
}

bool MMemory::Initialize(u64 TotalAllocSize)
{
    // Сумма, необходимая состоянию системы.
    u64 StateMemoryRequirement = sizeof(MemoryState);

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
    state = new(block) MemoryState(
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

MINLINE void MMemory::Shutdown()
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

void *MMemory::Allocate(u64 bytes, Memory::Tag tag, bool nullify, bool def)
{
    if (tag == Memory::Unknown) {
        MWARN("allocate вызывается с использованием MemoryTag::Unknown. Переклассифицировать это распределение.");
    }

    // Либо выделяйте из системного распределителя, либо из ОС. Последнее никогда не должно произойти.
    u8* block = nullptr;

    if (state && !def) {
        // Убедитесь, что многопоточные запросы не мешают друг другу.
        if (!state->AllocationMutex.Lock()) {
            MFATAL("Ошибка получения блокировки мьютекса во время выделения.");
            return nullptr;
        }
        state->TotalAllocated += bytes;
        state->TaggedAllocations[tag] += bytes;
        state->AllocCount++;
        block = reinterpret_cast<u8*>(state->allocator.Allocate(bytes));
        state->AllocationMutex.Unlock();
    } else {
        if (!state) {
            // Если система еще не запустилась, предупредите об этом, но дайте пока память.
            MWARN("Memory::Allocate вызывается перед инициализацией системы памяти.");
        }
        // ЗАДАЧА: Memory alignment
        block = new u8[bytes]; //platform_allocate(size, false);
    }

    if (block) {
        if (nullify) {
            MMemory::ZeroMem(block, bytes);
        }
        return block;
    }
    
    MFATAL("MMemory::Allocate не удалось успешно распределить.");
    return nullptr;
}

void MMemory::Free(void *block, u64 bytes, Memory::Tag tag, bool def)
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
            state->TotalAllocated -= bytes;
            state->TaggedAllocations[tag] -= bytes;
            state->AllocCount--;
            bool result = state->allocator.Free(block, bytes);
            block = nullptr;
            // Если освобождение не удалось, возможно, это связано с тем, что выделение было выполнено до запуска этой системы. 
            // Поскольку это абсолютно должно быть исключением из правил, попробуйте освободить его на уровне платформы. 
            // Если это не удастся, значит, начнется какой-то другой вид мошенничества, и у нас возникнут более серьезные проблемы.
            /*if (!result) {
                // ЗАДАЧА: Выравнивание памяти
                u8* ptrRawMem = reinterpret_cast<u8*>(block);
                delete[] ptrRawMem;                             //platform_free(block, false);
            }*/
            state->AllocationMutex.Unlock();
        } else {
            u8* ptrRawMem = reinterpret_cast<u8*>(block);
            delete[] ptrRawMem;                             //platform_free(block, false);
        }
    }
}

void* MMemory::ZeroMem(void* block, u64 bytes)
{
    return memset(block, 0, bytes);
}

void MMemory::CopyMem(void *dest, const void *source, u64 bytes)
{
    std::memmove(dest, source, bytes);
}

void *MMemory::SetMemory(void *dest, i32 value, u64 bytes)
{
    return std::memset(dest, value, bytes);
}

u64 MMemory::GetMemoryAllocCount()
{
    return state->AllocCount;
}

template <class U, class... Args>
inline void MMemory::Construct(U *ptr, Args &&...args)
{
    new(start) U(std::forward<Args>(args)...);
}

char* MMemory::GetMemoryUsageStr()
{
    const u64 gib = 1024 * 1024 * 1024;
    const u64 mib = 1024 * 1024;
    const u64 kib = 1024;

    char buffer[8000] = "Использование системной памяти (с тегами):\n";
    u64 offset = MString::Length(buffer);
    for (u32 i = 0; i < Memory::MaxTags; ++i) {
        char unit[4] = "XiB";
        float amount = 1.0f;
        if (state->TaggedAllocations[i] >= gib) {
            unit[0] = 'G';
            amount = state->TaggedAllocations[i] / (float)gib;
        } else if (state->TaggedAllocations[i] >= mib) {
            unit[0] = 'M';
            amount = state->TaggedAllocations[i] / (float)mib;
        } else if (state->TaggedAllocations[i] >= kib) {
            unit[0] = 'K';
            amount = state->TaggedAllocations[i] / (float)kib;
        } else {
            unit[0] = 'B';
            unit[1] = 0;
            amount = (float)state->TaggedAllocations[i];
        }

        i32 length = snprintf(buffer + offset, 8000, "  %s: %.2f%s\n", MemoryTagStrings[i], amount, unit);
        offset += length;
    }
    char* Out = MString::Duplicate(buffer);
    return Out;
}

/*void * MMemory::operator new(u64 size)
{
    return LinearAllocator::Instance().Allocate(size);
}*/