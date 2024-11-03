#include "dynamic_allocator_tests.hpp"
#include "../test_manager.hpp"
#include "../expect.hpp"

#include <defines.hpp>

#include <core/mmemory.hpp>
#include <memory/dynamic_allocator.hpp>

u8 DynamicAllocatorShouldCreateAndDestroy() {
    DynamicAllocator alloc;
    u64 MemoryRequirement = 0;
    // Получите требования к памяти
    bool result = alloc.Create(1024, MemoryRequirement, nullptr);
    ExpectToBeTrue(result);

    // Фактически создайте распределитель.
    void* memory = MMemory::Allocate(MemoryRequirement, Memory::Engine);
    result = alloc.Create(1024, MemoryRequirement, memory);
    ExpectToBeTrue(result);
    // ExpectShouldNotBe(0, alloc.state);
    u64 FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(1024, FreeSpace);

    // Уничтожьте распределитель.
    alloc.Destroy();
    // ExpectShouldBe(0, alloc.state);
    MMemory::Free(memory, MemoryRequirement, Memory::Engine);
    return true;
}

u8 DynamicAllocatorSingleAllocationAllSpace() {
    DynamicAllocator alloc;
    u64 MemoryRequirement = 0;
    const u64 AllocatorSize = 1024;
    const u64 alignment = 1;
    // Общий необходимый размер, включая заголовки.
    const u64 TotalAllocatorSize = AllocatorSize + (alloc.HeaderSize() + alignment);
    // Получите требования к памяти
    bool result = alloc.Create(TotalAllocatorSize, MemoryRequirement, nullptr);
    ExpectToBeTrue(result);

    // Фактически создайте распределитель.
    void* memory = MMemory::Allocate(MemoryRequirement, Memory::Engine);
    result = alloc.Create(TotalAllocatorSize, MemoryRequirement, memory);
    ExpectToBeTrue(result);
    // ExpectShouldNotBe(0, alloc.state);
    u64 FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(TotalAllocatorSize, FreeSpace);

    // Выделите все это.
    void* block = alloc.Allocate(1024);
    ExpectShouldNotBe(0, block);

    // Проверьте свободное место
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(0, FreeSpace);

    // Освободить распределение
    alloc.Free(block, 1024);

    // Проверьте свободное место.
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(TotalAllocatorSize, FreeSpace);

    // Уничтожьте распределитель.
    alloc.Destroy();
    // ExpectShouldBe(0, alloc.state);
    MMemory::Free(memory, MemoryRequirement, Memory::Engine);
    return true;
}

u8 DynamicAllocatorMultiAllocationAllSpace() {
    DynamicAllocator alloc;
    u64 MemoryRequirement = 0;

    const u64 AllocatorSize = 1024;
    const u64 alignment = 1;
    u64 HeaderSize = (alloc.HeaderSize() + alignment);
    // Общий необходимый размер, включая заголовки.
    const u64 TotalAllocatorSize = AllocatorSize + (HeaderSize * 3);

    // Получите требования к памяти
    bool result = alloc.Create(TotalAllocatorSize, MemoryRequirement, nullptr);
    ExpectToBeTrue(result);

    // Фактически создайте распределитель.
    void* memory = MMemory::Allocate(MemoryRequirement, Memory::Engine);
    result = alloc.Create(TotalAllocatorSize, MemoryRequirement, memory);
    ExpectToBeTrue(result);
    // ExpectShouldNotBe(0, alloc.state);
    u64 FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(TotalAllocatorSize, FreeSpace);

    // Выделить часть блока.
    void* block = alloc.Allocate(256);
    ExpectShouldNotBe(0, block);

    // Проверьте свободное место
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(768 + (HeaderSize * 2), FreeSpace);

    // Выделите другую часть блока.
    void* block2 = alloc.Allocate(512);
    ExpectShouldNotBe(0, block2);

    // Проверьте свободное место
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(256 + (HeaderSize * 1), FreeSpace);

    // Выделите последнюю часть блока.
    void* block3 = alloc.Allocate(256);
    ExpectShouldNotBe(0, block3);

    // Проверьте свободное место
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(0, FreeSpace);

    // Освободите выделенные области, не по порядку, и проверьте свободное пространство
    alloc.Free(block3, 256);

    // Проверьте свободное место.
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(256 + (HeaderSize * 1), FreeSpace);

    // Освободите следующее распределение, вне очереди
    alloc.Free(block, 256);

    // Проверьте свободное место.
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(512 + (HeaderSize * 2), FreeSpace);

    // Освободите окончательное распределение, вне очереди
    alloc.Free(block2, 512);

    // Проверьте свободное место.
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(TotalAllocatorSize, FreeSpace);

    // Уничтожьте распределитель.
    alloc.Destroy();
    // ExpectShouldBe(0, alloc.state);
    MMemory::Free(memory, MemoryRequirement, Memory::Engine);
    return true;
}

u8 DynamicAllocatorMultiAllocationOverAllocate() {
    DynamicAllocator alloc;
    u64 MemoryRequirement = 0;

    const u64 AllocatorSize = 1024;
    const u64 alignment = 1;
    u64 HeaderSize = (alloc.HeaderSize() + alignment);
    // Общий необходимый размер, включая заголовки.
    const u64 TotalAllocatorSize = AllocatorSize + (HeaderSize * 3);

    // Получите требования к памяти
    bool result = alloc.Create(TotalAllocatorSize, MemoryRequirement, nullptr);
    ExpectToBeTrue(result);

    // Фактически создайте распределитель.
    void* memory = MMemory::Allocate(MemoryRequirement, Memory::Engine);
    result = alloc.Create(TotalAllocatorSize, MemoryRequirement, memory);
    ExpectToBeTrue(result);
    // ExpectShouldNotBe(0, alloc.state);
    u64 FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(TotalAllocatorSize, FreeSpace);

    // Выделить часть блока.
    void* block = alloc.Allocate(256);
    ExpectShouldNotBe(0, block);

    // Проверьте свободное место
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(768 + (HeaderSize * 2), FreeSpace);

    // Выделите другую часть блока.
    void* block2 = alloc.Allocate(512);
    ExpectShouldNotBe(0, block2);

    // Проверьте свободное место
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(256 + (HeaderSize * 1), FreeSpace);

    // Выделите последнюю часть блока.
    void* block3 = alloc.Allocate(256);
    ExpectShouldNotBe(0, block3);

    // Проверьте свободное место
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(0, FreeSpace);

    // Попытайтесь еще раз выделить ресурсы, намеренно пытаясь добиться переполнения.
    MDEBUG("Примечание: следующие предупреждения и ошибки намеренно вызваны этим тестом.");
    void* FailBlock = alloc.Allocate(256);
    ExpectShouldBe(0, FailBlock);

    // Проверьте свободное место.
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(0, FreeSpace);

    // Уничтожьте распределитель.
    alloc.Destroy();
    // ExpectShouldBe(0, alloc.state);
    MMemory::Free(memory, MemoryRequirement, Memory::Engine);
    return true;
}

u8 DynamicAllocatorMultiAllocationMostSpaceRequestTooBig() {
    DynamicAllocator alloc;
    u64 MemoryRequirement = 0;

    const u64 AllocatorSize = 1024;
    const u64 alignment = 1;
    u64 HeaderSize = (alloc.HeaderSize() + alignment);
    // Общий необходимый размер, включая заголовки.
    const u64 TotalAllocatorSize = AllocatorSize + (HeaderSize * 3);

    // Получите требования к памяти
    bool result = alloc.Create(TotalAllocatorSize, MemoryRequirement, nullptr);
    ExpectToBeTrue(result);

    // Фактически создайте распределитель.
    void* memory = MMemory::Allocate(MemoryRequirement, Memory::Engine);
    result = alloc.Create(TotalAllocatorSize, MemoryRequirement, memory);
    ExpectToBeTrue(result);
    //ExpectShouldNotBe(0, alloc.state);
    u64 FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(TotalAllocatorSize, FreeSpace);

    // Выделить часть блока.
    void* block = alloc.Allocate(256);
    ExpectShouldNotBe(0, block);

    // Проверьте свободное место
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(768 + (HeaderSize * 2), FreeSpace);

    // Выделите другую часть блока.
    void* block2 = alloc.Allocate(512);
    ExpectShouldNotBe(0, block2);

    // Проверьте свободное место
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(256 + (HeaderSize * 1), FreeSpace);

    // Выделите последнюю часть блока.
    void* block3 = alloc.Allocate(128);
    ExpectShouldNotBe(0, block3);

    // Проверьте свободное место
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(128 + (HeaderSize * 0), FreeSpace);

    // Попытайтесь еще раз выделить ресурсы, намеренно пытаясь добиться переполнения.
    MDEBUG("Примечание: следующие предупреждения и ошибки намеренно вызваны этим тестом.");
    void* FailBlock = alloc.Allocate(256);
    ExpectShouldBe(0, FailBlock);

    // Проверьте свободное место. Should not have changed.
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(128 + (HeaderSize * 0), FreeSpace);

    // Уничтожьте распределитель.
    alloc.Destroy();
    // ExpectShouldBe(0, alloc.state);
    MMemory::Free(memory, MemoryRequirement, Memory::Engine);
    return true;
}

u8 DynamicAllocatorSingleAllocAligned() {
    DynamicAllocator alloc;
    u64 MemoryRequirement = 0;
    const u64 AllocatorSize = 1024;
    const u64 alignment = 16;
    // Общий необходимый размер, включая заголовки.
    const u64 TotalAllocatorSize = AllocatorSize + (alloc.HeaderSize() + alignment);
    // Получите требования к памяти
    bool result = alloc.Create(TotalAllocatorSize, MemoryRequirement, nullptr);
    ExpectToBeTrue(result);

    // Фактически создайте распределитель.
    void* memory = MMemory::Allocate(MemoryRequirement, Memory::Engine);
    result = alloc.Create(TotalAllocatorSize, MemoryRequirement, memory);
    ExpectToBeTrue(result);
    // ExpectShouldNotBe(0, alloc.state);
    u64 FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(TotalAllocatorSize, FreeSpace);

    // Выделите все это.
    void* block = alloc.AllocateAligned(1024, alignment);
    ExpectShouldNotBe(0, block);

    // Проверьте размер и выравнивание
    u64 BlockSize;
    u16 BlockAlignment;
    result = DynamicAllocator::GetSizeAlignment(block, BlockSize, BlockAlignment);
    ExpectToBeTrue(result);
    ExpectShouldBe(alignment, BlockAlignment);
    ExpectShouldBe(1024, BlockSize);

    // Проверьте свободное место
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(0, FreeSpace);

    // Освободить распределение
    alloc.Free(block, 0, true);

    // Проверьте свободное место.
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(TotalAllocatorSize, FreeSpace);

    // Уничтожьте распределитель.
    alloc.Destroy();
    // ExpectShouldBe(0, alloc.state);
    MMemory::Free(memory, MemoryRequirement, Memory::Engine);
    return true;
}

struct AllocData {
    void* block;
    u16 alignment;
    u64 size;
};

u8 DynamicAllocatorMultipleAllocAlignedDifferentAlignments() {
    DynamicAllocator alloc;
    u64 MemoryRequirement = 0;
    u64 CurrentlyAllocated = 0;
    const u64 HeaderSize = alloc.HeaderSize();

    const u32 AllocDataCount = 4;
    AllocData AllocDatas[4];
    AllocDatas[0] = (AllocData){0, 1, 31};   // 1-byte alignment.
    AllocDatas[1] = (AllocData){0, 16, 82};  // 16-byte alignment.
    AllocDatas[2] = (AllocData){0, 1, 59};   // 1-byte alignment.
    AllocDatas[3] = (AllocData){0, 8, 73};   // 1-byte alignment.
    // Общий необходимый размер, включая заголовки.
    u64 TotalAllocatorSize = 0;
    for (u32 i = 0; i < AllocDataCount; ++i) {
        TotalAllocatorSize += AllocDatas[i].alignment + HeaderSize + AllocDatas[i].size;
    }

    // Получите требования к памяти
    bool result = alloc.Create(TotalAllocatorSize, MemoryRequirement, nullptr);
    ExpectToBeTrue(result);

    // Фактически создайте распределитель.
    void* memory = MMemory::Allocate(MemoryRequirement, Memory::Engine);
    result = alloc.Create(TotalAllocatorSize, MemoryRequirement, memory);
    ExpectToBeTrue(result);
    // ExpectShouldNotBe(0, alloc.state);
    u64 FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(TotalAllocatorSize, FreeSpace);

    // Выделяем первое.
    {
        u32 idx = 0;
        AllocDatas[idx].block = alloc.AllocateAligned(AllocDatas[idx].size, AllocDatas[idx].alignment);
        ExpectShouldNotBe(0, AllocDatas[idx].block);

        // Проверьте размер и выравнивание
        u64 BlockSize;
        u16 BlockAlignment;
        result = DynamicAllocator::GetSizeAlignment(AllocDatas[idx].block, BlockSize, BlockAlignment);
        ExpectToBeTrue(result);
        ExpectShouldBe(AllocDatas[idx].alignment, BlockAlignment);
        ExpectShouldBe(AllocDatas[idx].size, BlockSize);

        // Отслеживайте.
        CurrentlyAllocated += (AllocDatas[idx].size + HeaderSize + AllocDatas[idx].alignment);

        // Проверьте свободное место
        FreeSpace = alloc.FreeSpace();
        ExpectShouldBe(TotalAllocatorSize - CurrentlyAllocated, FreeSpace);
    }

    // Выделяем второе.
    {
        u32 idx = 1;
        AllocDatas[idx].block = alloc.AllocateAligned(AllocDatas[idx].size, AllocDatas[idx].alignment);
        ExpectShouldNotBe(0, AllocDatas[idx].block);

        // Проверьте размер и выравнивание
        u64 BlockSize;
        u16 BlockAlignment;
        result = DynamicAllocator::GetSizeAlignment(AllocDatas[idx].block, BlockSize, BlockAlignment);
        ExpectToBeTrue(result);
        ExpectShouldBe(AllocDatas[idx].alignment, BlockAlignment);
        ExpectShouldBe(AllocDatas[idx].size, BlockSize);

        // Отслеживайте.
        CurrentlyAllocated += (AllocDatas[idx].size + HeaderSize + AllocDatas[idx].alignment);

        // Проверьте свободное место
        FreeSpace = alloc.FreeSpace();
        ExpectShouldBe(TotalAllocatorSize - CurrentlyAllocated, FreeSpace);
    }

    // Выделяем третью.
    {
        u32 idx = 2;
        AllocDatas[idx].block = alloc.AllocateAligned(AllocDatas[idx].size, AllocDatas[idx].alignment);
        ExpectShouldNotBe(0, AllocDatas[idx].block);

        // Проверьте размер и выравнивание
        u64 BlockSize;
        u16 BlockAlignment;
        result = DynamicAllocator::GetSizeAlignment(AllocDatas[idx].block, BlockSize, BlockAlignment);
        ExpectToBeTrue(result);
        ExpectShouldBe(AllocDatas[idx].alignment, BlockAlignment);
        ExpectShouldBe(AllocDatas[idx].size, BlockSize);

        // Отслеживайте.
        CurrentlyAllocated += (AllocDatas[idx].size + HeaderSize + AllocDatas[idx].alignment);

        // Проверьте свободное место
        FreeSpace = alloc.FreeSpace();
        ExpectShouldBe(TotalAllocatorSize - CurrentlyAllocated, FreeSpace);
    }

    // Выделяем четвертого.
    {
        u32 idx = 3;
        AllocDatas[idx].block = alloc.AllocateAligned(AllocDatas[idx].size, AllocDatas[idx].alignment);
        ExpectShouldNotBe(0, AllocDatas[idx].block);

        // Проверьте размер и выравнивание
        u64 BlockSize;
        u16 BlockAlignment;
        result = DynamicAllocator::GetSizeAlignment(AllocDatas[idx].block, BlockSize, BlockAlignment);
        ExpectToBeTrue(result);
        ExpectShouldBe(AllocDatas[idx].alignment, BlockAlignment);
        ExpectShouldBe(AllocDatas[idx].size, BlockSize);

        // Отслеживайте.
        CurrentlyAllocated += (AllocDatas[idx].size + HeaderSize + AllocDatas[idx].alignment);

        // Проверьте свободное место
        FreeSpace = alloc.FreeSpace();
        ExpectShouldBe(TotalAllocatorSize - CurrentlyAllocated, FreeSpace);
    }

    // Освободите выделения вне очереди.

    // Освободите второе выделение
    {
        u32 idx = 1;
        alloc.Free(AllocDatas[idx].block, 0, true);
        AllocDatas[idx].block = 0;
        CurrentlyAllocated -= (AllocDatas[idx].size + HeaderSize + AllocDatas[idx].alignment);

        // Проверьте свободное место.
        FreeSpace = alloc.FreeSpace();
        ExpectShouldBe(TotalAllocatorSize - CurrentlyAllocated, FreeSpace);
    }
    // Освободить четвертое распределение
    {
        u32 idx = 3;
        alloc.Free(AllocDatas[idx].block, 0, true);
        AllocDatas[idx].block = 0;
        CurrentlyAllocated -= (AllocDatas[idx].size + HeaderSize + AllocDatas[idx].alignment);

        // Проверьте свободное место.
        FreeSpace = alloc.FreeSpace();
        ExpectShouldBe(TotalAllocatorSize - CurrentlyAllocated, FreeSpace);
    }

    // Освободить третье распределение
    {
        u32 idx = 2;
        alloc.Free(AllocDatas[idx].block, 0, true);
        AllocDatas[idx].block = 0;
        CurrentlyAllocated -= (AllocDatas[idx].size + HeaderSize + AllocDatas[idx].alignment);

        // Проверьте свободное место.
        FreeSpace = alloc.FreeSpace();
        ExpectShouldBe(TotalAllocatorSize - CurrentlyAllocated, FreeSpace);
    }

    // Освободите первое распределение
    {
        u32 idx = 0;
        alloc.Free(AllocDatas[idx].block, 0, true);
        AllocDatas[idx].block = 0;
        CurrentlyAllocated -= (AllocDatas[idx].size + HeaderSize + AllocDatas[idx].alignment);

        // Проверьте свободное место.
        FreeSpace = alloc.FreeSpace();
        ExpectShouldBe(TotalAllocatorSize - CurrentlyAllocated, FreeSpace);
    }

    // Уничтожьте распределитель.
    alloc.Destroy();
    // ExpectShouldBe(0, alloc.state);
    MMemory::Free(memory, MemoryRequirement, Memory::Engine);
    return true;
}

u8 UtilAllocate(DynamicAllocator& allocator, AllocData& data, u64& CurrentlyAllocated, u64 HeaderSize, u64 TotalAllocatorSize) {
    data.block = allocator.AllocateAligned(data.size, data.alignment);
    ExpectShouldNotBe(0, data.block);

    // Проверьте размер и выравнивание
    u64 BlockSize;
    u16 BlockAlignment;
    bool result = DynamicAllocator::GetSizeAlignment(data.block, BlockSize, BlockAlignment);
    ExpectToBeTrue(result);
    ExpectShouldBe(data.alignment, BlockAlignment);
    ExpectShouldBe(data.size, BlockSize);

    // Отслеживайте.
    CurrentlyAllocated += (data.size + HeaderSize + data.alignment);

    // Проверьте свободное место
    u64 FreeSpace = allocator.FreeSpace();
    ExpectShouldBe(TotalAllocatorSize - CurrentlyAllocated, FreeSpace);

    return true;
}

u8 UtilFree(DynamicAllocator& allocator, AllocData& data, u64& CurrentlyAllocated, u64 HeaderSize, u64 TotalAllocatorSize) {
    if (!allocator.Free(data.block, 0, true)) {
        MERROR("UtilFree, DynamicAllocator::Free failed");
        return false;
    }
    data.block = 0;
    CurrentlyAllocated -= (data.size + HeaderSize + data.alignment);

    // Проверьте свободное место.
    u64 FreeSpace = allocator.FreeSpace();
    ExpectShouldBe(TotalAllocatorSize - CurrentlyAllocated, FreeSpace);

    return true;
}

u8 DynamicAllocatorMultipleAllocAlignedDifferentAlignmentsRandom() {
    DynamicAllocator alloc;
    u64 MemoryRequirement = 0;
    u64 CurrentlyAllocated = 0;
    const u64 HeaderSize = alloc.HeaderSize();

    const u32 AllocDataCount = 65556;
    AllocData AllocDatas[65556] = {0};
    u16 po2[8] = {1, 2, 4, 8, 16, 32, 64, 128};

    // Выбирайте случайные размеры и выравнивания.
    for (u32 i = 0; i < AllocDataCount; ++i) {
        AllocDatas[i].alignment = po2[Math::RandomInRange(0, 7)]; 
        AllocDatas[i].size = (u64)Math::RandomInRange(1, 65536);
    }

    // Общий необходимый размер, включая заголовки.
    u64 TotalAllocatorSize = 0;
    for (u32 i = 0; i < AllocDataCount; ++i) {
        TotalAllocatorSize += AllocDatas[i].alignment + HeaderSize + AllocDatas[i].size;
    }

    // Получите требования к памяти
    bool result = alloc.Create(TotalAllocatorSize, MemoryRequirement, nullptr);
    ExpectToBeTrue(result);

    // Фактически создайте распределитель.
    void* memory = MMemory::Allocate(MemoryRequirement, Memory::Engine);
    result = alloc.Create(TotalAllocatorSize, MemoryRequirement, memory);
    ExpectToBeTrue(result);
    // ExpectShouldNotBe(0, alloc.state);
    u64 FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(TotalAllocatorSize, FreeSpace);

    // Распределяйте случайным образом, пока все не будут распределены.
    u32 AllocCount = 0;
    while (AllocCount != AllocDataCount) {
        u32 index = (u32)Math::RandomInRange(0, AllocDataCount);
        if (AllocDatas[index].block == 0) {
            if (!UtilAllocate(alloc, AllocDatas[index], CurrentlyAllocated, HeaderSize, TotalAllocatorSize)) {
                MERROR("Ошибка UtilAllocate в индексе: %u.");
                return false;
            }
            AllocCount++;
        }
    }

    MTRACE("Случайно выделение %u раз. Освобождается случайным образом...", AllocCount);

    // Если все они будут распределены на этом этапе, освободите их случайным образом.
    while (AllocCount != AllocDataCount) {
        u32 index = (u32)Math::RandomInRange(0, AllocDataCount);
        if (AllocDatas[index].block != 0) {
            if (!UtilFree(alloc, AllocDatas[index], CurrentlyAllocated, HeaderSize, TotalAllocatorSize)) {
                MERROR("Ошибка UtilFree в индексе: %u.");
                return false;
            }
            AllocCount--;
        }
    }

    // Уничтожьте распределитель.
    alloc.Destroy();
    // ExpectShouldBe(0, alloc.state);
    MMemory::Free(memory, MemoryRequirement, Memory::Engine);
    return true;
}

u8 DynamicAllocatorMultipleAllocAndFreeAlignedDifferentAlignmentsRandom() {
    DynamicAllocator alloc;
    u64 MemoryRequirement = 0;
    u64 CurrentlyAllocated = 0;
    const u64 HeaderSize = alloc.HeaderSize();

    const u32 AllocDataCount = 65556;
    AllocData AllocDatas[65556] = {0};
    u16 po2[8] = {1, 2, 4, 8, 16, 32, 64, 128};

    // Выбирайте случайные размеры и выравнивания.
    for (u32 i = 0; i < AllocDataCount; ++i) {
        AllocDatas[i].alignment = po2[Math::RandomInRange(0, 7)];
        AllocDatas[i].size = (u64)Math::RandomInRange(1, 65536);
    }

    // Общий необходимый размер, включая заголовки.
    u64 TotalAllocatorSize = 0;
    for (u32 i = 0; i < AllocDataCount; ++i) {
        TotalAllocatorSize += AllocDatas[i].alignment + HeaderSize + AllocDatas[i].size;
    }

    // Получите требования к памяти
    bool result = alloc.Create(TotalAllocatorSize, MemoryRequirement, nullptr);
    ExpectToBeTrue(result);

    // Фактически создайте распределитель.
    void* memory = MMemory::Allocate(MemoryRequirement, Memory::Engine);
    result = alloc.Create(TotalAllocatorSize, MemoryRequirement, memory);
    ExpectToBeTrue(result);
    // ExpectShouldNotBe(0, alloc.state);
    u64 FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(TotalAllocatorSize, FreeSpace);

    u32 OpCount = 0;
    const u32 MaxOpCount = 10000000;
    u32 AllocCount = 0;
    while (OpCount < MaxOpCount) {
        // If there are no allocations, or we "roll" high, allocate. Otherwise deallocate.
        if (AllocCount == 0 || Math::RandomInRange(0, 99) > 50) {
            while (true) {
                u32 index = (u32)Math::RandomInRange(0, AllocDataCount - 1);
                if (AllocDatas[index].block == 0) {
                    if (!UtilAllocate(alloc, AllocDatas[index], CurrentlyAllocated, HeaderSize, TotalAllocatorSize)) {
                        MERROR("Ошибка UtilAllocate для индекса: %u.", index);
                        return false;
                    }
                    AllocCount++;
                    break;
                }
            }
            OpCount++;
        } else {
            while (true) {
                u32 index = (u32)Math::RandomInRange(0, AllocDataCount - 1);
                if (AllocDatas[index].block != 0) {
                    if (!UtilFree(alloc, AllocDatas[index], CurrentlyAllocated, HeaderSize, TotalAllocatorSize)) {
                        MERROR("Ошибка UtilFree при индексе: %u.", index);
                        return false;
                    }
                    AllocCount--;
                    break;
                }
            }
            OpCount++;
        }
    }

    MTRACE("Достигнуто максимальное количество операций %u. Освобождение оставшихся выделений.", MaxOpCount);
    for (u32 i = 0; i < AllocDataCount; ++i) {
        if (AllocDatas[i].block != 0) {
            if (!UtilFree(alloc, AllocDatas[i], CurrentlyAllocated, HeaderSize, TotalAllocatorSize)) {
                MERROR("UtilFree не удалось выполнить индекс: %u.");
                return false;
            }
        }
    }

    // Уничтожьте распределитель.
    alloc.Destroy();
    // ExpectShouldBe(0, alloc.state);
    MMemory::Free(memory, MemoryRequirement, Memory::Engine);
    return true;
}

void DynamicAllocatorRegisterTests() {
    TestManagerRegisterTest(DynamicAllocatorShouldCreateAndDestroy, "Динамический распределитель должен создавать и уничтожать");
    TestManagerRegisterTest(DynamicAllocatorSingleAllocationAllSpace, "Единичное выделение динамического распределителя для всего пространства");
    TestManagerRegisterTest(DynamicAllocatorMultiAllocationAllSpace, "Многократное выделение динамического распределителя для всего пространства");
    TestManagerRegisterTest(DynamicAllocatorMultiAllocationOverAllocate, "Попытка перераспределения динамического распределителя");
    TestManagerRegisterTest(DynamicAllocatorMultiAllocationMostSpaceRequestTooBig, "Динамический распределитель должен попытаться перераспределить при недостаточном количестве места, но не при 0 оставшемся пространстве.");
    TestManagerRegisterTest(DynamicAllocatorSingleAllocAligned, "Единичное выровненное выделение динамического распределителя");
    TestManagerRegisterTest(DynamicAllocatorMultipleAllocAlignedDifferentAlignments, "Множественные выровненные выделения динамического распределителя с различными выравниваниями");
    TestManagerRegisterTest(DynamicAllocatorMultipleAllocAlignedDifferentAlignmentsRandom, "Множественные выровненные выделения динамического распределителя с различными выравниваниями в случайном порядке.");
    TestManagerRegisterTest(DynamicAllocatorMultipleAllocAndFreeAlignedDifferentAlignmentsRandom, "Тест рандомизации динамического распределителя.");
}
