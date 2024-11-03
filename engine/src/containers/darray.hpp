#pragma once
#include "defines.hpp"
#include "core/mmemory.hpp"

#include <utility>

template<typename T>
void Swap(T& a, T& b) {
    auto temp = std::move(a);
    a = std::move(b);
    b = std::move(temp);
}

template<typename T>
i32 Partition(T arr[], i32 LowIndex, i32 HighIndex, bool ascending) {
    auto pivot = arr[HighIndex];
    i32 i = (LowIndex - 1);

    for (i32 j = LowIndex; j <= HighIndex - 1; ++j) {
        if (ascending) {
            if (arr[j].distance < pivot.distance) {
                ++i;
                Swap(arr[i], arr[j]);
            }
        } else {
            if (arr[j].distance > pivot.distance) {
                ++i;
                Swap(arr[i], arr[j]);
            }
        }
    }
    Swap(arr[i + 1], arr[HighIndex]);
    return i + 1;
}

/// @brief Частная, рекурсивная, функция сортировки на месте для структур GeometryDistance.
/// @tparam T тип объектов(переменных), которые находятся в массиве
/// @param arr Массив структур GeometryDistance, которые нужно отсортировать.
/// @param LowIndex Низкий индекс, с которого нужно начать сортировку (обычно 0)
/// @param HighIndex Высокий индекс, которым нужно закончить (обычно длина массива - 1)
/// @param ascending true для сортировки в порядке возрастания; в противном случае - по убыванию.
template<typename T>
void QuickSort(T arr[], i32 LowIndex, i32 HighIndex, bool ascending)
{
    if (LowIndex < HighIndex) {
        i32 partitionIndex = Partition(arr, LowIndex, HighIndex, ascending);

        // Независимая сортировка элементов до и после индекса раздела.
        QuickSort(arr, LowIndex, partitionIndex - 1, ascending);
        QuickSort(arr, partitionIndex + 1, HighIndex, ascending);
    }
}

template<typename T>
class MAPI DArray
{
// Переменные
private:
    u64 size      {};   // Количество элементов в массиве
    u64 capacity  {};   // Выделенная память под данное количество элементов
    T* data{nullptr};   // Указатель на область памяти где хранятся элементы

// Функции
public:
    constexpr DArray() : size(), capacity(), data(nullptr) {}
    constexpr DArray(T&& value) { PushBack(value); }
    constexpr DArray(u64 size) : size(), capacity(size), data(size ? MMemory::TAllocate<T>(Memory::DArray, capacity, true) : nullptr) {}
    
    /// @brief Конструктор копирования
    /// @param other динамический массив из которого нужно копировать данные
    constexpr DArray(const DArray& other) : size(other.size), capacity(other.capacity), data() {
        if (!capacity) {
            return;
        }
        
        data = MMemory::TAllocate<T>(Memory::DArray, capacity);
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
        if(data) {
            Clear();
            MMemory::Free(data, sizeof(T) * capacity, Memory::DArray);
            size = capacity = 0;
            data = nullptr;
        }
    }

    // Операторы--------------------------------------------------------------------------------

    /// @brief Присваивает внутренние данные одного массива другому.
    /// ПРИМЕЧАНИЕ: если будет удалены данные одного массива то и второго тоже
    /// @param darr константная ссылка на массив данные которого нужно присвоить
    /// @return Возвращает ссылку на динамический массив
    DArray& operator=(const DArray& da) {
        Clear();
        if (!data && capacity < da.size) {
            Resize(da.size);
        }
        for (u64 i = 0; i < da.size; i++) {
            data[i] = da.data[i];
        }
        
        return *this;
    }

    DArray& operator=(DArray&& darr) {
        if (data) {
            Clear();
        }
        data = darr.data;
        size = darr.size;
        capacity = darr.capacity;
        darr.data = nullptr;
        darr.size = 0;
        darr.capacity = 0;
        return *this;
    }

    explicit operator bool() const {
        if (!data && capacity < 1 && size < 1) {
            return false;
        }
        return true;
    }

    // Доступ к элементу------------------------------------------------------------------------

    constexpr T& operator [] (u64 index) {
        if(index < 0 || index >= size) {
            MERROR("Индекс за пределами этого массива! Длина: %i, индекс: %i", size, index);
        }
        return data[index];
    }

    // Доступ к элементу
    const T& operator [] (u64 index) const {
        if(index < 0 || index >= size) {
            MERROR("Индекс за пределами этого массива! Длина: %i, индекс: %i", size, index);
        }
        return data[index];
    }

    void Create(T* data, u64 size) {
        if (this->data) {
            MERROR("Динамический массив уже был создан!");
            return;
        }
        
        this->data = data;
        this->size = capacity = size;
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
        if (NewCap == 0) {
            MWARN("DArray::Reserve - резерв не был выполнен, т.к. было указан 0.")
            return;
        }
        
        if (capacity == 0) {
            data = MMemory::TAllocate<T>(Memory::DArray, NewCap, true);
            capacity = NewCap;
        }
        else if (NewCap > capacity) {
            T* ptrNew = MMemory::TAllocate<T>(Memory::DArray, NewCap, true);
            for (u64 i = 0; i < size; i++) {
                ptrNew[i] = static_cast<T&&>(data[i]);
            }
            MMemory::Free(data, sizeof(T) * capacity, Memory::DArray);
            data = ptrNew;
            capacity = NewCap;
        }
    }
    /// @return количество элементов контейнера
    constexpr const u64& Length() const noexcept {
        return size;
    }
    /// @return количество зарезервированных ячеек памяти типа Т
    constexpr const u64 Capacity() const noexcept {
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
        if(!capacity && !size) {
            Reserve(2);
        } else if(size == capacity) {
            Reserve(capacity * 2);
        }
        data[size] = value;
        size++;
    }

    void PushBack(T&& value)
    {
        if(!capacity && !size) {
            Reserve(2);
        } else if(size == capacity) {
            Reserve(capacity * 2);
        }
        data[size] = static_cast<T&&>(value);
        size++;
    }

    template<typename... Args>
    T& EmplaceBack(Args&&... args) {
        if(!capacity && !size) {
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

    /// @brief Изменяет размер контейнера, чтобы он содержал NewSize элементов инициализируемых по умолчанию.
    void Resize(u64 NewSize) {
        if(NewSize > capacity) {
            Reserve(NewSize);
        }
        size = NewSize;
    }

    /// @brief Изменяет размер контейнера, чтобы он содержал NewSize value элементов.
    /// @param NewSize новый размер контейнера
    void Resize(u64 NewSize, const T& value) {
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
