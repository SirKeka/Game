#pragma once
#include "defines.hpp"
#include "core/mmemory.hpp"
#include "core/logger.hpp"

template<typename T>
class MAPI DArray
{
// Переменные
private:
    u64 size;
    u64 capacity;
    T* ptrValue;

// Функции
public:
    DArray() : size(0), capacity(0), ptrValue(nullptr) {}

    DArray(u64 lenght, const T& value = T{}) {
        if(lenght > 0) {
            this->size = lenght;
            this->capacity = lenght;
            ptrValue = MMemory::TAllocate<T>(capacity, MEMORY_TAG_DARRAY);
            for (u64 i = 0; i < lenght; i++) {
                ptrValue[i] = value;
            }
        }
    }

    ~DArray() {
        if(this->capacity != 0) MMemory::Free(reinterpret_cast<void*>(ptrValue), sizeof(T) * capacity, MEMORY_TAG_DARRAY);
    }

    // Конструктор копирования
    DArray(const DArray& arr) : size(arr.size), capacity(arr.capacity), ptrValue(arr.ptrValue) {}

    // Конструктор перемещения
    DArray(DArray&& arr) :  size(arr.size), capacity(arr.capacity), ptrValue(arr.ptrValue)
    {
        arr.size = 0;
        arr.capacity = 0;
        //arr.mem = nullptr;
        arr.ptrValue = nullptr;
    }

    // Доступ к элементу

    T& operator [] (u64 index) {
        if(index < 0 || index >= size) MERROR("Индекс за пределами этого массива! Длина: %i, индекс: %index", size, index);
        return ptrValue[index];
    }

    const T& operator [] (u64 index) const{
        if(index < 0 || index >= size) MERROR("Индекс за пределами этого массива! Длина: %i, индекс: %index", size, index);
        return ptrValue[index];
    }

    /// @return Возвращает указатель на базовый массив, служащий хранилищем элементов.
    T* Data() {
        return ptrValue;
    }

    const T* Data() const {
        return ptrValue;
    }

    // Емкость

    /// @brief Увеличьте емкость вектора (общее количество элементов, 
    /// которые вектор может содержать без необходимости перераспределения) до значения, 
    /// большего или равного NewCap. Если значение NewCap больше текущей capacity(емкости), 
    /// выделяется новое хранилище, в противном случае функция ничего не делает.
    void Reserve(const u64& NewCap) {
        // TODO: добавить std::move()
        if (capacity == 0) {
            ptrValue = MMemory::TAllocate<T>(NewCap, MEMORY_TAG_DARRAY);
            capacity = NewCap;
        }
        else if (NewCap > capacity) {
            void* ptrNew = MMemory::Allocate(sizeof(T) * NewCap, MEMORY_TAG_DARRAY);
            MMemory::CopyMem(ptrNew, reinterpret_cast<void*>(ptrValue), sizeof(T) * capacity);
            MMemory::Free(ptrValue, sizeof(T) * capacity, MEMORY_TAG_DARRAY);
            ptrValue = reinterpret_cast<T*> (ptrNew);
            capacity = NewCap;
        }
    }

    /// @return количество элементов контейнера
    constexpr u64 Lenght() const noexcept {
        return  this->size;
    }

    /// @return количество зарезервированных ячеек памяти типа Т
    u64 Capacity() {
        return this->capacity;
    }

    // Модифицирующие методы

    /// @brief Добавляет заданное значение элемента в конец контейнера.
    /// @param value элемент который нужно поместить в конец контейнера.
    void PushBack(const T& value) {
        if(size == 0) Reserve(2);
        if(size == capacity) Reserve(capacity * 2);
        ptrValue[size] = value;
        size++;
    }

    //void PushBack(T&& value);
    /// @brief Удаляет последний элемент контейнера.
    void PopBack() {
        if(size > 0) size--;
    }

    /// @brief Вставляет value в позицию index.
    /// @param value элемент, который нужно ставить.
    /// @param index индекс элемента в контейнере.
    void Insert(const T& value, u64 index) {
        if (index >= size) MERROR("Индекс за пределами этого массива! Длина: %i, индекс: %index", size, index);
        if(index > size) Resize(size + 1, value);
        // Если не последний элемент, скопируйте остальное наружу.
        if (index != size - 1) {
            MMemory::CopyMem(
                reinterpret_cast<void*>(ptrValue + (index* sizeof(T))),
                reinterpret_cast<void*>(ptrValue + (index + 1 * sizeof(T))),
                size - 1);
        }
        size++;
    }

    /// @brief Удаляет выбранный элемент.
    void PopAt(u64 index) {
        if (index >= size) MERROR("Индекс за пределами этого массива! Длина: %i, индекс: %index", size, index);

        // Если не последний элемент, вырезаем запись и копируем остальное внутрь. TODO: оптимизироваать
        if (index != size - 1) {
            MMemory::CopyMem(
                reinterpret_cast<void*>(ptrValue + (index * sizeof(T))),
                reinterpret_cast<void*>(ptrValue + (index + 1 * sizeof(T))),
                size - 1);
        }
        size--;
    }

    /// @brief Изменяет размер контейнера, чтобы он содержал NewSize элементы, ничего не делает, если NewSize == size().
    /// @param NewSize новый размер контейнера
    void Resize(u64 NewSize, const T& value = T()) {
        if(NewSize > capacity) {
            Reserve(NewSize);
            // TODO: изменить
            for (u64 i = size; i < NewSize; i++) {
                ptrValue[i] = value;
            }
        
        }
        size = NewSize;
        }
};
