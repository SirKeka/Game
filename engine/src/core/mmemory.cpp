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

MMemory::State* MMemory::state = nullptr;

MMemory::~MMemory()
{
}

bool MMemory::Initialize(u64 TotalAllocSize)
{
    // Сумма, необходимая состоянию системы.
    u64 StateMemoryRequirement = sizeof(State);

    // Выясните, сколько места нужно динамическому распределителю.
    u64 AllocRequirement = 0;
    state->allocator.GetMemoryRequirement(TotalAllocSize, AllocRequirement);

    // Вызовите распределитель платформы, чтобы получить память для всей системы, включая состояние.
    // СДЕЛАТЬ: выравнивание памяти
    u8* block = new u8[StateMemoryRequirement + AllocRequirement](); // platform_allocate(StateMemoryRequirement + AllocRequirement, false);
    if (!block) {
        MFATAL("Не удалось выделить память в системе, и система не может продолжить работу.");
        return false;
    }

    // Состояние находится в первой части массивного блока памяти.
    state = reinterpret_cast<State*>(block);
    state->TotalAllocSize = TotalAllocSize;
    state->AllocCount = 0;
    state->AllocatorMemoryRequirement = AllocRequirement;
    // Блок распределителя находится в том же блоке памяти, но после состояния.
    state->AllocatorBlock = reinterpret_cast<void*>(block + StateMemoryRequirement);

    if (!state->allocator.Create(
            TotalAllocSize,
            state->AllocatorMemoryRequirement,
            state->AllocatorBlock)) {
        MFATAL("Система памяти не может настроить внутренний распределитель. Работа приложения не может быть продолжена.");
        return false;
    }

    MDEBUG("Система памяти успешно выделила %llu байт.", TotalAllocSize);
    return true;
}

MINLINE void MMemory::Shutdown()
{
    //this->~MMemory();
}

void *MMemory::Allocate(u64 bytes, MemoryTag tag)
{
    if (tag == MemoryTag::Unknown) {
        MWARN("allocate вызывается с использованием MemoryTag::Unknown. Переклассифицировать это распределение.");
    }

    state->TotalAllocated += bytes;
    state->TaggedAllocations[static_cast<u32>(tag)] += bytes;
    state->AllocCount++;

    u8* ptrRawMem = new u8[bytes];
    
    return ptrRawMem;
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

        u8 *ptrRawMem = reinterpret_cast<u8 *>(block);
        delete[] ptrRawMem;
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

void * MMemory::operator new(u64 size)
{
    return LinearAllocator::Instance().Allocate(size);
}