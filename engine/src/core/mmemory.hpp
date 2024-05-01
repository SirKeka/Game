/// @file mmemory.hpp
/// @author 
/// @brief Этот файл содержит структуры и функции системы памяти.
/// Он отвечает за взаимодействие памяти с уровнем платформы, 
/// такое как выделение/освобождение и маркировка выделенной памяти.
/// @note Обратите внимание, что на это, скорее всего, будут полагаться только основные системы, 
/// поскольку элементы, использующие распределения напрямую, будут использовать распределители по мере их добавления в систему.
/// @version 1.0
/// @date 
/// 
/// @copyright 
/// 
#pragma once

#include "defines.hpp"
#include "core/logger.hpp"
#include "containers/mstring.hpp"
#include "memory/dynamic_allocator.hpp"


/// @brief Теги, указывающие на использование выделенной памяти в этой системе.
enum class MemoryTag 
{
    // Для временного использования. Должно быть присвоено одно из следующих значений или создан новый тег.
    Unknown,
    Array,
    LinearAllocator,
    DArray,
    Dict,
    RingQueue,
    BST,
    String,
    Application,
    Job,
    Texture,
    MAaterialInstance,
    Renderer,
    Game,
    Transform,
    Entity,
    EntityNode,
    Scene,

    MaxTags
};

class MAPI MMemory
{
private:
    /*struct SharPtr
    {
        void* ptr;
        u16 count;
        // u64 Bytes;
    };*/

    //static DArray<SharPtr> ptr;
    static struct State {
        u64 TotalAllocSize; // Общий размер памяти в байтах, используемый внутренним распределителем для этой системы.
        u64 TotalAllocated;
        u64 TaggedAllocations[static_cast<u32>(MemoryTag::MaxTags)];
        u64 AllocCount;
        u64 AllocatorMemoryRequirement;
        //DynamicAllocator allocator;
        //void* AllocatorBlock;
    } state;
    
public:
    // MMemory() = default;
    // MMemory(const MMemory&) = delete;
    // MMemory& operator=(MMemory&) = delete;
    ~MMemory(); /*noexcept*/ //= default;
    bool Initialize(u64 TotalAllocSize);
    void Shutdown();
    /// @brief Функция выделяет память
    /// @param bytes размер выделяемой памяти в байтах
    /// @param tag название(тег) для каких нужд используется память
    /// @return указатель на выделенный блок памяти
    static void* Allocate(u64 bytes, MemoryTag tag);

    template<typename T>
    static T* TAllocate(u64 size, MemoryTag tag) {
        if (tag == MemoryTag::Unknown) {
            MWARN("allocate вызывается с использованием MemoryTag::Unknown. Переклассифицировать это распределение.");
        }

        state.TotalAllocated += size * sizeof(T);
        state.TaggedAllocations[static_cast<u32>(tag)] += size * sizeof(T);

        T* ptrRawMem = new T[size]();

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
            if (tag == MemoryTag::Unknown) {
                MWARN("free вызывается с использованием MemoryTag::Unknown. Переклассифицировать это распределение.");
            }

            state.TotalAllocated -= sizeof(T) * factor;
            state.TaggedAllocations[static_cast<u32>(tag)] -= sizeof(T) * factor;

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
};
