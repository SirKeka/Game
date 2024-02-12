#include "defines.hpp"
#include "core/mmemory.hpp"
#include "core/logger.hpp"

template<typename T>
class MAPI DArray
{
// Переменные
public:
    u64 size;
    u64 capacity;

private:
    // MMemory* mem;
    T* ptrValue;

// Функции
public:
    DArray();
    DArray(u64 lenght, const T& value = T());
    ~DArray();
    // Конструктор копирования
    DArray(const DArray& arr);
    // Конструктор перемещения
    DArray(DArray&& arr);

    // Доступ к элементу

    T& operator [] (u64 index);
    const T& operator [] (u64 index) const;

    // Емкость

    /// @brief Увеличьте емкость вектора (общее количество элементов, 
    /// которые вектор может содержать без необходимости перераспределения) до значения, 
    /// большего или равного NewCap. Если значение NewCap больше текущей capacity(емкости), 
    /// выделяется новое хранилище, в противном случае функция ничего не делает.
    void Reserve(const u64& NewCap);
    /// @brief 
    /// @return количество элементов контейнера
    u64 Lenght();

    // Модифицирующие методы

    /// @brief Добавляет заданное значение элемента в конец контейнера.
    /// @param value элемент который нужно поместить в конец контейнера.
    void PushBack(const T& value);
    /// @brief Удаляет последний элемент контейнера.
    void PopBack();

    /// @brief Вставляет value в позицию index.
    /// @param value элемент, который нужно ставить.
    /// @param index индекс элемента в контейнере.
    void Insert(const T& value, u64 index);
    /// @brief Удаляет выбранный элемент.
    void PopAt(u64 index);

    /// @brief Изменяет размер контейнера, чтобы он содержал NewSize элементы, ничего не делает, если NewSize == size().
    /// @param NewSize новый размер контейнера
    void Resize(u64 NewSize, const T& value = T());
};

template<typename T>
inline DArray<T>::DArray()
:
size(0),
capacity(0),
//mem(new MMemory()),
ptrValue(nullptr/*reinterpret_cast<T*>(MMemory::Allocate(sizeof(T) * capacity, MEMORY_TAG_DARRAY))*/) {}

template <typename T>
DArray<T>::DArray(u64 lenght, const T &value)
/*:
size(lenght),
capacity(lenght),
//mem(new MMemory()),
ptrValue(reinterpret_cast<T*>(MMemory::Allocate(sizeof(T) * capacity, MEMORY_TAG_DARRAY)))*/
{
    if(lenght > 0) {
        this->size = lenght;
        this->capacity = lenght;
        ptrValue = reinterpret_cast<T*>(MMemory::Allocate(sizeof(T) * capacity, MEMORY_TAG_DARRAY));
        for (u64 i = 0; i < lenght; i++) {
            ptrValue[i] = value;
        }
    }
}

template <typename T>
DArray<T>::~DArray()
{
    MMemory::Free(reinterpret_cast<void*>(ptrValue), sizeof(T) * capacity, MEMORY_TAG_DARRAY);
    ptrValue = nullptr;
    //delete mem;
}

template <typename T>
DArray<T>::DArray(const DArray &arr) 
: 
size(arr.size),
capacity(arr.capacity),
//mem(arr.mem),
ptrValue(arr.ptrValue) {}

template <typename T>
DArray<T>::DArray(DArray &&arr) :  size(arr.size), capacity(arr.capacity), /*mem(arr.mem),*/ ptrValue(arr.ptrValue)
{
    arr.size = 0;
    arr.capacity = 0;
    //arr.mem = nullptr;
    arr.ptrValue = nullptr;
}

template <typename T>
T &DArray<T>::operator[](u64 index)
{
    if(index < 0 || index >= size) MERROR("Индекс за пределами этого массива! Длина: %i, индекс: %index", size, index);
    return ptrValue[index];
}

template <typename T>
const T &DArray<T>::operator[](u64 index) const
{
    if(index < 0 || index >= size) MERROR("Индекс за пределами этого массива! Длина: %i, индекс: %index", size, index);
    return ptrValue[index];
}

template <typename T>
void DArray<T>::Reserve(const u64 &NewCap)
{
    // TODO: добавить std::move()
    if (NewCap > capacity) {
        void* ptrNew = (MMemory::Allocate(sizeof(T) * NewCap, MEMORY_TAG_DARRAY));
        ptrNew = MMemory::CopyMemory(ptrNew, reinterpret_cast<void*>(ptrValue), /*sizeof(T) * */capacity);
        MMemory::Free(ptrValue, sizeof(T) * capacity, MEMORY_TAG_DARRAY);
        ptrValue = reinterpret_cast<T*> (ptrNew);
        capacity = NewCap;
    }
}

template <typename T>
u64 DArray<T>::Lenght()
{
    return  this->size;
}

template <typename T>
void DArray<T>::PushBack(const T &value)
{
    if(size == 0) Reserve(2);
    if(size > capacity) Reserve(capacity * 2);
    ptrValue[size] = value;
    size++;
}
template <typename T>
void DArray<T>::PopBack()
{
    if(size > 0) size--;
}

template <typename T>
void DArray<T>::Insert(const T &value, u64 index)
{
    if (index >= size) MERROR("Индекс за пределами этого массива! Длина: %i, индекс: %index", size, index);

    if(index > size) Resize(size + 1, value);

    // Если не последний элемент, скопируйте остальное наружу.
    if (index != size - 1) {
        ptrValue = reinterpret_cast<T*>(MMemory::CopyMemory(
            reinterpret_cast<void*>(ptrValue + (index/* * sizeof(T)*/)),
            reinterpret_cast<void*>(ptrValue + (index + 1/* * sizeof(T)*/)),
            size - 1));
    }
    size++;
}

template <typename T>
void DArray<T>::PopAt(u64 index)
{
    if (index >= size) MERROR("Индекс за пределами этого массива! Длина: %i, индекс: %index", size, index);

    // Если не последний элемент, вырезаем запись и копируем остальное внутрь. TODO: оптимизироваать
    if (index != size - 1) {
        ptrValue = reinterpret_cast<T*>(MMemory::CopyMemory(
            reinterpret_cast<void*>(ptrValue + (index/* * sizeof(T)*/)),
            reinterpret_cast<void*>(ptrValue + (index + 1/* * sizeof(T)*/)),
            size - 1));
    }
    size--;
    
}

template <typename T>
void DArray<T>::Resize(u64 NewSize, const T& value)
{
    if(NewSize > capacity) {
        Reserve(NewSize);
        // TODO: изменить
        for (u64 i = size; i < NewSize; i++) {
            ptrValue[i] = value;
        }
        
    }
    size = NewSize;
}