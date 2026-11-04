#pragma once

#include "core/memory_system.h"

/// @brief Простой стековый контейнер. Элементы могут быть только вставлены в стек или вытащены из него.
/// @tparam T Тип элемента который может быть вставлен в стек.
template<typename T>
class Stack
{
public:
    /// @brief Размер элемента, который хранится в байтах.
    u32 ElementSize;
    /// @brief Количество хранящихся элементов.
    u32 ElementCount;
    /// @brief Выделенный блок памяти, где хранятся данные.
    T* data;
    /// @brief Общий объем выделенной в данный момент памяти.
    u32 allocated;

    constexpr Stack(): ElementSize(sizeof(T)), ElementCount(), data((T*)MemorySystem::Allocate(ElementSize, Memory::Stack, true)), allocated(ElementSize) {}

    constexpr Stack(const T& element) : ElementSize(sizeof(T)), ElementCount(1), data(MemorySystem::Allocate(ElementSize, Memory::Stack, true)), allocated(ElementSize) {
        Push(element);
    }

    constexpr Stack(T&& element) : ElementSize(sizeof(T)), ElementCount(1), data(MemorySystem::Allocate(ElementSize, Memory::Stack, true)), allocated(ElementSize) {
        Push((T&&)element);
    }

    ~Stack() { MemorySystem::Free(data, allocated, Memory::Stack); }

    operator bool() const {
        return ElementCount;
    }

    ///@brief Пытается получить элемент (записывая копию данных элемента в случае успеха) из стека. 
    /// Если стек пуст, ничего не делается и возвращается false. Память стека не изменяется.
    /// @param OutElementData Указатель для записи данных элемента. Необходимый.
    /// @return true при успехе; в противном случае false.
    bool Peek(T& OutElementData) const {
        if (ElementCount < 1) {
            MWARN("Невозможно просмотреть пустую стопку.");
            return false;
        }

        OutElementData = data[ElementCount - 1];

        return true;
    }

    /// @brief Помещает элемент (копию данных элемента) в стек.
    /// @param element данные элемента, которые необходимо отправить.
    /// @return true при успехе; в противном случае false.
    bool Push(const T& element) {
        EnsureAllocated(ElementCount + 1);
        data[ElementCount] = element;
        ElementCount++;
        return true;
    }

    bool Push(T&& element) {
        EnsureAllocated(ElementCount + 1);
        data[ElementCount] = (T&&)element;
        ElementCount++;
        return true;
    }

    /// @brief Извлекает верхний элемент и записывает его в указанную ссылку
    /// @param element ссылка на переменную в которую нужно записать извлеченный элемент.
    /// @return true если успешно, иначе false
    bool Pop(T& element) {
        if (ElementCount < 1) {
            MWARN("Невозможно извлечь из пустого стека.");
            return false;
        }

        element = (T&&)data[ElementCount - 1];

        ElementCount--;

        return true;
    }

    /// @brief Извлекает верхний элемент.
    /// @param element указатель на переменную в которую нужно записать извлеченный элемент. Если ничего не указано, то просто удаляет последний элемент
    /// @return true если успешно, иначе false
    bool Pop(T* element = nullptr) {
        if (ElementCount < 1) {
            MWARN("Невозможно извлечь из пустого стека.");
            return false;
        }

        if (element) {
            *element = (T&&)data[ElementCount - 1];
        }

        ElementCount--;

        return true;
    }

    /// @return указатель на верхний элемент (без удаления).
    T* Top() const {
        return &data[ElementCount - 1];
    }
private:
    void EnsureAllocated(u32 count) {
        if (allocated < ElementSize * count) {
            // data = reinterpret_cast<T*>(MemorySystem::Realloc(data, ));
            T* temp = (T*)MemorySystem::Allocate(ElementSize * count, Memory::Stack);
            if (data) {
                for (u32 i = 0; i < ElementCount; i++) {
                    temp[i] = (T&&)data[i];
                }
                MemorySystem::Free(data, allocated, Memory::Stack);
            }
            data = temp;
            allocated = count * ElementSize;
        }
    }
};
