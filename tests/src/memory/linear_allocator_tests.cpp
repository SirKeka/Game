#include "linear_allocator_tests.hpp"

#include "../test_manager.hpp"
#include "../expect.hpp"

#include <defines.hpp>

#include <memory/linear_allocator.hpp>

u8 LinearAllocatorShouldCreateAndDestroy() {
    LinearAllocator alloc{ sizeof(u64) };

    ExpectShouldNotBe(0, alloc.memory);
    ExpectShouldBe(sizeof(u64), alloc.TotalSize);
    ExpectShouldBe(0, alloc.allocated);

    alloc.~LinearAllocator();

    ExpectShouldBe(0, alloc.memory);
    ExpectShouldBe(0, alloc.TotalSize);
    ExpectShouldBe(0, alloc.allocated);

    return true;
}

u8 LinearAllocatorSingleAllocationAllSpace() {
    LinearAllocator alloc{ sizeof(u64) };

    // Единое распределение.
    void* block = alloc.Allocate(sizeof(u64));

    // Подтвердите это
    ExpectShouldNotBe(0, block);
    ExpectShouldBe(sizeof(u64), alloc.allocated);

    alloc.~LinearAllocator();

    return true;
}

u8 LinearAllocatorMultiAllocationAllSpace() {
    u64 MaxAllocs = 1024;
    LinearAllocator alloc{sizeof(u64) * MaxAllocs, nullptr};

    // Множественное распределение - полное.
    void* block;
    for (u64 i = 0; i < MaxAllocs; ++i) {
        block = alloc.Allocate(sizeof(u64));
        // Подтвердите это
        ExpectShouldNotBe(0, block);
        ExpectShouldBe(sizeof(u64) * (i + 1), alloc.allocated);
    }

    alloc.~LinearAllocator();

    return true;
}

u8 LinearAllocatorMultiAllocationOverAllocate() {
    u64 MaxAllocs = 3;
    LinearAllocator alloc{sizeof(u64) * MaxAllocs, nullptr};

    // Множественное распределение - полное.
    void* block;
    for (u64 i = 0; i < MaxAllocs; ++i) {
        block = alloc.Allocate(sizeof(u64));
        // Подтвердите это
        ExpectShouldNotBe(0, block);
        ExpectShouldBe(sizeof(u64) * (i + 1), alloc.allocated);
    }

    MDEBUG("Примечание: Приведенная ниже ошибка намеренно вызвана этим тестом.");

    // Запросите еще одно распределение. Должно возникнуть сообщение об ошибке и вернуться значение 0.
    block = alloc.Allocate(sizeof(u64));
    // Проверьте это - выделенное должно быть неизменным.
    ExpectShouldBe(0, block);
    ExpectShouldBe(sizeof(u64) * (MaxAllocs), alloc.allocated);

    alloc.~LinearAllocator();

    return true;
}

u8 LinearAllocatorMultiAllocationAllSpaceThenFree() {
    u64 MaxAllocs = 1024;
    LinearAllocator alloc{ sizeof(u64) * MaxAllocs, nullptr };

    // Множественное распределение - полное.
    void* block;
    for (u64 i = 0; i < MaxAllocs; ++i) {
        block = alloc.Allocate(sizeof(u64));
        // Подтвердите это
        ExpectShouldNotBe(0, block);
        ExpectShouldBe(sizeof(u64) * (i + 1), alloc.allocated);
    }

    // Убедитесь, что указатель сброшен
    alloc.FreeAll();
    ExpectShouldBe(0, alloc.allocated);

    alloc.~LinearAllocator();

    return true;
}

void LinearAllocatorRegisterTests()
{
    TestManagerRegisterTest(LinearAllocatorShouldCreateAndDestroy, "Линейный распределитель должен создаваться и уничтожаться");
    TestManagerRegisterTest(LinearAllocatorSingleAllocationAllSpace, "Линейный распределитель, одиночное распределение для всего пространства");
    TestManagerRegisterTest(LinearAllocatorMultiAllocationAllSpace, "Linear allocator multi LinearAllocator::Instance() for all space");
    TestManagerRegisterTest(LinearAllocatorMultiAllocationOverAllocate, "Линейный распределитель попробуйте выделить");
    TestManagerRegisterTest(LinearAllocatorMultiAllocationAllSpaceThenFree, "Выделенный линейный распределитель должен быть равен 0 после FreeAll.");
}