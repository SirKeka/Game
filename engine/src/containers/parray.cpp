#include "parray.h"
#include "core/memory_system.h"
#include "memory/linear_allocator.h"

void *_PArrayCreate(u64 length, u64 stride, LinearAllocator *FrameAllocator)
{
    u64 HeaderSize = sizeof(PArrayHeader);
    u64 ArraySize = length * stride;
    void* NewArray = nullptr;
    
    if (FrameAllocator) {
        NewArray = FrameAllocator->Allocate(HeaderSize + ArraySize);
        MemorySystem::ZeroMem(NewArray, HeaderSize + ArraySize);
    } else {
        NewArray = MemorySystem::Allocate(HeaderSize + ArraySize, Memory::DArray, true);
    }

    if (length == 0) {
        MFATAL("Вызвана функция _parray_create с длиной 0.");
    }
    auto header = (PArrayHeader*)NewArray;
    header->capacity = length;
    header->stride = stride;
    header->allocator = FrameAllocator;

    return (void*)((u8*)NewArray + HeaderSize);
}

void PArrayDestroy(void *array)
{
    if (!array) { return; }
    u64 HeaderSize = sizeof(PArrayHeader);
    auto header = (PArrayHeader*)((u8*)array - HeaderSize);
    u64 TotalSize = HeaderSize + header->capacity * header->stride;
    if (!header->allocator) {
        MemorySystem::Free(header, TotalSize, Memory::DArray);
    }
}

void *_PArrayResize(void *array)
{
    u64 HeaderSize = sizeof(PArrayHeader);
    PArrayHeader* header = (PArrayHeader*)((u8*)array - HeaderSize);
    if (header->capacity == 0) {
        MFATAL("Функция _parray_resize была вызвана для массива с ёмкостью 0. Такого быть не должно.");
        return nullptr;
    }
    void* temp = _PArrayCreate((PARRAY_RESIZE_FACTOR * header->capacity), header->stride, header->allocator);

    PArrayHeader* NewHeader = (PArrayHeader*)((u8*)temp - HeaderSize);
    NewHeader->length = header->length;

    MemorySystem::CopyMem(temp, array, header->length * header->stride);

    PArrayDestroy(array);
    return temp;
}

void *_PArrayPush(void *array, const void *value_ptr)
{
    u64 HeaderSize = sizeof(PArrayHeader);
    auto header = (PArrayHeader*)((u8*)array - HeaderSize);
    if (header->length >= header->capacity) {
        array = _PArrayResize(array);
        header = (PArrayHeader*)((u8*)array - HeaderSize);
    }

    u64 addr = reinterpret_cast<u64>(array);
    addr += (header->length * header->stride);
    MemorySystem::CopyMem((void*)addr, value_ptr, header->stride);
    PArrayLengthSet(array, header->length + 1);
    return array;
}

void *_PArrayInsertAt(void *array, u64 index, void *value_ptr)
{
    u64 length = PArrayLength(array);
    u64 stride = PArrayStride(array);
    if (index >= length) {
        MERROR("Индекс выходит за границы этого массива! Длина: %i, индекс: %index", length, index);
        return array;
    }
    if (length >= PArrayCapacity(array)) {
        array = _PArrayResize(array);
    }

    u64 addr = reinterpret_cast<u64>(array);

    // Сдвинуть элемент(ы), начиная с указанного индекса, на одну позицию вперёд. 
    // Это должно происходить даже в том случае, если вставка выполняется по последнему индексу.
    MemorySystem::CopyMem(
        (void*)(addr + ((index + 1) * stride)),
        (void*)(addr + (index * stride)),
        stride * (length - index));

    // Установить значение по указанному индексу.
    MemorySystem::CopyMem((void*)(addr + (index * stride)), value_ptr, stride);

    PArrayLengthSet(array, length + 1);
    return array;
}

void *AcquireBack(void *array)
{
    auto header = &((PArrayHeader*)array)[-1];
    if (header->length >= header->capacity) {
        array = _PArrayResize(array);
        header = &((PArrayHeader*)array)[-1];
    }

    u64 addr = reinterpret_cast<u64>(array);
    addr += (header->length * header->stride);
    header->length++;
    return reinterpret_cast<void*>(addr);
}

void PArrayPop(void *array, void *dest)
{
    u64 length = ((PArrayHeader*)array)[-1].length;
    u64 stride = ((PArrayHeader*)array)[-1].stride;
    if (length < 1) {
        MERROR("Функция PArrayPop была вызвана для пустого динамического массива (darray). Действий не требуется.");
        return;
    }
    u64 addr = reinterpret_cast<u64>(array);
    addr += ((length - 1) * stride);
    MemorySystem::CopyMem(dest, reinterpret_cast<void*>(addr), stride);
    PArrayLengthSet(array, length - 1);
}

void *PArrayPopAt(void *array, u64 index, void *dest)
{
    u64 length = ((PArrayHeader*)array)[-1].length;
    u64 stride = ((PArrayHeader*)array)[-1].stride;
    if (index >= length) {
        MERROR("Индекс выходит за границы этого массива! Длина: %i, индекс: %index", length, index);
        return array;
    }

    u64 addr = reinterpret_cast<u64>(array);
    MemorySystem::CopyMem(dest, (void*)(addr + (index * stride)), stride);

    // Если это не последний элемент, удалите запись, а оставшуюся часть сдвиньте внутрь.
    if (index != length - 1) {
        MemorySystem::CopyMem(
            (void*)(addr + (index * stride)),
            (void*)(addr + ((index + 1) * stride)),
            stride * (length - (index - 1)));
    }

    PArrayLengthSet(array, length - 1);
    return array;
}
