/**
 * @file queue.h
 * @author
 * @brief Простой контейнер типа очередь. Элементы извлекаются из очереди в том же порядке, в котором они туда помещались.
 * @version 1.0
 * @date 10.11.2026
 *
 * @copyright
 *
 */
#pragma once

#include "defines.h"

/// @brief Простой контейнер очереди. Элементы извлекаются из очереди в том же порядке, в котором они туда помещались.
class MAPI Queue 
{
public:
    /// @brief Конструктор по умолчанию
    constexpr Queue() : element_size(), element_count(), allocated(), memory() {}
    constexpr Queue(u32 ElementSize) : element_size(ElementSize), element_count(), allocated(), memory(Allocate(1)) {}
    ~Queue() { Destroy(); }

    NON_COPYABLE(Queue);

    /// @brief Конструкторм перемещения
    /// @param other очередь которую "перемещаем"
    constexpr Queue(Queue&& other) : element_size(other.element_size), element_count(other.element_count), allocated(other.allocated), memory(other.memory) {
        other.element_size = other.element_count = other.allocated = 0; other.memory = nullptr;
    }

    /// @brief Создает новую очередь.
    /// @param ElementSize Размер каждого элемента в очереди.
    /// @return true при успехе; иначе false.
    bool Create(u32 ElementSize);

    /// @brief Уничтожает данную очередь.
    void Destroy();

    u32 ElementCount() { return element_count; }

    /// @brief Добавляет элемент (копию данных элемента) в конец очереди.
    /// @param ElementData Данные элемента, которые нужно добавить. Обязательно.
    /// @return true при успехе; иначе false.
    bool Push(void* ElementData);

    /// @brief Попытки заглянуть в элемент (записать копию данных элемента при успехе) из очереди. 
    /// Если очередь пуста, ничего не происходит и возвращается false. Память очереди не изменяется.
    /// @param OutElementData Указатель, куда записывать данные элемента. Обязательно.
    /// @return true при успехе; иначе false.
    bool Peek(void* OutElementData) const; 

    /// @brief Попытки извлечь элемент (с одновременным созданием его копии в случае успеха) из начала очереди. 
    /// Если очередь пуста, ничего не происходит и возвращается false.
    /// @param OutElementData Указатель, чтобы записать данные элемента. Обязательно.
    /// @return true при успехе; иначе false.
    bool Pop(void* OutElementData);

private:
    u32 element_size;   // Размер элемента в байтах. 
    u32 element_count;  // Текущее количество элементов. 
    u32 allocated;      // Общее количество выделенной памяти.
    void* memory;       // Выделенный блок памяти.

    constexpr void* Allocate(u32 count);
};
