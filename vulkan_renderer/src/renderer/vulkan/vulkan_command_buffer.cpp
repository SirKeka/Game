#include "vulkan_command_buffer.h"
#include "vulkan_api.h"

//#include "core/mmemory.hpp"

void VulkanCommandBufferAllocate(VulkanAPI *VkAPI, VkCommandPool pool, bool IsPrimary, VulkanCommandBuffer &OutCommandBuffer)
{
    // MMemory::ZeroMem(OutCommandBuffer, sizeof(VulkanCommandBuffer));

    VkCommandBufferAllocateInfo AllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = pool,
        .level = IsPrimary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY,
        .commandBufferCount = 1
    };

    OutCommandBuffer.state = COMMAND_BUFFER_STATE_NOT_ALLOCATED;
    VK_CHECK(vkAllocateCommandBuffers(VkAPI->device.LogicalDevice, &AllocateInfo, &OutCommandBuffer.handle));
    OutCommandBuffer.state = COMMAND_BUFFER_STATE_READY;
}

void VulkanCommandBufferFree(VulkanAPI *VkAPI, VkCommandPool pool, VulkanCommandBuffer &CommandBuffer)
{
    vkFreeCommandBuffers(VkAPI->device.LogicalDevice, pool, 1, &CommandBuffer.handle);

    CommandBuffer.handle = nullptr;
    CommandBuffer.state = COMMAND_BUFFER_STATE_NOT_ALLOCATED;
}

void VulkanCommandBufferBegin(VulkanCommandBuffer &CommandBuffer, bool IsSingleUse, bool IsRenderpassContinue, bool IsSimultaneousUse)
{
    VkCommandBufferBeginInfo BeginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0
    };
    if (IsSingleUse) {
        BeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;     // указывает, что каждая запись командного буфера будет отправлена только один раз, и командный буфер будет сбрасываться и записываться снова между каждой отправкой.
    }
    if (IsRenderpassContinue) {
        BeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;// указывает, что дополнительный командный буфер считается полностью находящимся внутри прохода рендеринга. Если это основной командный буфер, то этот бит игнорируется.
    }
    if (IsSimultaneousUse) {
        BeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;    // указывает, что буфер команд может быть повторно отправлен в любую очередь того же семейства очередей, пока он находится в состоянии ожидания, и записан в несколько первичных буферов команд.
    }

    VK_CHECK(vkBeginCommandBuffer(CommandBuffer.handle, &BeginInfo));
    CommandBuffer.state = COMMAND_BUFFER_STATE_RECORDING;
}

void VulkanCommandBufferEnd(VulkanCommandBuffer &CommandBuffer)
{
    VK_CHECK(vkEndCommandBuffer(CommandBuffer.handle));
    CommandBuffer.state = COMMAND_BUFFER_STATE_RECORDING_ENDED;
}

void VulkanCommandBufferAllocateAndBeginSingleUse(VulkanAPI *VkAPI, VkCommandPool pool, VulkanCommandBuffer &OutCommandBuffer)
{
    VulkanCommandBufferAllocate(VkAPI, pool, true, OutCommandBuffer);
    VulkanCommandBufferBegin(OutCommandBuffer, true, false, false);
}

void VulkanCommandBufferEndSingleUse(VulkanAPI *VkAPI, VkCommandPool pool, VulkanCommandBuffer &CommandBuffer, VkQueue queue)
{
    // Завершить буфер команд.
    VulkanCommandBufferEnd(CommandBuffer);

    // Отправить очередь
    VkSubmitInfo SubmitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &CommandBuffer.handle;

    VkResult result = vkQueueSubmit(queue, 1, &SubmitInfo, 0);
    VK_CHECK(result);

    // Подождите, пока это закончится
    result = vkQueueWaitIdle(queue);
    VK_CHECK(result);

    // Освободите буфер команд.v
    VulkanCommandBufferFree(VkAPI, pool, CommandBuffer);
}
