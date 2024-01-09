#include "containers/darray.hpp"
#include "darray.hpp"
#include "core/logger.hpp"

template<typename T>
inline DArray<T>::DArray() 
:
mem(new MMemory()),
size(0),
capacity(2),
ptrValue(reinterpret_cast<T*>(mem->Allocate(sizeof(T) * capacity, MEMORY_TAG_DARRAY))) {}

template <typename T>
DArray<T>::DArray(u64 lenght, const T& value = T())
:
mem(new MMemory()),
size(lenght),
capacity(lenght),
ptrValue(reinterpret_cast<T*>(mem->Allocate(sizeof(T) * capacity, MEMORY_TAG_DARRAY)))
{
    for (u64 i = 0; i < lenght; i++) {
        ptrValue[i] = value;
    }
}

template <typename T>
DArray<T>::DArray(const DArray &arr) 
: 
mem(arr.mem),
size(arr.size),
capacity(arr.capacity),
ptrValue(arr.ptrValue) {}

template <typename T>
DArray<T>::DArray(DArray &&arr) : mem(arr.mem), size(arr.size), capacity(arr.capacity), ptrValue(arr.ptrValue)
{
    arr.mem = nullptr;
    arr.size = 0;
    arr.capacity = 0;
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
        void* ptrNew = (mem->Allocate(sizeof(T) * NewCap, MEMORY_TAG_DARRAY));
        ptrNew = mem->CopyMemory(ptrNew, reinterpret_cast<void*>(ptrValue), /*sizeof(T) * */capacity);
        mem->Free(ptrValue, sizeof(T) * capacity, MEMORY_TAG_DARRAY);
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
        ptrValue = reinterpret_cast<T*>(mem->CopyMemory(
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
        ptrValue = reinterpret_cast<T*>(mem->CopyMemory(
            reinterpret_cast<void*>(ptrValue + (index/* * sizeof(T)*/)),
            reinterpret_cast<void*>(ptrValue + (index + 1/* * sizeof(T)*/)),
            size - 1));
    }
    size--;
    
}

template <typename T>
void DArray<T>::Resize(u64 NewSize, const T& value = T())
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
