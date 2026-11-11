#include "queue.h"
#include "core/memory_system.h"

bool Queue::Create(u32 ElementSize)
{
    element_size = ElementSize;
    Allocate(1);
    return true;
}

void Queue::Destroy()
{
    if (memory) {
        MemorySystem::Free(memory, allocated, Memory::Array);
    }
    MemorySystem::ZeroMem(this, sizeof(Queue));
}

bool Queue::Push(void *ElementData)
{
    Allocate(element_count + 1);
    MemorySystem::CopyMem((void*)((u64)memory + (element_count * element_size)), ElementData, element_size);
    element_count++;
    return true;
}

bool Queue::Peek(void *OutElementData) const
{
    if (element_count < 1) {
        MWARN("Невозможно заглянуть в пустую очередь.");
        return false;
    }

    // Скопировать первый элемент в OutElementData
    MemorySystem::CopyMem(OutElementData, memory, element_size);

    return true;
}

bool Queue::Pop(void *OutElementData)
{
    if (element_count < 1) {
        MWARN("Невозможно извлечь элемент из пустой очереди.");
        return false;
    }

    // Скопировать первый элемент в OutElementData
    MemorySystem::CopyMem(OutElementData, memory, element_size);

    // Сдвинуть всё «вперёд».
    
    MemorySystem::CopyMem(memory, (void*)(((u64)memory) + element_size), element_size * (element_count - 1));

    element_count--;

    return true;
}

constexpr void *Queue::Allocate(u32 count)
{
    if (allocated < element_size * count) {
        void* temp = MemorySystem::Allocate(count * element_size, Memory::Array);
        if (memory) {
            MemorySystem::CopyMem(temp, memory, allocated);
            MemorySystem::Free(memory, allocated, Memory::Array);
        }
        memory = temp;
        allocated = count * element_size;
    }
    return memory;
}
