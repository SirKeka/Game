#pragma once

#include "defines.hpp"
#include "core/mstring.hpp"

enum MemoryTag 
{
     // Для временного использования. Должно быть присвоено одно из следующих значений или создан новый тег.
    MEMORY_TAG_UNKNOWN,
    MEMORY_TAG_ARRAY,
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

class MMemory
{
private:
    // u64 Bytes{0};
    static u64 TotalAllocated;
    static u64 TaggedAllocations[MEMORY_TAG_MAX_TAGS];

    [[maybe_unused]] void* Start = nullptr;
    
public:
    MAPI MMemory() = default;
    //MMemory(const MMemory&) = delete;
    //MMemory& operator=(MMemory&) = delete;
    ~MMemory(); /*noexcept*/ //= default;
    MAPI void ShutDown();
    /// @brief Функция выделяет память
    /// @param bytes размер выделяемой памяти в байтах
    /// @param tag название(тег) для каких нужд используется память
    /// @return указатель на выделенный блок памяти
    static MAPI void* Allocate(u64 bytes, MemoryTag tag);

    /// @brief Функция освобождает память
    /// @param block указатель на блок памяти, который нужно освободить
    /// @param bytes размер блока памяти в байтах
    /// @param tag название(тег) для чего использовалась память
    static MAPI void Free(void* block, u64 bytes, MemoryTag tag);

    /// @brief Функция зануляет выделенный блок памяти
    /// @param block указатель на блок памяти, который нужно обнулить
    /// @param bytes размер блока памяти в байтах
    /// @return указатель на нулевой блок памяти
    //MAPI void* ZeroMemory(void* block, u64 bytes);

    /// @brief Функция копирует массив байтов из source указателя в dest
    /// @param dest указатель куда комируется массив байтов
    /// @param source указатель из которого копируется массив байтов
    /// @param bytes количество байт памяти которое копируется
    /// @return указатель
    static MAPI void CopyMemory(void* dest, const void* source, u64 bytes);

    //MAPI void* SetMemory(void* dest, i32 value, u64 bytes);

    template<class U, class... Args>
    void Construct (U* ptr, Args && ...args);

    static MAPI MString GetMemoryUsageStr();
    
};
