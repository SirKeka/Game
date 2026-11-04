#pragma once

#include "core/memory_system.h"
#include "memory/linear_allocator.h"

#include <utility>

enum class AllocatorType : u8 {
    Standart, Temp, Frame
};

template<typename T>
class MAPI DArray
{
// Функции
public:
    class Iterator;

    constexpr DArray(AllocatorType type = AllocatorType::Standart) : size(), capacity(), allocator(), data(), type(type) {}
    constexpr explicit DArray(u32 capacity, AllocatorType type = AllocatorType::Standart, LinearAllocator* allocator = nullptr) : size(), capacity(), allocator(), data(), type(type) { Reserve(capacity); }
    constexpr explicit DArray(LinearAllocator* allocator) : size(), capacity(), allocator(allocator),  data(), type(allocator ? AllocatorType::Frame : AllocatorType::Standart) {}
    constexpr DArray(u32 size, u32 capacity, void* array) : size(size), capacity(capacity ? capacity : size), allocator(), data((T*)array), type(AllocatorType::Standart) {}

    /// @brief Конструктор копирования
    /// @param other динамический массив из которого нужно копировать данные
    constexpr DArray(const DArray& other) : size(other.size), capacity(other.capacity), allocator(other.allocator), data(), type(other.type) {
        if (!capacity) {
            return;
        }

        data = (T*)(MemorySystem::Allocate(capacity * sizeof(T), Memory::DArray));
        
        for (u32 i = 0; i < size; i++) {
            data[i] = other.data[i];
        }
    }
    /// @brief Конструктор перемещения
    /// @param other динамический массив из которого нужно переместить данные
    constexpr DArray(DArray&& other) : size(other.size), capacity(other.capacity), allocator(other.allocator), data(other.data), type(other.type) {
        other.size = other.capacity = 0; other.data = nullptr; other.allocator = nullptr;
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
        if (data && type == AllocatorType::Standart) {
            Clear();
            MemorySystem::Free(data, capacity * sizeof(T), Memory::DArray);
        }
        data = darr.data;
        size = darr.size;
        capacity = darr.capacity;
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

    void Create(T* data, u32 size, AllocatorType type = AllocatorType::Standart) {
        if (this->data) {
            MERROR("Динамический массив уже был создан!");
            return;
        }
        
        this->data = data;
        this->size = capacity = size;
        this->type = type;
    }

    /// @brief Находит самый левый узел в дереве (минимальный ключ) и возвращает итератор на него.
    Iterator begin() {
        return Iterator(data);
    }

    const Iterator begin() const {
        return Iterator(data);
    }

    Iterator end() {
        return Iterator(data + size);
    }

    const Iterator end() const {
        return Iterator(data + size);
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

        u64 CurrentSize = capacity * sizeof(T);
        
        if (NewCap == 0) {
            capacity = capacity ? capacity * 2 : 2;
        } else {
            capacity = NewCap;
        }

        u64 NewSize = capacity * sizeof(T); 
        T* NewData = nullptr;
        using enum AllocatorType;
        switch (type) {
        case Temp:
            NewData = AllocateTemp<T>(NewSize, true);
            break;
        case Frame:
            if (CurrentSize == 0) {
                data = (T*)allocator->Allocate(NewSize);
            } else {
                allocator->Allocate(NewSize - CurrentSize);
            }
            return;
        
        default:
            NewData = (T*)MemorySystem::Allocate(NewSize, Memory::DArray, true);
            break;
        }

        if (!NewData) {
            // size = capacity = 0;
            MERROR("DArray::Reserve: Не удалось выделить память");
            return;
        }
        MemorySystem::CopyMem(NewData, data, CurrentSize);
        data = NewData;
    }

    /// @brief Уменьшает емкость(capacity) массива до размера(size)
    void ShrinkToFit() {
        if (capacity > size && type == AllocatorType::Standart) {
            MemorySystem::Free(data + size, (capacity - size) * sizeof(T), Memory::DArray);
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

    //-----------------------------------------------------------------------------------Емкость
    // Модифицирующие методы--------------------------------------------------------------------

    void Destroy() {
        if (!data) {
            return;
        }
        
        Clear();

        switch (type) {
        case AllocatorType::Temp: case AllocatorType::Frame:
            break;
        
        default:
            MemorySystem::Free(data, sizeof(T) * capacity, Memory::DArray);
            break;
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

        data[index].~T();
        
        // Если не последний элемент, вырезаем запись и копируем остальное внутрь. ЗАДАЧА: оптимизироваать
        if (index != size - 1) {
            MemorySystem::CopyMem(data + index, data + index + 1, (size - index - 1) * sizeof(T));
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
        Destroy();
        this->allocator = allocator;
        return *this;
    }

    // Итератор
    class Iterator {
    public:
        constexpr Iterator(T* ptr) : data(ptr) {}

        // Разыменование
        T& operator*() const { return *data; }
        T* operator->() const { return data; }

        // Префиксный инкремент
        Iterator& operator++() {
            ++data;
            return *this;
        }

        // Постфиксный инкремент
        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        // Декремент (для полноты, можно убрать, если не нужен)
        Iterator& operator--() {
            --data;
            return *this;
        }

        Iterator operator--(int) {
            Iterator tmp = *this;
            --(*this);
            return tmp;
        }

        // Арифметика итераторов
        Iterator operator+(u64 n) const {
            return Iterator(data + n);
        }

        Iterator operator-(u64 n) const {
            return Iterator(data - n);
        }

        Iterator& operator+=(u64 n) {
            data += n;
            return *this;
        }

        Iterator& operator-=(u64 n) {
            data -= n;
            return *this;
        }

        // Вычитание итераторов: даёт расстояние
        u64 operator-(const Iterator& other) const {
            return data - other.data;
        }

        // Сравнение
        bool operator==(const Iterator& other) const {
            return data == other.data;
        }

        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }

        bool operator<(const Iterator& other) const {
            return data < other.data;
        }

        bool operator<=(const Iterator& other) const {
            return data <= other.data;
        }

        bool operator>(const Iterator& other) const {
            return data > other.data;
        }
        
        bool operator>=(const Iterator& other) const {
            return data >= other.data;
        }

    private:
        T* data = nullptr;
    };

// private:
    // Переменные
    /// @brief Количество элементов в массиве
    u32 size;
    /// @brief Выделенная память под данное количество элементов
    u32 capacity;
    /// @brief Размер одного элемента в байтах
    // u32 elementSize;
    /// @brief Указатель на линейный(кадровый) аллокатор
    LinearAllocator* allocator;
    /// @brief Указатель на область памяти где хранятся элементы
    T* data;
    AllocatorType type;

    constexpr T* Alloc(u64 size) {
        using enum AllocatorType;
        switch (type) {
        case Temp:
            return AllocateTemp<T>(size, true);
        case Frame: 
            return (T*)allocator->Allocate(size);
        default:
            type = Standart;
            return (T*)MemorySystem::Allocate(size, Memory::DArray, true);
        }
    };
};
