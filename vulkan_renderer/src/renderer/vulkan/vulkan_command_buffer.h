/* Команды в Vulkan, такие как операции рисования и операции перемещения в памяти, 
 * не выполняются непосредственно во время вызова соответствующих функций. 
 * Все необходимые операции должны быть записаны в буфер. Это удобно тем, 
 * что сложный процесс настройки команд может быть выполнен заранее и в нескольких потоках. 
 * В основном цикле вам остается лишь указать Vulkan, чтобы он запустил команды.
 */
#pragma once
#include "defines.h"
#include <vulkan/vulkan.h>

class VulkanAPI;

enum class VulkanCommandBufferState 
{
    READY,
    RECORDING,
    IN_RENDER_PASS,
    RECORDING_ENDED,
    SUBMITTED,
    NOT_ALLOCATED
};

struct VulkanCommandBuffer 
{
    VkCommandBuffer handle;

    // Состояние буфера команд.
    VulkanCommandBufferState state;

    void Allocate(VulkanAPI* VkAPI, VkCommandPool pool, bool IsPrimary);
    void Free(VulkanAPI* VkAPI, VkCommandPool pool);
    void Begin(bool IsSingleUse, bool IsRenderpassContinue, bool IsSimultaneousUse);
    void End();
    void UpdateSubmitted() { state = VulkanCommandBufferState::SUBMITTED; }
    void Reset();

    /// @brief Выделяет и начинает запись в командный буфер.
    /// @param VkAPI 
    /// @param pool 
    void AllocateAndBeginSingleUse(VulkanAPI* VkAPI, VkCommandPool pool);

    /// @brief Завершает запись, передает и ожидает операции очереди и освобождает предоставленный командный буфер.
    /// @param VkAPI 
    /// @param pool 
    /// @param queue 
    void EndSingleUse(VulkanAPI* VkAPI, VkCommandPool pool, VkQueue queue);
};
