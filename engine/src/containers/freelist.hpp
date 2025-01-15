#pragma once

#include "defines.hpp"

struct FreelistNode;

/// @brief Структура данных, которая будет использоваться вместе с распределителем 
/// для динамического распределения памяти. Отслеживает свободные области памяти.
class MAPI FreeList
{
private:
    struct FreeListState {
        u64 TotalSize{};
        u64 MaxNodes{};
        FreelistNode* head{};
        FreelistNode* nodes{};
    }* state;

public:
    constexpr FreeList() : state(nullptr) {}
    // constexpr FreeList(u64 TotalSize, void* memory);
    /// @brief Создает новый свободный список или получает требуемую для него память. 
    /// Вызов дважды; один раз передача 0 в память для получения требуемой памяти, 
    /// а второй раз передача выделенного блока в память.
    /// @param TotalSize общий размер в байтах, который должен отслеживать список свободных мест.
    /// @param MemoryRequirement ссылка на переменную, которая хранит значение требуемой памяти для самого списка свободных мест.
    /// @param memory nullptr или предварительно выделенный блок памяти для использования списком свободных.
    //FreeList(u64 TotalSize, u64& MemoryRequirement, void* memory);
    /// @brief Уничтожает список
    ~FreeList();

    static u64 GetMemoryRequirement(u64 TotalSize);

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
    bool AllocateBlock(u64 size, u64 &OutOffset);

    /// @brief Пытается перераспределить блок памяти.
    /// @param NewSize новый размер блока.
    /// @param BlockOffset смещение блока.
    /// @return true если успешно, иначе false.
    bool ReallocateBlock(u64 size, u64 NewSize, u64& BlockOffset);

    /// @brief Пытается освободить блок памяти по заданному смещению и заданному размеру. Может произойти сбой, если переданы неверные данные.
    /// @param size размер, который нужно освободить.
    /// @param offset смещение
    /// @return true в случае успеха; в противном случае ложь. Значение False следует рассматривать как ошибку.
    bool FreeBlock(u64 size, u64 offset);

    /// @brief Пытается изменить размер предоставленного свободного списка до заданного размера. 
    /// Внутренние данные копируются в новый блок памяти. После этого вызова старый блок должен быть освобожден.
    /// ПРИМЕЧАНИЕ: Новый размер должен быть _больше_ существующего размера данного списка.
    /// ПРИМЕЧАНИЕ: Перед вызовом нужно запросить требуемую память через функцию GetMemoryRequirement(u64 TotalSize, u64& MemoryRequirement).
    /// @param NewMemory новый блок памяти состояний.
    /// @param NewSize новый размер должен быть больше размера предоставленного списка.
    /// @param OutOldMemory указатель на старый блок памяти, чтобы его можно было освободить после этого вызова.
    /// @return true в случае успеха; иначе false.
    bool Resize(void* NewMemory, u64 NewSize, void** OutOldMemory);

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
    /// @brief Присоединяет новый узел к существующием
    void AttachNode(FreelistNode* node, FreelistNode* previous, FreelistNode* NewNode);
    bool Allocate(FreelistNode *&node, u64 size, u64 &OutOffset, FreelistNode *&previous);
    bool Free(FreelistNode *&node, u64 offset, u64 size, FreelistNode *&previous);

    // ЗАДАЧА: временно для отладки
    void Verification(FreelistNode* CheckNode);
};