#include "mmemory.hpp"

#include "platform/platform.hpp"
#include "core/application.hpp"

#include <cstring>
#include <stdio.h>

static const char* MemoryTagStrings[static_cast<u32>(MemoryTag::MaxTags)] = {
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
    "SCENE      "};

MMemory::MemoryState* MMemory::state = nullptr;

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
    // СДЕЛАТЬ: выравнивание памяти
    u8* block = new u8[StateMemoryRequirement + AllocRequirement](); // platform_allocate(StateMemoryRequirement + AllocRequirement, false);
    if (!block) {
        MFATAL("Не удалось выделить память в системе, и система не может продолжить работу.");
        return false;
    }

    // Состояние находится в первой части массивного блока памяти.
    state = reinterpret_cast<MemoryState*>(block);
    state->TotalAllocSize = TotalAllocSize;
    //state->AllocCount = 0;
    state->AllocatorMemoryRequirement = AllocRequirement;
    // Блок распределителя находится в том же блоке памяти, но после состояния.
    state->AllocatorBlock = reinterpret_cast<void*>(block + StateMemoryRequirement);

    if (!state->allocator.Create(TotalAllocSize, state->AllocatorBlock)) {
        MFATAL("Система памяти не может настроить внутренний распределитель. Работа приложения не может быть продолжена.");
        return false;
    }

    MDEBUG("Система памяти успешно выделила %llu байт.", TotalAllocSize);
    return true;
}

MINLINE void MMemory::Shutdown()
{
    if (state) {
        state->allocator.Destroy();
        // Освободите целый блок.
        //platform_free(state_ptr, state_ptr->allocator_memory_requirement + sizeof(memory_system_state));
        delete[] state;
    }
}

void *MMemory::Allocate(u64 bytes, MemoryTag tag)
{
    if (tag == MemoryTag::Unknown) {
        MWARN("allocate вызывается с использованием MemoryTag::Unknown. Переклассифицировать это распределение.");
    }

    // Либо выделяйте из системного распределителя, либо из ОС. Последнее никогда не должно произойти.
    u8* block = nullptr;

    if (state) {
        state->TotalAllocated += bytes;
        state->TaggedAllocations[static_cast<u32>(tag)] += bytes;
        state->AllocCount++;
        block = reinterpret_cast<u8*>(state->allocator.Allocate(bytes));
    } else {
        // Если система еще не запустилась, предупредите об этом, но дайте пока память.
        MWARN("Memory::Allocate вызывается перед инициализацией системы памяти.");
        // СДЕЛАТЬ: Memory alignment
        block = new u8[bytes](); //platform_allocate(size, false);
    }

    if (block) {
        // MMemory::ZeroMem(block, bytes);
        return block;
    }
    
    MFATAL("MMemory::Allocate не удалось успешно распределить.");
    return nullptr;
}

void MMemory::Free(void *block, u64 bytes, MemoryTag tag)
{
    if (block) {
        if (tag == MemoryTag::Unknown) {
            MWARN("free вызывается с использованием MemoryTag::Unknown. Переклассифицировать это распределение.");
        }

        state->TotalAllocated -= bytes;
        state->TaggedAllocations[static_cast<u32>(tag)] -= bytes;
        state->AllocCount--;
        bool result = state->allocator.Free(block, bytes);

        // Если освобождение не удалось, возможно, это связано с тем, что выделение было выполнено до запуска этой системы. 
        // Поскольку это абсолютно должно быть исключением из правил, попробуйте освободить его на уровне платформы. 
        // Если это не удастся, значит, начнется какой-то другой вид мошенничества, и у нас возникнут более серьезные проблемы.
        if (!result) {
            // СДЕЛАТЬ: Выравнивание памяти
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

MString MMemory::GetMemoryUsageStr()
{
    const u64 gib = 1024 * 1024 * 1024;
    const u64 mib = 1024 * 1024;
    const u64 kib = 1024;

    char buffer[8000] = "Использование системной памяти (с тегами):\n";
    u64 offset = strlen(buffer);
    for (u32 i = 0; i < static_cast<u32>(MemoryTag::MaxTags); ++i) {
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
    MString Out = buffer;
    return Out;
}

/*void * MMemory::operator new(u64 size)
{
    return LinearAllocator::Instance().Allocate(size);
}*/