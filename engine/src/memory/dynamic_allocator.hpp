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
    /// @param alignment Выравнивание в байтах.
    /// @return Выровненный, выделенный блок памяти, если эта операция не завершается неудачей, то 0.
    void* AllocateAligned(u64 size, u16 alignment);
    /// @brief Освобождает указанный блок памяти.
    /// @param block Блок, который необходимо освободить. Должен быть выделен предоставленным распределителем.
    /// @param size Размер блока.
    /// @return True в случае успеха; в противном случае false.
    bool Free(void *block, u64 size);
    /// @brief Освобождает указанный блок выровненной памяти. Технически то же самое, что и вызов DynamicAllocator::Free, но здесь для согласованности API. Размер не требуется.
    /// @param block Блок, который нужно освободить. Должен быть выделен предоставленным распределителем.
    /// @return True в случае успеха; в противном случае false.
    bool FreeAligned(void* block);
    /// @brief Получает размер и выравнивание указанного блока памяти. Может завершиться ошибкой, если переданы недопустимые данные.
    /// @param block Блок памяти.
    /// @param OutSize Указатель для хранения размера.
    /// @param OutAlignment Указатель для хранения выравнивания.
    /// @return True в случае успеха; в противном случае false.
    static bool GetSizeAlignment(void* block, u64& OutSize, u16& OutAlignment);
    u64 FreeSpace();
    /// @brief Получает объем общего пространства, изначально доступного в предоставленном распределителе.
    /// @return Общий объем пространства, изначально доступного в байтах.
    u64 TotalSpace();

    operator bool() const;
private:
    u64 GetFreeListRequirement(u64 TotalSize);
};
