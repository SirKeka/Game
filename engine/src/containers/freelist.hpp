#pragma once

#include "defines.hpp"

/// @brief Структура данных, которая будет использоваться вместе с распределителем 
/// для динамического распределения памяти. Отслеживает свободные области памяти.
class MAPI FreeList
{
private:
    u64 TotalSize;
    u64 MaxEntries;
    struct FreelistNode* head;
    struct FreelistNode* nodes;

public:
    FreeList() : TotalSize(), MaxEntries(), head(nullptr), nodes(nullptr) {}
    /// @brief Создает новый свободный список или получает требуемую для него память. 
    /// Вызов дважды; один раз передача 0 в память для получения требуемой памяти, 
    /// а второй раз передача выделенного блока в память.
    /// @param TotalSize общий размер в байтах, который должен отслеживать список свободных мест.
    /// @param MemoryRequirement ссылка на переменную, которая хранит значение требуемой памяти для самого списка свободных мест.
    /// @param memory nullptr или предварительно выделенный блок памяти для использования списком свободных.
    //FreeList(u64 TotalSize, u64& MemoryRequirement, void* memory);
    /// @brief Уничтожает список
    ~FreeList();

    /// @brief Присваивает значению MemoryRequirement новое
    /// @param TotalSize общий размер в байтах, который должен отслеживать список свободных мест.
    /// @param MemoryRequirement ссылка на переменную, которая хранит значение требуемой памяти для самого списка свободных мест.
    void GetMemoryRequirement(u64 TotalSize, u64& MemoryRequirement);

    /// @brief Создает новый свободный список. 
    /// @param memory nullptr или предварительно выделенный блок памяти для использования списком свободных мест.
    void Create(u64 TotalSize, void* memory);
    /// @brief Пытается найти свободный блок памяти заданного размера.
    /// @param size размер для выделения.
    /// @param OutOffset указатель для хранения смещения выделенной памяти.
    /// @return истинно(true), если блок памяти был найден и выделен; в противном случае ложь(false).
    bool AllocateBlock(u64 size, u64& OutOffset);
    /// @brief Пытается освободить блок памяти по заданному смещению и заданному размеру. Может произойти сбой, если переданы неверные данные.
    /// @param size размер, который нужно освободить.
    /// @param offset смещение
    /// @return true в случае успеха; в противном случае ложь. Значение False следует рассматривать как ошибку.
    bool FreeBlock(u64 size, u64 offset);
    /// @brief Очищает свободный список.
    void Clear();
    /// @brief Возвращает объем свободного места в этом списке. ПРИМЕЧАНИЕ: 
    /// Поскольку при этом необходимо выполнить итерацию всего внутреннего списка,
    /// эта операция может оказаться дорогостоящей. Используйте экономно.
    /// @return объем свободного места в байтах.
    u64 FreeSpace();

    operator bool() const;
private:
    struct FreelistNode* GetNode();
    void ReturnNode(struct FreelistNode* node);
};
