#pragma once

#include "core/memory_system.h"
#include "memory/linear_allocator.h"

#include <utility>

template<typename T>
class MAPI DArray
{
    // Переменные
    /// @brief Количество элементов в массиве
    u32 size;
    /// @brief Выделенная память под данное количество элементов
    u32 capacity;
    /// @brief Размер одного элемента в байтах
    u32 elementSize;
    bool temp;
    /// @brief Указатель на область памяти где хранятся элементы
    T* data;
    /// @brief Указатель на линейный(кадровый) аллокатор
    LinearAllocator* allocator;

    constexpr T* Alloc(u64 size, bool temp, LinearAllocator* allocator) {
        if (temp) {
            return AllocateTemp<T>(size, true);
        } else if (allocator) {
            return (T*)allocator->Allocate(size);
        } else {
            return Allocate<T>(size, Memory::DArray, true);
        }
    };

// Функции
public:
    constexpr DArray() { MemorySystem::ZeroMem(this, sizeof(DArray)); }
    constexpr DArray(bool temp) : size(), capacity(), elementSize(sizeof(T)), temp(temp), data(), allocator() {}
    constexpr DArray(u32 capacity, bool temp = false, LinearAllocator* allocator = nullptr) : size(), capacity(capacity), elementSize(sizeof(T)), temp(temp), data(Alloc(capacity * elementSize, temp, allocator)), allocator() {}
    constexpr DArray(LinearAllocator* allocator) : size(), capacity(), elementSize(sizeof(T)), temp(), data(), allocator(allocator) {}
    constexpr DArray(u32 size, u32 capacity, void* array) : size(size), capacity(capacity ? capacity : size), elementSize(sizeof(T)), temp(), data((T*)array), allocator() {}

    /// @brief Конструктор копирования
    /// @param other динамический массив из которого нужно копировать данные
    constexpr DArray(const DArray& other) : size(other.size), capacity(other.capacity), elementSize(other.elementSize), temp(other.temp), data(), allocator(other.allocator) {
        if (!capacity) {
            return;
        }

        data = (T*)(MemorySystem::Allocate(capacity * elementSize, Memory::DArray));
        
        for (u32 i = 0; i < size; i++) {
            data[i] = other.data[i];
        }
    }
    /// @brief Конструктор перемещения
    /// @param other динамический массив из которого нужно переместить данные
    constexpr DArray(DArray&& other) : size(other.size), capacity(other.capacity), elementSize(other.elementSize), temp(other.temp), data(other.data), allocator(other.allocator) {
        other.size = other.capacity = other.elementSize = 0; other.temp = false; other.data = nullptr; other.allocator = nullptr;
    }

    ~DArray() {
        if(data) {
            Destroy();
        }
    }

    // Операторы--------------------------------------------------------------------------------

    /// @brief Присваивает внутренние данные одного массива другому.
    /// ПРИМЕЧАНИЕ: если будет удалены данные одного массива то и второго тоже
    /// @param darr константная ссылка на массив данные которого нужно присвоить
    /// @return Возвращает ссылку на динамический массив
    DArray& operator=(const DArray& da) {
        if (!da) {
            return *this;
        }
        
        Clear();
        if (!data && capacity < da.size) {
            Resize(da.size);
        }
        for (u64 i = 0; i < da.size; i++) {
            data[i] = da.data[i];
        }
        size = da.size;
        allocator = da.allocator;
        
        return *this;
    }

    DArray& operator=(DArray&& darr) {
        if (data && !temp) {
            Clear();
            MemorySystem::Free(data, capacity * elementSize, Memory::DArray);
        }
        data = darr.data;
        size = darr.size;
        capacity = darr.capacity;
        elementSize = darr.elementSize;
        allocator = darr.allocator;
        darr.data = nullptr; darr.allocator = nullptr;
        darr.size = darr.capacity = 0;
        return *this;
    }

    explicit operator bool() const {
        return data && capacity && size > 0;
    }

    // Доступ к элементу------------------------------------------------------------------------

    constexpr T& operator [] (u32 index) {
#if defined(_DEBUG)
        if (index > size) {
            MFATAL("Индекс получаемого элемента данных больше чем размер массива");
        }
#endif
        return data[index];
    }

    // Доступ к элементу
    constexpr const T& operator [] (u32 index) const {
#if defined(_DEBUG)
        if (index > size) {
            MFATAL("Индекс получаемого элемента данных больше чем размер массива");
        }
#endif
        return data[index];
    }

    void Create(T* data, u32 size) {
        if (this->data) {
            MERROR("Динамический массив уже был создан!");
            return;
        }
        
        this->data = data;
        this->size = capacity = size;
        this->elementSize = sizeof(T);
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

    /// @brief Увеличивает емкость массива (общее количество элементов, 
    /// которые вектор может содержать без необходимости перераспределения) до значения, 
    /// большего или равного NewCap. Если значение NewCap больше текущей capacity(емкости), 
    /// выделяется новое хранилище, в противном случае функция ничего не делает.
    /// @param NewCap величина памяти которой нужно зарезервировать. 
    /// ПРИМЕЧАНИЕ: если ничего не указано или указан 0, то резервируется 2 при capacity = 2 или х2 при capacity > 0
    void Reserve(u32 NewCap = 0) {
        if (capacity && capacity == NewCap) {
            return;
        }
        
        if (!elementSize) {
            elementSize = sizeof(T);
        }

        u64 CurrentSize = capacity * elementSize;
        // u64 NewSize = 0;
        if (capacity == 0) {
            capacity = NewCap ? NewCap : 2;

            data = Alloc(CurrentSize, temp, allocator); // (T*)MemorySystem::Allocate(CurrentSize, Memory::DArray, true);
        } else {
            if (NewCap == 0) {
                capacity *= 2;
                // NewSize = CurrentSize;
            } else {
                // NewSize = NewCap * elementSize;
                capacity = NewCap;
            }

            T* NewData = Alloc(CurrentSize, temp, allocator); // (T*)MemorySystem::Allocate(CurrentSize, Memory::DArray, true); // (T*)MemorySystem::Realloc(data, CurrentSize, NewSize, Memory::DArray);
            MemorySystem::CopyMem(NewData, data, CurrentSize);

            if (!NewData) {
                // size = capacity = 0;
                MERROR("DArray::Reserve: Не удалось выделить память");
                return;
            }
            data = NewData;
            // MemorySystem::ZeroMem(data + size, NewSize - CurrentSize);
        }
    }

    /// @brief Уменьшает емкость(capacity) массива до размера(size)
    void ShrinkToFit() {
        if (capacity > size && !temp) {
            MemorySystem::Free(data + size, (capacity - size) * elementSize, Memory::DArray);
            capacity = size;
        }
    }

    /// @return количество элементов контейнера
    constexpr u32 Length() const {
        return size;
    }
    /// @return количество зарезервированных ячеек памяти типа Т
    constexpr u32 Capacity() const {
        return capacity;
    }
    constexpr u32 ElementSize() {
        return elementSize;
    }
    constexpr u32 ElementSize() const {
        return elementSize;
    }
    //-----------------------------------------------------------------------------------Емкость
    // Модифицирующие методы--------------------------------------------------------------------

    void Destroy() {
        if (!temp) {
            Clear();
        
            MemorySystem::Free(data, elementSize * capacity, Memory::DArray);
        }
        
        size = capacity = 0;
        data = nullptr;
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

    /// @return Возвращает указатель на новый элемент в конце списка, для его последующего заполнения
    T* PushBack() {
        if(size == capacity) {
            Reserve();
        }

        T* ptr = &data[size];
        size++;

        return ptr;
    }
 
    /// @return Возвращает ссылку на новый элемент в конце списка, для его последующего заполнения
    /*T& PushBack() {
        if(size == capacity) {
            Reserve();
        }

        T& ref = data[size];
        size++;

        return ref;
    }*/

    /// @brief Добавляет заданное значение элемента в конец контейнера.
    /// @param value элемент который нужно поместить в конец контейнера.
    void PushBack(const T& value) {
        if(size == capacity) {
            Reserve();
        }
        data[size] = value;
        size++;
    }

    /// @brief Перемещает заданное значение элемента в конец контейнера.
    /// @param value элемент который нужно переместить в конец контейнера.
    void PushBack(T&& value)
    {
        if(size == capacity) {
            Reserve();
        }
        data[size] = (T&&)value;
        size++;
    }

    template<typename... Args>
    T& EmplaceBack(Args&&... args) {
        if(size == capacity) {
            Reserve();
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
    void Insert(const T& value, u32 index) {
        if (index >= size) {
            MERROR("Индекс за пределами этого массива! Длина: %i, индекс: %index", size, index);
        }
        if(index > size) {
            Resize(size + 1, value);
        }
        // Если не последний элемент, скопируйте остальное наружу.
        if (index != size - 1) {
            MemorySystem::CopyMem((data + index), (data + index + 1), size - 1);
        }
        size++;
    }

    /// @brief Достает выбранный элемент.
    /// @param index индекс элемента который нужно удалить.
    /// @param value указатель для хранения извлеченного элемента. 
    void PopAt(u64 index) {
        if (index >= size) MERROR("Индекс за пределами этого массива! Длина: %i, индекс: %index", size, index);

        // if (value) {
        //     *value = data[index];
        // }
        
        // Если не последний элемент, вырезаем запись и копируем остальное внутрь. ЗАДАЧА: оптимизироваать
        if (index != size - 1) {
            MemorySystem::CopyMem(data + index, data + index + 1, (size - index - 1) * elementSize);
        }
        size--;
    }

    /// @brief Изменяет размер контейнера, чтобы он содержал NewSize элементов инициализируемых по умолчанию.
    void Resize(u32 NewSize) {
        if(NewSize > capacity) {
            Reserve(NewSize);
        }
        size = NewSize;
    }

    /// @brief Изменяет размер контейнера, чтобы он содержал NewSize value элементов.
    /// @param NewSize новый размер контейнера
    void Resize(u32 NewSize, const T& value) {
        if(NewSize > capacity) {
            Reserve(NewSize);
            // ЗАДАЧА: изменить
            for (u32 i = size; i < NewSize; i++) {
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
        ShrinkToFit();
        data = nullptr;
        size = capacity = 0;
        return ptr;
    }

    DArray& SetAllocator(LinearAllocator* allocator) { 
        this->allocator = allocator;
        return *this;
    }
};
