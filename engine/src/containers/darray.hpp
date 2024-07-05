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
    T* data{nullptr};   // Указатель на область памяти где хранятся элементы

// Функции
public:
    constexpr DArray() : size(), capacity(), data(nullptr) {}
    constexpr DArray(u64 capacity) : size(), capacity(capacity), data(capacity ? MMemory::TAllocate<T>(MemoryTag::DArray, capacity, true) : nullptr) {}
    constexpr DArray(u64 size, const T& value) {
        if(size > 0) {
            this->size = size;
            this->capacity = size;
            data = MMemory::TAllocate<T>(MemoryTag::DArray, capacity, true);
            for (u64 i = 0; i < size; i++) {
                data[i] = value;
            }
        }
    }
    
    /// @brief Конструктор копирования
    /// @param other динамический массив из которого нужно копировать данные
    constexpr DArray(const DArray& other) : size(other.size), capacity(other.capacity), data() {
        data = MMemory::TAllocate<T>(MemoryTag::DArray, capacity);
        for (u64 i = 0; i < size; i++) {
            data[i] = other.data[i];
        }
    }
    /// @brief Конструктор перемещения
    /// @param other динамический массив из которого нужно переместить данные
    constexpr DArray(DArray&& other) : size(other.size), capacity(other.capacity), data(other.data) {
        other.size = 0;
        other.capacity = 0;
        other.data = nullptr;
    }

    ~DArray() {
        if(this->data) {
            Clear();
            MMemory::Free(data, sizeof(T) * capacity, MemoryTag::DArray);
            size = capacity = 0;
            data = nullptr;
        }
    }

    // Операторы--------------------------------------------------------------------------------

    DArray& operator=(const DArray& darr) {
        if (data) {
            Clear();
        }
        data = MMemory::TAllocate<T>(MemoryTag::DArray, darr.capacity);
        for (u64 i = 0; i < darr.size; i++) {
            data[i] = darr.data[i];
        }
        size = darr.size;
        return *this;
    }

    // Доступ к элементу------------------------------------------------------------------------

    constexpr T& operator [] (u64 index) {
        if(index < 0 || index >= size) {
            MERROR("Индекс за пределами этого массива! Длина: %i, индекс: %i", size, index);
        }
        return data[index];
    }
    // Доступ к элементу
    constexpr const T& operator [] (u64 index) const {
        if(index < 0 || index >= size) {
            MERROR("Индекс за пределами этого массива! Длина: %i, индекс: %i", size, index);
        }
        return data[index];
    }
    // ------------------------------------------------------------------------Доступ к элементу

    /// @brief Возвращает указатель на базовый массив, служащий хранилищем элементов.
    /// @return Указатель на базовый массив, служащий хранилищем элементов.
    MINLINE T* Data() {
        return data;
    }
    /// @brief Возвращает указатель на базовый массив, служащий хранилищем элементов.
    /// @return Указатель на базовый массив.
    constexpr T* Data() const {
        return data;
    }
    // Емкость----------------------------------------------------------------------------------

    /// @brief Увеличьте емкость вектора (общее количество элементов, 
    /// которые вектор может содержать без необходимости перераспределения) до значения, 
    /// большего или равного NewCap. Если значение NewCap больше текущей capacity(емкости), 
    /// выделяется новое хранилище, в противном случае функция ничего не делает.
    void Reserve(u64 NewCap) {
        // ЗАДАЧА: добавить std::move()
        if (capacity == 0) {
            data = MMemory::TAllocate<T>(MemoryTag::DArray, NewCap, true);
            capacity = NewCap;
        }
        else if (NewCap > capacity) {
            T* ptrNew = MMemory::TAllocate<T>(MemoryTag::DArray, NewCap, true);
            for (u64 i = 0; i < size; i++) {
                ptrNew[i] = std::move(data[i]);
            }
            MMemory::Free(data, sizeof(T) * capacity, MemoryTag::DArray);
            data = ptrNew;
            capacity = NewCap;
        }
    }
    /// @return количество элементов контейнера
    constexpr u64 Length() const noexcept {
        return size;
    }
    /// @return количество зарезервированных ячеек памяти типа Т
    constexpr u64 Capacity() const noexcept {
        return capacity;
    }
    //-----------------------------------------------------------------------------------Емкость
    // Модифицирующие методы--------------------------------------------------------------------

    /// @brief Очищает массив. Емкость остается прежней.
    void Clear() {
        if (data && size){
            for (u64 i = 0; i < size; i++) {
                data[i].~T();
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
        data[size] = value;
        size++;
    }

    void PushBack(T&& value)
    {
        if(size == 0) {
            Reserve(2);
        } else if(size == capacity) {
            Reserve(capacity * 2);
        }
        data[size] = std::move(value);
        size++;
    }

    template<typename... Args>
    T& EmplaceBack(Args&&... args) {
        if(size == 0) {
            Reserve(2);
        } else if(size == capacity) {
            Reserve(capacity * 2);
        }
        data[size] = T(std::forward<Args>(args)...); 
        return data[size++];
    }

    /// @brief Удаляет последний элемент контейнера.
    void PopBack() {
        if(size > 0) {
            data[size].~T();
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
            MMemory::CopyMem((data + index), (data + index + 1), size - 1);
        }
        size++;
    }

    /// @brief Удаляет выбранный элемент.
    void PopAt(u64 index) {
        if (index >= size) MERROR("Индекс за пределами этого массива! Длина: %i, индекс: %index", size, index);

        // Если не последний элемент, вырезаем запись и копируем остальное внутрь. ЗАДАЧА: оптимизироваать
        if (index != size - 1) {
            MMemory::CopyMem(data + index, data + index + 1, size - 1);
        }
        size--;
    }

    /// @brief Изменяет размер контейнера, чтобы он содержал NewSize элементы, ничего не делает, если NewSize == size().
    /// @param NewSize новый размер контейнера
    void Resize(u64 NewSize, const T& value = T()) {
        if(NewSize > capacity) {
            Reserve(NewSize);
            // ЗАДАЧА: изменить
            for (u64 i = size; i < NewSize; i++) {
                data[i] = value;
            }
        
        }
        size = NewSize;
    }

    /// @brief Функция перемещает данные из вектора, после чего его длина и емкость становятся равными нулю.
    /// ПРИМЕЧАНИЕ: Перемещаемые данные нужно будет удалить вручную во избежание утечки памяти
    /// @return указатель на данные
    T* MovePtr() {
        T* ptr = data;
        data = nullptr;
        size = capacity = 0;
        return ptr;
    }
};
