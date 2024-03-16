#include "mmemory.hpp"

#include "platform/platform.hpp"

#include <cstring>
#include <stdio.h>

static const char* MemoryTagStrings[MEMORY_TAG_MAX_TAGS] = {
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

u64 MMemory::TotalAllocated;
u64 MMemory::TaggedAllocations[MEMORY_TAG_MAX_TAGS];
u64 MMemory::AllocCount;

MMemory::~MMemory()
{
}

MINLINE void MMemory::Shutdown()
{
    this->~MMemory();
}

void *MMemory::Allocate(u64 bytes, MemoryTag tag)
{
    if (tag == MEMORY_TAG_UNKNOWN) {
        MWARN("allocate вызывается с использованием MEMORY_TAG_UNKNOWN. Переклассифицировать это распределение.");
    }

    TotalAllocated += bytes;
    TaggedAllocations[tag] += bytes;
    AllocCount++;

    u8* ptrRawMem = new u8[bytes];
    
    return ptrRawMem;
}

void MMemory::Free(void *block, u64 bytes, MemoryTag tag)
{
    if (block) {
        if (tag == MEMORY_TAG_UNKNOWN) {
            MWARN("free вызывается с использованием MEMORY_TAG_UNKNOWN. Переклассифицировать это распределение.");
        }

        TotalAllocated -= bytes;
        TaggedAllocations[tag] -= bytes;
        AllocCount--;

        u8* ptrRawMem = reinterpret_cast<u8*>(block);
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
    return AllocCount;
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
    for (u32 i = 0; i < MEMORY_TAG_MAX_TAGS; ++i) {
        char unit[4] = "XiB";
        float amount = 1.0f;
        if (TaggedAllocations[i] >= gib) {
            unit[0] = 'G';
            amount = TaggedAllocations[i] / (float)gib;
        } else if (TaggedAllocations[i] >= mib) {
            unit[0] = 'M';
            amount = TaggedAllocations[i] / (float)mib;
        } else if (TaggedAllocations[i] >= kib) {
            unit[0] = 'K';
            amount = TaggedAllocations[i] / (float)kib;
        } else {
            unit[0] = 'B';
            unit[1] = 0;
            amount = (float)TaggedAllocations[i];
        }

        i32 length = snprintf(buffer + offset, 8000, "  %s: %.2f%s\n", MemoryTagStrings[i], amount, unit);
        offset += length;
    }
    MString Out = buffer;
    return Out;
}

void * MMemory::operator new(u64 size)
{
    return LinearAllocator::Allocate(size);
}