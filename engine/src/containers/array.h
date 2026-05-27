#pragma once
#include "core/memory_system.h"
#include "memory/linear_allocator.h"

// ЗАДАЧА: Сделать на основе макросов?
/// @brief Массив выделяющий память в куче, в отличие от DArray не может изменить свой размер
/// @tparam T 
template<typename T>
class MAPI Array
{
public:
    constexpr Array() : capacity(), size(), FrameAllocator(), data() {}
    constexpr Array(u32 capacity, LinearAllocator* allocator = nullptr) : capacity(capacity), size(), FrameAllocator(allocator), data(allocator ? (T*)allocator->Allocate(sizeof(T) * capacity) : nul_alloc(T, capacity, Memory::Array)) {}
    ~Array() {
        if (!FrameAllocator) {
            MemorySystem::Free(data, capacity * sizeof(T), Memory::Array);
        }
        data = nullptr;
    }

    /// @brief Доступ к элементу
    /// @param index номерер элемента в массиве, который нужно получить
    /// @return элемент маассива по заданному индексу
    constexpr T& operator [] (u32 index) {
#if defined(_DEBUG)
    if (index > size) {
        MFATAL("Индекс получаемого элемента данных больше чем размер массива");
    }
#endif
    return data[index];
}

    /// @brief Доступ к элементу
    /// @param index номерер элемента в массиве, который нужно получить
    /// @return элемент маассива по заданному индексу
constexpr const T& operator [] (u32 index) const {
#if defined(_DEBUG)
    if (index > size) {
        MFATAL("Индекс получаемого элемента данных больше чем размер массива");
    }
#endif
    return data[index];
}

/// @return количество элементов, которое может храниться в контецнере
constexpr u32 Capacity() const {
    return capacity;
}

/// @brief Очищает массив. Емкость остается прежней.
void Clear() {
    if (data && size){
        for (u32 i = 0; i < size; i++) {
            data[i].~T();
        }
        
        size = 0;
    }
}

/// @brief Создает массив
/// @param capacity максимальное количество элементтов в массиве
/// @param s если true, то size = capacity
/// @param n если true, то массив является временным
/// @param allocator 
Array& Create(u32 capacity, bool s = false, bool t = false, LinearAllocator* allocator = nullptr) {
    this->capacity = capacity;
    if (s) {
        size = capacity;
    }
    
    if (!FrameAllocator && allocator) {
        FrameAllocator = true;
        data = (T*)allocator->Allocate(sizeof(T) * capacity);
    } else if (t) {
        data = AllocateTemp<T>(capacity, true, true);
    } else {
        data = (T*)MemorySystem::Allocate(sizeof(T) * capacity, Memory::Array, true);
    }
    return *this;
}

T* Data() { return data; }

/// @return количество элементов контейнера
constexpr u32 Size() const { return size; }

/// @brief 
/// @return Возвращает указатель на новый элемент в конце списка, для его последующего заполнения
T* PushBack() {
    if(size < capacity) {
        T* ret = &data[size];
        size++;
        return ret;            
    }
    return nullptr;
}

/// @brief Добавляет заданное значение элемента в конец контейнера.
/// @param value элемент который нужно поместить в конец контейнера.
void PushBack(const T& value) {
    if (size < capacity) {
        data[size] = value;
        size++;
    }
}

/// @brief Перемещает заданное значение элемента в конец контейнера.
/// @param value элемент который нужно переместить в конец контейнера.
void PushBack(T&& value)
{
    if(size < capacity) {
        data[size] = (T&&)value;
        size++;
    }
}

/// @brief Изменяет размер контейнера, чтобы он содержал NewSize элементов инициализируемых по умолчанию.
void Resize(u32 NewSize) {
    if(NewSize < capacity) {
        size = NewSize;
    }
}

private:
    // Переменные
    /// @brief Выделенная память под данное количество элементов
    u16 capacity;
    /// @brief Количество инициализированных элементов в массиве
    u16 size;
    /// @brief Указывает является ли массив временным
    bool temp;
    /// @brief Указывает используется ли линейный распределитель или нет
    bool FrameAllocator;
    /// @brief Указатель на область памяти где хранятся элементы
    T* data;
};
