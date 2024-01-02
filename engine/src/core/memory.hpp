#pragma once

#include "defines.hpp"

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

class Memory
{
private:
    u64 Bytes;
    u64 TotalAllocated;
    u64 TaggedAllocations[MEMORY_TAG_MAX_TAGS];

    void* Start;
    
public:
    MAPI Memory();
    //Memory(const Memory&) = delete;
    //Memory& operator=(Memory&) = delete;
    ~Memory() /*noexcept*/;
    MAPI void ShutDown();
    /// @brief Функция выделяет память
    /// @param bytes размер выделяемой памяти в байтах
    /// @param tag название(тег) для каких нужд используется память
    /// @return указатель на выделенный блок памяти
    MAPI void* Allocate(u64 bytes, MemoryTag tag);

    /// @brief Функция освобождает память
    /// @param block указатель на блок памяти, который нужно освободить
    /// @param bytes размер блока памяти в байтах
    /// @param tag название(тег) для чего использовалась память
    MAPI void Free(void* block, u64 bytes, MemoryTag tag);

    /// @brief Функция зануляет выделенный блок памяти
    /// @param block указатель на блок памяти, который нужно обнулить
    /// @param bytes размер блока памяти в байтах
    /// @return указатель на нулевой блок памяти
    //MAPI void* ZeroMemory(void* block, u64 bytes);

    //MAPI void* CopyMemory(void* dest, const void* source, u64 bytes);

    //MAPI void* SetMemory(void* dest, i32 value, u64 bytes);

    MAPI const char* GetMemoryUsageStr();
    
};
