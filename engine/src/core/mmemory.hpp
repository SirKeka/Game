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
    MaterialInstance,
    Renderer,
    Game,
    Transform,
    Entity,
    EntityNode,
    Scene,
    HashTable,
    Resource,

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
    static struct MemoryState {
        u64 TotalAllocSize{}; // Общий размер памяти в байтах, используемый внутренним распределителем для этой системы.
        u64 TotalAllocated{};
        u64 TaggedAllocations[static_cast<u32>(MemoryTag::MaxTags)]{};
        u64 AllocCount{};
        u64 AllocatorMemoryRequirement{};
        DynamicAllocator allocator{};
        void* AllocatorBlock{nullptr};
    }* state;
    
public:
    constexpr MMemory() = default;
    // MMemory(const MMemory&) = delete;
    // MMemory& operator=(MMemory&) = delete;
    ~MMemory(); /*noexcept*/ //= default;
    /// @brief Инициализирует систему памяти.
    /// @param TotalAllocSize общий размер распределителя
    /// @return true если инициализация успеша, иначе false
    static bool Initialize(u64 TotalAllocSize);
    /// @brief Выключает систему памяти.
    static void Shutdown();
    /// @brief Функция выделяет память
    /// @param bytes размер выделяемой памяти в байтах
    /// @param tag название(тег) для каких нужд используется память
    /// @param def использовать стандартный new при выделении памяти. Поумолчанию false
    /// @return указатель на выделенный блок памяти
    static void* Allocate(u64 bytes, MemoryTag tag, bool nullify = false, bool def = false);

    template<typename T>
    static constexpr T* TAllocate(MemoryTag tag, u64 size = 1, bool nullify = false, bool def = false) {
        return (reinterpret_cast<T*>(Allocate(sizeof(T) * size, tag, nullify, def)));
    }
    /// @brief Функция освобождает память
    /// @param block указатель на блок памяти, который нужно освободить
    /// @param bytes размер блока памяти в байтах
    /// @param tag название(тег) для чего использовалась память
    /// @param def использовать стандартный new при выделении памяти. Если при выделении памяти использовался. Поумолчанию false
    static void Free(void* block, u64 bytes, MemoryTag tag, bool def = false);
    /// @brief Функция зануляет выделенный блок памяти
    /// @param block указатель на блок памяти, который нужно обнулить
    /// @param bytes размер блока памяти в байтах
    /// @return указатель на нулевой блок памяти
    static void* ZeroMem(void* block, u64 bytes);
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

    static char* GetMemoryUsageStr();

    //void* operator new(u64 size);
};
