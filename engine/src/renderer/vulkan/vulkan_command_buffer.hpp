/* Команды в Vulkan, такие как операции рисования и операции перемещения в памяти, 
 * не выполняются непосредственно во время вызова соответствующих функций. 
 * Все необходимые операции должны быть записаны в буфер. Это удобно тем, 
 * что сложный процесс настройки команд может быть выполнен заранее и в нескольких потоках. 
 * В основном цикле вам остается лишь указать Vulkan, чтобы он запустил команды.
 */
#pragma once

#include "vulkan_api.hpp"

void VulkanCommandBufferAllocate(
    VulkanAPI* VkAPI,
    VkCommandPool pool,
    bool IsPrimary,
    VulkanCommandBuffer* OutCommandBuffer
);

void VulkanCommandBufferFree(
    VulkanAPI* VkAPI,
    VkCommandPool pool,
    VulkanCommandBuffer* CommandBuffer
);

void VulkanCommandBufferBegin(
    VulkanCommandBuffer* CommandBuffer,
    bool IsSingleUse,
    bool IsRenderpassContinue,
    bool IsSimultaneousUse
);

void VulkanCommandBufferEnd(VulkanCommandBuffer* CommandBuffer);

void VulkanCommandBufferUpdateSubmitted(VulkanCommandBuffer* CommandBuffer);

void VulkanCommandBufferReset(VulkanCommandBuffer* CommandBuffer);

// Выделяет и начинает запись в OutCommandBuffer.
void VulkanCommandBufferAllocateAndBeginSingleUse(
    VulkanAPI* VkAPI,
    VkCommandPool pool,
    VulkanCommandBuffer* OutCommandBuffer
);

// Завершает запись, передает и ожидает операции очереди и освобождает предоставленный командный буфер.
void VulkanCommandBufferEndSingleUse(
    VulkanAPI* VkAPI,
    VkCommandPool pool,
    VulkanCommandBuffer* CommandBuffer,
    VkQueue queue);