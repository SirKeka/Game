#pragma once
#include "defines.hpp"
#include "core/mmemory.hpp"
#include <new>
//#include "core/logger.hpp"
/*
template <typename T, typename = void_t<>>
struct IsClass : std::false_type { };

template <typename T>
struct IsClass<T, void_t<int T::*>> : std::true_type { };
*/
template<typename T>
class MAPI DArray
{
// Переменные
private:
    u64 size{};             // Количество элементов в массиве
    u64 capacity{};         // Выделенная память под данное количество элементов
    T* ptrValue{nullptr};   // Указатель на область памяти где хранятся элементы

// Функции
public:
    constexpr DArray() : size(), capacity(), ptrValue(nullptr) {}
    constexpr DArray(u64 capacity) : size(), capacity(capacity), ptrValue(capacity ? MMemory::TAllocate<T>(MemoryTag::DArray, capacity) : nullptr) {}
    constexpr DArray(u64 size, const T& value) {
        if(size > 0) {
            this->size = size;
            this->capacity = size;
            ptrValue = MMemory::TAllocate<T>(MemoryTag::DArray, capacity);
            for (u64 i = 0; i < size; i++) {
                ptrValue[i] = value;
            }
        }
    }

    ~DArray() {
        if(this->ptrValue) {
            Clear();
            MMemory::Free(ptrValue, sizeof(T) * capacity, MemoryTag::DArray);
            size = capacity = 0;
            ptrValue = nullptr;
        }
    }

    // Конструктор копирования
    constexpr DArray(const DArray& arr) : size(arr.size), capacity(arr.capacity), ptrValue(arr.ptrValue) {}

    // Конструктор перемещения
    constexpr DArray(DArray&& arr) :  size(arr.size), capacity(arr.capacity), ptrValue(arr.ptrValue)
    {
        arr.size = 0;
        arr.capacity = 0;
        //arr.mem = nullptr;
        arr.ptrValue = nullptr;
    }

    // Доступ к элементу------------------------------------------------------------------------

    MINLINE T& operator [] (u64 index) {
        if(index < 0 || index >= size) {
            MERROR("Индекс за пределами этого массива! Длина: %i, индекс: %index", size, index);
        }
        return ptrValue[index];
    }
    // Доступ к элементу
    MINLINE const T& operator [] (u64 index) const {
        if(index < 0 || index >= size) {
            MERROR("Индекс за пределами этого массива! Длина: %i, индекс: %index", size, index);
        }
        return ptrValue[index];
    }
    // ------------------------------------------------------------------------Доступ к элементу

    /// @brief Возвращает указатель на базовый массив, служащий хранилищем элементов.
    /// @return Указатель на базовый массив, служащий хранилищем элементов.
    MINLINE T* Data() {
        return ptrValue;
    }
    /// @brief Возвращает указатель на базовый массив, служащий хранилищем элементов.
    /// @return Указатель на базовый массив.
    MINLINE const T* Data() const {
        return ptrValue;
    }
    // Емкость----------------------------------------------------------------------------------

    /// @brief Увеличьте емкость вектора (общее количество элементов, 
    /// которые вектор может содержать без необходимости перераспределения) до значения, 
    /// большего или равного NewCap. Если значение NewCap больше текущей capacity(емкости), 
    /// выделяется новое хранилище, в противном случае функция ничего не делает.
    void Reserve(u64 NewCap) {
        // TODO: добавить std::move()
        if (capacity == 0) {
            ptrValue = MMemory::TAllocate<T>(MemoryTag::DArray, NewCap);
            capacity = NewCap;
        }
        else if (NewCap > capacity) {
            T* ptrNew = MMemory::TAllocate<T>(MemoryTag::DArray, NewCap);
            for (u64 i = 0; i < size; i++) {
                ptrNew[i] = ptrValue[i];
            }
            MMemory::Free(ptrValue, sizeof(T) * capacity, MemoryTag::DArray);
            ptrValue = ptrNew;
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
    //-----------------------------------------------------------------------------------Емкость
    // Модифицирующие методы--------------------------------------------------------------------

    /// @brief Очищает массив. Емкость остается прежней.
    void Clear() {
        if (ptrValue && size){
            for (u64 i = 0; i < size; i++) {
                ptrValue[i].~T();
            }
            
            size = 0;
        }
    }
    /// @brief Добавляет заданное значение элемента в конец контейнера.
    /// @param value элемент который нужно поместить в конец контейнера.
    void PushBack(const T& value) {
        if(size == 0) {
            Reserve(2);
        } else if(size == capacity) {
            Reserve(capacity * 2);
        }
        // ptrValue + size = new(ptrValue + size) T(value);
        ptrValue[size] = value;
        size++;
    }
/*
    void PushBack(T&& value)
    {

    }
*/
    /// @brief Удаляет последний элемент контейнера.
    void PopBack() {
        if(size > 0) {
            ptrValue[size].~T();
            size--;
        }
    }

    /// @brief Вставляет value в позицию index.
    /// @param value элемент, который нужно ставить.
    /// @param index индекс элемента в контейнере.
    void Insert(const T& value, u64 index) {
        if (index >= size) {
            MERROR("Индекс за пределами этого массива! Длина: %i, индекс: %index", size, index);
        }
        if(index > size) {
            Resize(size + 1, value);
        }
        // Если не последний элемент, скопируйте остальное наружу.
        if (index != size - 1) {
            MMemory::CopyMem((ptrValue + index), (ptrValue + index + 1), size - 1);
        }
        size++;
    }

    /// @brief Удаляет выбранный элемент.
    void PopAt(u64 index) {
        if (index >= size) MERROR("Индекс за пределами этого массива! Длина: %i, индекс: %index", size, index);

        // Если не последний элемент, вырезаем запись и копируем остальное внутрь. TODO: оптимизироваать
        if (index != size - 1) {
            MMemory::CopyMem(ptrValue + index, ptrValue + index + 1, size - 1);
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
