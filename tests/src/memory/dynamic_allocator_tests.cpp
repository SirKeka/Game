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
    bool result = alloc.GetMemoryRequirement(1024, MemoryRequirement);
    ExpectToBeTrue(result);

    // Собственно создайте распределитель.
    void* memory = MMemory::Allocate(MemoryRequirement, MemoryTag::Application);
    result = alloc.Create(memory);
    ExpectToBeTrue(result);
    ExpectShouldNotBe(0, (bool)alloc);
    u64 FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(1024, FreeSpace);

    // Уничтожить распределитель.
    alloc.Destroy();
    ExpectShouldBe(0, (bool)alloc);
    MMemory::Free(memory, MemoryRequirement, MemoryTag::Application);
    return true;
}

u8 DynamicAllocatorSingleAllocationAllSpace() {
    DynamicAllocator alloc;
    u64 MemoryRequirement = 0;
    // Получите требования к памяти
    bool result = alloc.GetMemoryRequirement(1024, MemoryRequirement);
    ExpectToBeTrue(result);

    // Собственно создайте распределитель.
    void* memory = MMemory::Allocate(MemoryRequirement, MemoryTag::Application);
    result = alloc.Create(memory);
    ExpectToBeTrue(result);
    ExpectShouldNotBe(0, (bool)alloc);
    u64 FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(1024, FreeSpace);

    // Выделите все подряд.
    void* block = alloc.Allocate(1024);
    ExpectShouldNotBe(nullptr, block);

    // Проверьте свободное место
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(0, FreeSpace);

    // Освободите распределение
    alloc.Free(block, 1024);

    // Проверьте свободное место.
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(1024, FreeSpace);

    // Уничтожить распределитель.
    alloc.Destroy();
    ExpectShouldBe(0, (bool)alloc);
    MMemory::Free(memory, MemoryRequirement, MemoryTag::Application);
    return true;
}

u8 DynamicAllocatorMultiAllocationAllSpace() {
    DynamicAllocator alloc;
    u64 MemoryRequirement = 0;
    // Получите требования к памяти
    bool result = alloc.GetMemoryRequirement(1024, MemoryRequirement);
    ExpectToBeTrue(result);

    // Собственно создайте распределитель.
    void* memory = MMemory::Allocate(MemoryRequirement, MemoryTag::Application);
    result = alloc.Create(memory);
    ExpectToBeTrue(result);
    ExpectShouldNotBe(0, (bool)alloc);
    u64 FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(1024, FreeSpace);

    // Выделить часть блока.
    void* block = alloc.Allocate(256);
    ExpectShouldNotBe(nullptr, block);

    // Проверьте свободное место
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(768, FreeSpace);

    // Выделите еще одну часть блока.
    void* block2 = alloc.Allocate(512);
    ExpectShouldNotBe(nullptr, block2);

    // Проверьте свободное место
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(256, FreeSpace);

    // Выделите последнюю часть блока.
    void* block3 = alloc.Allocate(256);
    ExpectShouldNotBe(nullptr, block3);

    // Проверьте свободное место
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(0, FreeSpace);

    // Освободите выделенные места, не по порядку, и проверьте свободное пространство.
    alloc.Free(block3, 256);

    // Проверьте свободное место.
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(256, FreeSpace);

    // Освободить следующее распределение, не по порядку
    alloc.Free(block, 256);

    // Проверьте свободное место.
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(512, FreeSpace);

    // Освободите окончательное распределение, не по порядку
    alloc.Free(block2, 512);

    // Проверьте свободное место.
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(1024, FreeSpace);

    // Уничтожить распределитель.
    alloc.Destroy();
    ExpectShouldBe(0, (bool)alloc);
    MMemory::Free(memory, MemoryRequirement, MemoryTag::Application);
    return true;
}

u8 DynamicAllocatorMultiAllocationOverAllocate() {
    DynamicAllocator alloc;
    u64 MemoryRequirement = 0;
    // Получите требования к памяти
    bool result = alloc.GetMemoryRequirement(1024, MemoryRequirement);
    ExpectToBeTrue(result);

    // Собственно создайте распределитель.
    void* memory = MMemory::Allocate(MemoryRequirement, MemoryTag::Application);
    result = alloc.Create(memory);
    ExpectToBeTrue(result);
    ExpectShouldNotBe(0, (bool)alloc);
    u64 FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(1024, FreeSpace);

    // Выделить часть блока.
    void* block = alloc.Allocate(256);
    ExpectShouldNotBe(nullptr, block);

    // Проверьте свободное место
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(768, FreeSpace);

    // Выделите еще одну часть блока.
    void* block2 = alloc.Allocate(512);
    ExpectShouldNotBe(nullptr, block2);

    // Проверьте свободное место
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(256, FreeSpace);

    // Выделите последнюю часть блока.
    void* block3 = alloc.Allocate(256);
    ExpectShouldNotBe(nullptr, block3);

    // Проверьте свободное место
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(0, FreeSpace);

    // Попытайтесь еще раз выделить, намеренно пытаясь переполниться.
    MDEBUG("Примечание. Следующие предупреждения и ошибки намеренно вызваны этим тестом.");
    void* FailBlock = alloc.Allocate(256);
    ExpectShouldBe(nullptr, FailBlock);

    // Проверьте свободное место.
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(0, FreeSpace);

    // Уничтожить распределитель.
    alloc.Destroy();
    ExpectShouldBe(0, (bool)alloc);
    MMemory::Free(memory, MemoryRequirement, MemoryTag::Application);
    return true;
}


u8 DynamicAllocatorMultiAllocationMostSpaceRequestTooBig() {
DynamicAllocator alloc;
    u64 MemoryRequirement = 0;
    // Получите требования к памяти
    bool result = alloc.GetMemoryRequirement(1024, MemoryRequirement);
    ExpectToBeTrue(result);

    // Собственно создайте распределитель.
    void* memory = MMemory::Allocate(MemoryRequirement, MemoryTag::Application);
    result = alloc.Create(memory);
    ExpectToBeTrue(result);
    ExpectShouldNotBe(0, (bool)alloc);
    u64 FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(1024, FreeSpace);

    // Выделить часть блока.
    void* block = alloc.Allocate(256);
    ExpectShouldNotBe(nullptr, block);

    // Проверьте свободное место
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(768, FreeSpace);

    // Выделите еще одну часть блока.
    void* block2 = alloc.Allocate(512);
    ExpectShouldNotBe(nullptr, block2);

    // Проверьте свободное место
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(256, FreeSpace);

    // Выделите последнюю часть блока.
    void* block3 = alloc.Allocate(128);
    ExpectShouldNotBe(nullptr, block3);

    // Проверьте свободное место
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(128, FreeSpace);

    // Попытайтесь еще раз выделить, намеренно пытаясь переполниться.
    MDEBUG("Примечание. Следующие предупреждения и ошибки намеренно вызваны этим тестом.");
    void* FailBlock = alloc.Allocate(256);
    ExpectShouldBe(nullptr, FailBlock);

    // Проверьте свободное место.
    FreeSpace = alloc.FreeSpace();
    ExpectShouldBe(128, FreeSpace);

    // Уничтожить распределитель.
    alloc.Destroy();
    ExpectShouldBe(0, (bool)alloc);
    MMemory::Free(memory, MemoryRequirement, MemoryTag::Application);
    return true;
}

void DynamicAllocatorRegisterTests()
{
    TestManagerRegisterTest(DynamicAllocatorShouldCreateAndDestroy, "Динамический распределитель должен создавать и уничтожать");
    TestManagerRegisterTest(DynamicAllocatorSingleAllocationAllSpace, "Динамический распределитель, единое распределение для всего пространства");
    TestManagerRegisterTest(DynamicAllocatorMultiAllocationAllSpace, "Многократное распределение динамического распределителя для всего пространства");
    TestManagerRegisterTest(DynamicAllocatorMultiAllocationOverAllocate, "Динамический распределитель пытается перераспределить");
    TestManagerRegisterTest(DynamicAllocatorMultiAllocationMostSpaceRequestTooBig, "Динамический распределитель должен попытаться выделить слишком много места, но при этом оставшееся пространство не равно 0.");
}