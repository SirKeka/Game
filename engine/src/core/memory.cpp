#include "memory.hpp"

#include "core/logger.hpp"

// TODO: Custom string lib
#include <string.h>
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

Memory::Memory()
{
    this->Bytes = 0;
    this->TotalAllocated = 0;
    //this->Start = nullptr;
}

Memory::~Memory()
{
}

void Memory::ShutDown()
{
}

void *Memory::Allocate(u64 bytes, MemoryTag tag)
{
    if (tag == MEMORY_TAG_UNKNOWN) {
        MWARN("kallocate called using MEMORY_TAG_UNKNOWN. Re-class this allocation.");
    }

    TotalAllocated += bytes;
    TaggedAllocations[tag] += bytes;

    u8* ptrRawMem = new u8[bytes]();
    this->Start = ptrRawMem;
    return ptrRawMem;
}

void Memory::Free(void *block, u64 bytes, MemoryTag tag)
{
    if (tag == MEMORY_TAG_UNKNOWN) {
        MWARN("kfree called using MEMORY_TAG_UNKNOWN. Re-class this allocation.");
    }

    TotalAllocated -= bytes;
    TaggedAllocations[tag] -= bytes;

if (block)
{
// Convert to U8 pointer.
u8* pRawMem = reinterpret_cast<u8*>(block);

delete[] pRawMem;
}

}

const char *Memory::GetMemoryUsageStr()
{
    const u64 gib = 1024 * 1024 * 1024;
    const u64 mib = 1024 * 1024;
    const u64 kib = 1024;

    char buffer[8000] = "System memory use (tagged):\n";
    u64 offset = strlen(buffer);
    for (u32 i = 0; i < MEMORY_TAG_MAX_TAGS; ++i) {
        char unit[4] = "XiB";
        float amount = 1.0f;
        if (TaggedAllocations[i] >= gib) {
            unit[0] = 'G';
            amount = TaggedAllocations[i] / /*(float)*/gib;
        } else if (TaggedAllocations[i] >= mib) {
            unit[0] = 'M';
            amount = TaggedAllocations[i] / /*(float)*/mib;
        } else if (TaggedAllocations[i] >= kib) {
            unit[0] = 'K';
            amount = TaggedAllocations[i] / /*(float)*/kib;
        } else {
            unit[0] = 'B';
            unit[1] = 0;
            amount = /*(float)*/TaggedAllocations[i];
        }

        i32 length = snprintf(buffer + offset, 8000, "  %s: %.2f%s\n", MemoryTagStrings[i], amount, unit);
        offset += length;
    }
    const char* OutString = _strdup(buffer);
    return OutString;
}
