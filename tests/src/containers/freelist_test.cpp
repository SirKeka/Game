#include "freelist_test.hpp"
#include "../test_manager.hpp"
#include "../expect.hpp"

#include <containers/freelist.hpp>
#include <core/mmemory.hpp>

u8 FreelistShouldCreateAndDestroy() {
    // ПРИМЕЧАНИЕ: Создание списка небольшого размера, который вызовет предупреждение.
    MDEBUG("Следующее предупреждающее сообщение является преднамеренным.");

    FreeList list;

    // Получение требуемой памяти
    u64 MemoryRequirement = 0;
    u64 TotalSize = 40;
    list.GetMemoryRequirement(TotalSize, MemoryRequirement);

    // Выделите и создайте FreeList.
    void* block = MMemory::Allocate(MemoryRequirement, MemoryTag::Application);
    list.Create(TotalSize, block);

    // Убедитесь, что память назначена.
    ExpectShouldNotBe(0, (bool)list);
    // Убедитесь, что весь блок свободен.
    u64 FreeSpace = list.FreeSpace();
    ExpectShouldBe(TotalSize, FreeSpace);

    // Уничтожьте и убедитесь, что память не назначена.
    list.~FreeList();
    ExpectShouldBe(0, (bool)list);
    MMemory::Free(block, MemoryRequirement, MemoryTag::Application);

    return true;
}

u8 FreelistShouldAllocateOneAndFreeOne() {
    FreeList list;

    // Получение требуемой памяти
    u64 MemoryRequirement = 0;
    u64 TotalSize = 512;
    list.GetMemoryRequirement(TotalSize, MemoryRequirement);

    // Выделите и создайте FreeList.
    void* block = MMemory::Allocate(MemoryRequirement, MemoryTag::Application);
    list.Create(TotalSize, block);

    // Выделите немного места.
    u64 offset = INVALID::ID;  // Начните с неверного идентификатора, что является хорошим вариантом по умолчанию, поскольку такого никогда не должно произойти.
    bool result = list.AllocateBlock(64, offset);
    // Убедитесь, что результат верен, смещение должно быть установлено на 0.
    ExpectToBeTrue(result);
    ExpectShouldBe(0, offset);

    // Убедитесь, что правильное количество свободного места.
    u64 FreeSpace = list.FreeSpace();
    ExpectShouldBe(TotalSize - 64, FreeSpace);

    // Теперь освободите блок.
    result = list.FreeBlock(64, offset);
    // Убедитесь, что результат верен
    ExpectToBeTrue(result);

    // Убедитесь, что весь блок свободен.
    FreeSpace = list.FreeSpace();
    ExpectShouldBe(TotalSize, FreeSpace);

    // Уничтожьте и убедитесь, что память не назначена.
    list.~FreeList();
    ExpectShouldBe(0, (bool)list);
    MMemory::Free(block, MemoryRequirement, MemoryTag::Application);

    return true;
}

u8 FreelistShouldAllocateOneAndFreeMulti() {
    FreeList list;

    // Получите требования к памяти
    u64 MemoryRequirement = 0;
    u64 TotalSize = 512;
    list.GetMemoryRequirement(TotalSize, MemoryRequirement);

    // Выделите и создайте FreeList.
    void* block = MMemory::Allocate(MemoryRequirement, MemoryTag::Application);
    list.Create(TotalSize, block);

    // Выделите немного места.
    u64 offset = INVALID::ID;  // Начните с неверного идентификатора, что является хорошим вариантом по умолчанию, поскольку такого никогда не должно произойти.
    bool result = list.AllocateBlock(64, offset);
    // Убедитесь, что результат верен, смещение должно быть установлено на 0.
    ExpectToBeTrue(result);
    ExpectShouldBe(0, offset);

    // Выделите еще немного места.
    u64 offset2 = INVALID::ID;  // Начните с неверного идентификатора, что является хорошим вариантом по умолчанию, поскольку такого никогда не должно произойти.
    result = list.AllocateBlock(64, offset2);
    // Убедитесь, что результат верен, смещение должно быть установлено в размере предыдущего выделения.
    ExpectToBeTrue(result);
    ExpectShouldBe(64, offset2);

    // Выделите еще одно место.
    u64 offset3 = INVALID::ID;  // Начните с неверного идентификатора, что является хорошим вариантом по умолчанию, поскольку такого никогда не должно произойти.
    result = list.AllocateBlock(64, offset3);
    // Убедитесь, что результат верен, смещение должно быть установлено равным смещению + размеру предыдущего выделения.
    ExpectToBeTrue(result);
    ExpectShouldBe(128, offset3);

    // Убедитесь, что правильное количество свободного места.
    u64 FreeSpace = list.FreeSpace();
    ExpectShouldBe(TotalSize - 192, FreeSpace);

    // Теперь освободите средний блок.
    result = list.FreeBlock(64, offset2);
    // Убедитесь, что результат верен
    ExpectToBeTrue(result);

    // Убедитесь, что правильная сумма является бесплатной.
    FreeSpace = list.FreeSpace();
    ExpectShouldBe(TotalSize - 128, FreeSpace);

    // Выделите еще немного места, это должно снова заполнить средний блок.
    u64 offset4 = INVALID::ID;  // Начните с неверного идентификатора, что является хорошим вариантом по умолчанию, поскольку такого никогда не должно произойти.
    result = list.AllocateBlock(64, offset4);
    // Убедитесь, что результат верен, смещение должно быть установлено в размере предыдущего выделения.
    ExpectToBeTrue(result);
    ExpectShouldBe(offset2, offset4);  // Смещение должно быть таким же, как 2, поскольку оно занимает то же пространство.

    // Убедитесь, что правильное количество свободного места.
    FreeSpace = list.FreeSpace();
    ExpectShouldBe(TotalSize - 192, FreeSpace);

    // Освободите первый блок и проверьте место.
    result = list.FreeBlock(64, offset);
    ExpectToBeTrue(result);
    FreeSpace = list.FreeSpace();
    ExpectShouldBe(TotalSize - 128, FreeSpace);

    // Освободите последний блок и проверьте место.
    result = list.FreeBlock(64, offset3);
    ExpectToBeTrue(result);
    FreeSpace = list.FreeSpace();
    ExpectShouldBe(TotalSize - 64, FreeSpace);

    // Освободите средний блок и проверьте место.
    result = list.FreeBlock(64, offset4);
    ExpectToBeTrue(result);
    FreeSpace = list.FreeSpace();
    ExpectShouldBe(TotalSize, FreeSpace);

    // Уничтожьте и убедитесь, что память не назначена.
    list.~FreeList();
    ExpectShouldBe(0, (bool)list);
    MMemory::Free(block, MemoryRequirement, MemoryTag::Application);

    return true;
}

u8 FreelistShouldAllocateOneAndFreeMultiVaryingSizes() {
    FreeList list;

    // Получите требования к памяти
    u64 MemoryRequirement = 0;
    u64 TotalSize = 512;
    list.GetMemoryRequirement(TotalSize, MemoryRequirement);

    // Выделите и создайте FreeList.
    void* block = MMemory::Allocate(MemoryRequirement, MemoryTag::Application);
    list.Create(TotalSize, block);

    // Allocate some space.
    u64 offset = INVALID::ID;  // Начните с неверного идентификатора, что является хорошим вариантом по умолчанию, поскольку такого никогда не должно произойти.
    bool result = list.AllocateBlock(64, offset);
    // Убедитесь, что результат верен, смещение должно быть установлено на 0.
    ExpectToBeTrue(result);
    ExpectShouldBe(0, offset);

    // Выделите еще немного места.
    u64 offset2 = INVALID::ID;  // Начните с неверного идентификатора, что является хорошим вариантом по умолчанию, поскольку такого никогда не должно произойти.
    result = list.AllocateBlock(32, offset2);
    // Убедитесь, что результат верен, смещение должно быть установлено в размере предыдущего выделения.
    ExpectToBeTrue(result);
    ExpectShouldBe(64, offset2);

    // Выделите еще одно место.
    u64 offset3 = INVALID::ID;  // Начните с неверного идентификатора, что является хорошим вариантом по умолчанию, поскольку такого никогда не должно произойти.
    result = list.AllocateBlock(64, offset3);
    // Убедитесь, что результат верен, смещение должно быть установлено равным смещению + размеру предыдущего выделения.
    ExpectToBeTrue(result);
    ExpectShouldBe(96, offset3);

    // Убедитесь, что правильное количество свободного места.
    u64 FreeSpace = list.FreeSpace();
    ExpectShouldBe(TotalSize - 160, FreeSpace);

    // Теперь освободите средний блок.
    result = list.FreeBlock(32, offset2);
    // Убедитесь, что результат верен
    ExpectToBeTrue(result);

    // Убедитесь, что правильная сумма является бесплатной.
    FreeSpace = list.FreeSpace();
    ExpectShouldBe(TotalSize - 128, FreeSpace);

    // Выделите еще немного места, на этот раз больше, чем старый средний блок. 
    // В конце списка должно быть новое смещение.
    u64 offset4 = INVALID::ID;  // Начните с неверного идентификатора, что является хорошим вариантом по умолчанию, поскольку такого никогда не должно произойти.
    result = list.AllocateBlock(64, offset4);
    // Убедитесь, что результат верен, смещение должно быть установлено в размере предыдущего выделения.
    ExpectToBeTrue(result);
    ExpectShouldBe(160, offset4);  // Смещение должно быть концом, поскольку оно занимает больше места, чем старый средний блок.

    // Убедитесь, что правильное количество свободного места.
    FreeSpace = list.FreeSpace();
    ExpectShouldBe(TotalSize - 192, FreeSpace);

    // Освободите первый блок и проверьте место.
    result = list.FreeBlock(64, offset);
    ExpectToBeTrue(result);
    FreeSpace = list.FreeSpace();
    ExpectShouldBe(TotalSize - 128, FreeSpace);

    // Освободите последний блок и проверьте место.
    result = list.FreeBlock(64, offset3);
    ExpectToBeTrue(result);
    FreeSpace = list.FreeSpace();
    ExpectShouldBe(TotalSize - 64, FreeSpace);

    // Освободите средний (теперь конечный) блок и проверьте место.
    result = list.FreeBlock(64, offset4);
    ExpectToBeTrue(result);
    FreeSpace = list.FreeSpace();
    ExpectShouldBe(TotalSize, FreeSpace);

    // Уничтожьте и убедитесь, что память не назначена.
    list.~FreeList();
    ExpectShouldBe(0, (bool)list);
    MMemory::Free(block, MemoryRequirement, MemoryTag::Application);

    return true;
}

u8 FreelistShouldAllocateToFullAndFailToAllocateMore() {
    FreeList list;

    // Получите требования к памяти
    u64 MemoryRequirement = 0;
    u64 TotalSize = 512;
    list.GetMemoryRequirement(TotalSize, MemoryRequirement);

    // Выделите и создайте FreeList.
    void* block = MMemory::Allocate(MemoryRequirement, MemoryTag::Application);
    list.Create(TotalSize, block);

    // Выделите все пространство.
    u64 offset = INVALID::ID;  // Начните с неверного идентификатора, что является хорошим вариантом по умолчанию, поскольку такого никогда не должно произойти.
    bool result = list.AllocateBlock(512, offset);
    // Убедитесь, что результат верен, смещение должно быть установлено на 0.
    ExpectToBeTrue(result);
    ExpectShouldBe(0, offset);

    // Убедитесь, что правильное количество свободного места.
    u64 FreeSpace = list.FreeSpace();
    ExpectShouldBe(0, FreeSpace);

    // Теперь попробуйте выделить еще немного
    u64 offset2 = INVALID::ID;  // Начните с неверного идентификатора, что является хорошим вариантом по умолчанию, поскольку такого никогда не должно произойти.
    MDEBUG("Следующее предупреждающее сообщение является преднамеренным.");
    result = list.AllocateBlock(64, offset2);
    // Убедитесь, что результат неверен
    ExpectToBeFalse(result);


    // Убедитесь, что правильное количество свободного места.
    FreeSpace = list.FreeSpace();
    ExpectShouldBe(0, FreeSpace);

    // Уничтожьте и убедитесь, что память не назначена.
    list.~FreeList();
    ExpectShouldBe(0, (bool)list);
    MMemory::Free(block, MemoryRequirement, MemoryTag::Application);

    return true;
}

void FreelistRegisterTests()
{
    TestManagerRegisterTest(FreelistShouldCreateAndDestroy, "Freelist должен создавать и уничтожать");
    TestManagerRegisterTest(FreelistShouldAllocateOneAndFreeOne, "Выделите Freelist и освободите одну запись.");
    TestManagerRegisterTest(FreelistShouldAllocateOneAndFreeMulti, "Freelist выделяет и освобождает несколько записей.");
    TestManagerRegisterTest(FreelistShouldAllocateOneAndFreeMultiVaryingSizes, "Freelist выделяет и освобождает несколько записей разного размера.");
    TestManagerRegisterTest(FreelistShouldAllocateToFullAndFailToAllocateMore, "Freelist выделяет полностью и терпит неудачу при попытке выделить больше.");
}