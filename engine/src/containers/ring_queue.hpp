#pragma once

#include "core/mmemory.hpp"

/// @brief Представляет собой кольцевую очередь определенного размера. Не изменяет размер динамически. 
/// Естественно, это структура «первым пришел — первым вышел».
class RingQueue
{
private:
    u32   length;       // Текущее количество содержащихся элементов.
    u32   stride;       // Размер каждого элемента в байтах.
    u32   capacity;     // Общее количество доступных элементов.
    u8*   block;        // Блок памяти для хранения данных.
    bool  OwnsMemory;   // Указывает, владеет ли очередь своим блоком памяти.
    i32   head;         // Индекс головы списка.
    i32   tail;         // Индекс хвоста списка.
public:
    /// @brief Создает новую кольцевую очередь заданной емкости и шага.
    /// @param stride размер каждого элемента в байтах.
    /// @param capacity общее количество элементов, доступных в очереди.
    /// @param memory блок памяти, используемый для хранения данных. Должен быть размером шаг * емкость. 
    /// Если передан nullptr, блок автоматически выделяется и освобождается при создании/уничтожении.
    constexpr RingQueue(u32 stride, u32 capacity, void *memory)
    :
    length(),
    stride(stride),
    capacity(capacity),
    block(memory ? (u8*)memory : MMemory::TAllocate<u8>(Memory::RingQueue, capacity * stride)),
    OwnsMemory(memory ? false : true),
    head(),
    tail(-1) {}
    /// @brief Уничтожает указанную очередь. Если память не была передана при создании, она освобождается здесь.
    ~RingQueue();

    /// @brief Добавляет значение в очередь, если есть место.
    /// @param value значение, которое нужно добавить.
    /// @return true в случае успеха; в противном случае false.
    bool Enqueue(void* value);
    /// @brief Пытается извлечь следующее значение из предоставленной очереди.
    /// @param OutValue указатель для хранения извлеченного значения.
    /// @return true в случае успеха; в противном случае false.
    bool Dequeue(void* OutValue);
    /// @brief Пытается извлечь, но не удалить, следующее значение в очереди, если оно не пустое.
    /// @param OutValue указатель для хранения извлеченного значения.
    /// @return true в случае успеха; в противном случае false.
    bool Peek(void* OutValue);
    const u32& Length() const { return length; }
};
