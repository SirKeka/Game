#pragma once

#include "defines.hpp"
#include "containers/freelist.hpp"

class MAPI DynamicAllocator
{
private:
    struct DynamicAllocatorState {
        u64 TotalSize{};
        FreeList list{};
        void* FreelistBlock{nullptr};
        void* MemoryBlock{nullptr};
    }* state;
public:
    DynamicAllocator() : state(nullptr) {} // TotalSize(), list(), FreelistBlock(nullptr), MemoryBlock(nullptr) {}
    ~DynamicAllocator();

    //bool GetMemoryRequirement(u64 TotalSize, u64 &MemoryRequirement);
    static bool MemoryRequirement(u64 TotalSize, u64 &MemoryRequirement);
    bool GetMemoryRequirement(u64 TotalSize, u64 &MemoryRequirement);
    bool Create(u64 TotalSize, u64 &MemoryRequirement, void *memory);
    /// @brief Создает динамический распредлитель
    /// @param MemoryRequirement размер указателя на область памяти которым будет управлять данный распределитель
    /// @param memory указатель на область памяти где которой будет управлять данный распределитель
    /// @return true если создан успешно, иначе false
    bool Create(u64 MemoryRequirement, void *memory);
    bool Destroy();

    /// @brief Выделяет указанный объем памяти из предоставленного распределителя.
    /// @param size размер памяти, который нужно выделить
    /// @return Указатель на блок памяти заданного размера.
    void *Allocate(u64 size);

    /// @brief Выделяет указанный объем выровненной памяти из предоставленного распределителя.
    /// @param size Объем в байтах, который будет выделен.
    /// @param alignment Выравнивание в байтах. 0 = 256 байтам
    /// @return Выровненный, выделенный блок памяти, если эта операция не завершается неудачей, то 0.
    void* AllocateAligned(u64& size, u16 alignment);

    void* Realloc(void* block, u64 size, u64 NewSize);

    /// @brief Освобождает указанный блок памяти.
    /// @param block блок, который необходимо освободить. Должен быть выделен предоставленным распределителем.
    /// @param size размер блока.
    /// @param aligned указывает на то какой блок памяти удаляется выровненый или нет.
    /// @return True в случае успеха; в противном случае false.
    bool Free(void *block, u64 size, bool aligned = false);

    /// @brief Получает размер и выравнивание указанного блока памяти. Может завершиться ошибкой, если переданы недопустимые данные.
    /// @param block блок памяти.
    /// @param OutSize ссылка на переменную для хранения размера.
    /// @param OutAlignment ссылка на переменную для хранения выравнивания.
    /// @return True в случае успеха; в противном случае false.
    static bool GetSizeAlignment(void* block, u64& OutSize, u16& OutAlignment);

    u64 FreeSpace();
    /// @brief Получает объем общего пространства, изначально доступного в предоставленном распределителе.
    /// @return Общий объем пространства, изначально доступного в байтах.
    u64 TotalSpace();

    // Получает размер внутреннего заголовка выделения. Это действительно используется только для целей модульного тестирования.
    u64 HeaderSize();

    operator bool() const;
private:
    u64 GetFreeListRequirement(u64 TotalSize);
};
