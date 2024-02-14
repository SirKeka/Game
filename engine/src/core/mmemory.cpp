#include "mmemory.hpp"

#include "core/logger.hpp"

// TODO: Custom string lib
#include <cstring>
#include <stdio.h>

static const char* MemoryTagStrings[MEMORY_TAG_MAX_TAGS] = {
    "UNKNOWN    ",
    "ARRAY      ",
    "DARRAY     ",
    "DICT       ",
    "RING_QUEUE ",
    "BST        ",
    "STRING     ",
    "APPLICATION",
    "JOB        ",
    "TEXTURE    ",
    "MAT_INST   ",
    "RENDERER   ",
    "GAME       ",
    "TRANSFORM  ",
    "ENTITY     ",
    "ENTITY_NODE",
    "SCENE      "};

u64 MMemory::TotalAllocated{0};
u64 MMemory::TaggedAllocations[MEMORY_TAG_MAX_TAGS]{};

MMemory::~MMemory()
{
}

void MMemory::ShutDown()
{
}

void *MMemory::Allocate(u64 bytes, MemoryTag tag)
{
    if (tag == MEMORY_TAG_UNKNOWN) {
        MWARN("allocate вызывается с использованием MEMORY_TAG_UNKNOWN. Переклассифицировать это распределение.");
    }

    TotalAllocated += bytes;
    TaggedAllocations[tag] += bytes;

    u8* ptrRawMem = new u8[bytes];
    
    // this->Start = ptrRawMem;
    return ptrRawMem;
}

void MMemory::Free(void *block, u64 bytes, MemoryTag tag)
{
    if (tag == MEMORY_TAG_UNKNOWN) {
        MWARN("free вызывается с использованием MEMORY_TAG_UNKNOWN. Переклассифицировать это распределение.");
    }

    TotalAllocated -= bytes;
    TaggedAllocations[tag] -= bytes;

if (block) {
    u8* ptrRawMem = reinterpret_cast<u8*>(block);
    delete[] ptrRawMem;
}

}

void MMemory::CopyMemory(void *dest, const void *source, u64 bytes)
{
    std::memmove(dest, source, bytes);
}

template<class U, class ...Args>
inline void MMemory::Construct(U * ptr, Args && ...args)
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
