/// @file vulkan_buffer.hpp
/// @author 
/// @brief Буфер данных, специфичный для Vulkan.
/// @version 1.0
/// @date 
/// 
/// @copyright 

#pragma once

#include "defines.hpp"
#include <vulkan/vulkan.h>
#include "containers/freelist.hpp"
class VulkanAPI;

class VulkanBuffer
{
public:
    u64 TotalSize;                  // Общий размер буфера.
    VkBuffer handle;                // Дескриптор внутреннего буфера.
    VkBufferUsageFlagBits usage;    // Флаги использования.
    bool IsLocked;                  // Указывает, заблокирована ли в данный момент память буфера.
    VkDeviceMemory memory;          // Память, используемая буфером.
    i32 MemoryIndex;                // Индекс памяти, используемой буфером.
    u32 MemoryPropertyFlags;        // Флаги свойств для памяти, используемой буфером.
    u64 FreeListMemoryRequirement;  // Объем памяти, необходимый для свободного списка.
    void* FreeListBlock;            // Блок памяти, используемый внутренним списком свободной памяти.
    FreeList BufferFreeList;        // Свободный список для отслеживания выделений.
    bool HasFreelist;
public:
    VulkanBuffer();
    ~VulkanBuffer();

    /// @brief Создает новый буфер Vulkan.
    /// @param VkAPI указатель на класс отрисовщика Vulkan.
    /// @param size размер буфера в байтах.
    /// @param usage флаги использования буфера (VkBufferUsageFlagBits)
    /// @param MemoryPropertyFlags Флаги свойства памяти.
    /// @param BindOnCreate указывает, должен ли этот буфер привязываться при создании.
    /// @param UseFreelist Указывает, следует ли использовать список свободных ресурсов. В противном случае функции выделения/освобождения также не следует использовать.
    /// @return true в случае успеха; в противном случае false.
    bool Create(VulkanAPI* VkAPI, u64 size, VkBufferUsageFlagBits usage, u32 MemoryPropertyFlags, bool BindOnCreate, bool UseFreelist);
    /// @brief Уничтожает заданный буфер.
    /// @param VkAPI указатель на класс отрисовщика Vulkan.
    void Destroy(VulkanAPI* VkAPI);
    /// @brief Изменяет размер данного буфера. В этом случае создается 
    /// новый внутренний буфер заданного размера, в него копируются данные 
    /// из старого буфера, затем старый буфер уничтожается. Это означает, 
    /// что эту операцию необходимо выполнять, когда буфер не используется.
    /// @param VkAPI указатель на класс отрисовщика Vulkan.
    /// @param NewSize новый размер буфера.
    /// @param queue очередь, используемая для операций изменения размера буфера.
    /// @param pool пул команд, используемый для внутреннего временного буфера команд.
    /// @return тrue в случае успеха; в противном случае false.
    bool Resize(VulkanAPI* VkAPI, u64 NewSize, VkQueue queue, VkCommandPool pool);
    /// @brief Связывает данный буфер для использования.
    /// @param VkAPI указатель на класс отрисовщика Vulkan.
    /// @param offset cмещение в байтах для привязки буфера.
    void Bind(VulkanAPI* VkAPI, u64 offset);
    /// @brief Блокирует (или сопоставляет) буферную память с временным расположением памяти хоста, 
    /// которое следует разблокировать перед выключением или уничтожением.
    /// @param VkAPI указатель на класс отрисовщика Vulkan.
    /// @param offset cмещение в байтах, по которому блокируется память.
    /// @param size объем памяти, которую необходимо заблокировать.
    /// @param flags флаги, которые будут использоваться в операции блокировки (VkMemoryMapFlags).
    /// @return указатель на блок памяти, сопоставленный с памятью буфера.
    void* LockMemory(VulkanAPI* VkAPI, u64 offset, u64 size, u32 flags);
    /// @brief Разблокирует (или не отображает) буферную память.
    /// @param VkAPI указатель на класс отрисовщика Vulkan.
    void UnlockMemory(VulkanAPI* VkAPI);
    /// @brief Выделяет пространство из буфера Vulkan. Предоставляет смещение, 
    /// при котором произошло выделение. Это потребуется для копирования и освобождения данных.
    /// @param size размер в байтах, который будет выделен.
    /// @param OutOffset ссылка, которая содержит смещение в байтах от начала буфера.
    /// @return true при успехе; иначе false.
    bool Allocate(u64 size, u64& OutOffset);
    /// @brief Освобождает место в буфере вулкана.
    /// @param size pазмер в байтах, который необходимо освободить.
    /// @param offset cмещение в байтах от начала буфера.
    /// @return true при успехе; иначе false.
    bool Free(u64 size, u64 offset);
    /// @brief Загружает диапазон данных в заданный буфер по заданному смещению. 
    /// Внутренне выполняет сопоставление, копирование и отмену сопоставления.
    /// @param VkAPI указатель на класс отрисовщика Vulkan.
    /// @param offset cмещение в байтах от начала буфера.
    /// @param size oбъем данных в байтах, которые будут загружены.
    /// @param flags флаги, которые будут использоваться в операции блокировки (VkMemoryMapFlags).
    /// @param data указатель на данные, которые необходимо загрузить.
    void LoadData(VulkanAPI* VkAPI, u64 offset, u64 size, u32 flags, const void* data);
    /// @brief Копирует диапазон данных из одного буфера в другой.
    /// @param VkAPI указатель на класс отрисовщика Vulkan.
    /// @param pool используемый пул команд.
    /// @deprecated @param fence ограждение, которое будет использоваться.
    /// @param queue очередь, которая будет использоваться.
    /// @param SourceOffset смещение исходного буфера.
    /// @param dest целевой буфер.
    /// @param DestOffset смещение целевого буфера.
    /// @param size размер копируемых данных в байтах.
    void CopyTo(VulkanAPI* VkAPI,
                VkCommandPool pool,
                VkFence fence,
                VkQueue queue,
                u64 SourceOffset,
                VkBuffer dest,
                u64 DestOffset,
                u64 size);
private:
    void CleanupFreelist();
};
