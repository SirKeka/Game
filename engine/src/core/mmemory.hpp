#pragma once

#include "defines.hpp"
#include "memory/linear_allocator.hpp"
#include "core/logger.hpp"
#include "containers/mstring.hpp"
//#include "containers/darray.hpp"


enum MemoryTag 
{
     // Для временного использования. Должно быть присвоено одно из следующих значений или создан новый тег.
    MEMORY_TAG_UNKNOWN,
    MEMORY_TAG_ARRAY,
    MEMORY_TAG_LINEAR_ALLOCATOR,
    MEMORY_TAG_DARRAY,
    MEMORY_TAG_DICT,
    MEMORY_TAG_RING_QUEUE,
    MEMORY_TAG_BST,
    MEMORY_TAG_STRING,
    MEMORY_TAG_APPLICATION,
    MEMORY_TAG_JOB,
    MEMORY_TAG_TEXTURE,
    MEMORY_TAG_MATERIAL_INSTANCE,
    MEMORY_TAG_RENDERER,
    MEMORY_TAG_GAME,
    MEMORY_TAG_TRANSFORM,
    MEMORY_TAG_ENTITY,
    MEMORY_TAG_ENTITY_NODE,
    MEMORY_TAG_SCENE,

    MEMORY_TAG_MAX_TAGS
};

class MAPI MMemory
{
using LinearAllocator = WrapLinearAllocator<MMemory>;

private:
    /*struct SharPtr
    {
        void* ptr;
        u16 count;
        // u64 Bytes;
    };*/

    //static DArray<SharPtr> ptr;
    
    static u64 TotalAllocated;
    static u64 TaggedAllocations[MEMORY_TAG_MAX_TAGS];
    static u64 AllocCount;
    
public:
    MMemory() = default;
    //MMemory(const MMemory&) = delete;
    //MMemory& operator=(MMemory&) = delete;
    ~MMemory(); /*noexcept*/ //= default;
    void Shutdown();
    /// @brief Функция выделяет память
    /// @param bytes размер выделяемой памяти в байтах
    /// @param tag название(тег) для каких нужд используется память
    /// @return указатель на выделенный блок памяти
    static void* Allocate(u64 bytes, MemoryTag tag);

    template<typename T>
    static T* TAllocate(u64 size, MemoryTag tag) {
        if (tag == MEMORY_TAG_UNKNOWN) {
            MWARN("allocate вызывается с использованием MEMORY_TAG_UNKNOWN. Переклассифицировать это распределение.");
        }

        TotalAllocated += size * sizeof(T);
        TaggedAllocations[tag] += size * sizeof(T);

        T* ptrRawMem = new T[size];

        return ptrRawMem;
    }

    /// @brief Функция освобождает память
    /// @param block указатель на блок памяти, который нужно освободить
    /// @param bytes размер блока памяти в байтах
    /// @param tag название(тег) для чего использовалась память
    static void Free(void* block, u64 bytes, MemoryTag tag);

    template<typename T>
    /// @brief Функция освобождает память
    /// @param block указатель на блок памяти, который нужно освободить
    /// @param factor количество элементов Т в массиве блока памяти
    /// @param tag название(тег) для чего использовалась память
    static void TFree(T* block, u64 factor, MemoryTag tag) {
        if (block) {
            if (tag == MEMORY_TAG_UNKNOWN) {
                MWARN("free вызывается с использованием MEMORY_TAG_UNKNOWN. Переклассифицировать это распределение.");
            }

            TotalAllocated -= sizeof(T) * factor;
            TaggedAllocations[tag] -= sizeof(T) * factor;

            delete[] block;
        }
    }

    /// @brief Функция зануляет выделенный блок памяти
    /// @param block указатель на блок памяти, который нужно обнулить
    /// @param bytes размер блока памяти в байтах
    /// @return указатель на нулевой блок памяти
    static void* ZeroMem(void* block, u64 bytes);

    template<typename T>
    static void* TZeroMem(T* block, u64 bytes){
        return ZeroMem(reinterpret_cast<void*>(block), bytes);
    }

    /// @brief Функция копирует массив байтов из source указателя в dest
    /// @param dest указатель куда комируется массив байтов
    /// @param source указатель из которого копируется массив байтов
    /// @param bytes количество байт памяти которое копируется
    /// @return указатель
    static void CopyMem(void* dest, const void* source, u64 bytes);

    /// @brief 
    /// @param ptr значение указателя которое присваевается новому указателю
    /// @return 
    //MAPI static void* ptrMove(void* ptr);

    static void* SetMemory(void* dest, i32 value, u64 bytes);

    static u64 GetMemoryAllocCount();

    template<class U, class... Args>
    void Construct (U* ptr, Args && ...args);

    static MString GetMemoryUsageStr();
    
    void* operator new(u64 size);
    //void operator delete(void* ptr);
};

/*template<typename T>
MAPI T * MMemory::TZeroMemory(T * block, u64 bytes)
{
return memset(block, 0, bytes);
}*/