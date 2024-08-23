#include "vulkan_command_buffer.hpp"
#include "vulkan_api.hpp"

//#include "core/mmemory.hpp"

void VulkanCommandBufferAllocate(VulkanAPI *VkAPI, VkCommandPool pool, bool IsPrimary, VulkanCommandBuffer *OutCommandBuffer)
{
    // MMemory::ZeroMem(OutCommandBuffer, sizeof(VulkanCommandBuffer));

    VkCommandBufferAllocateInfo AllocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    AllocateInfo.commandPool = pool;
    AllocateInfo.level = IsPrimary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    AllocateInfo.commandBufferCount = 1;
    AllocateInfo.pNext = 0;

    OutCommandBuffer->state = COMMAND_BUFFER_STATE_NOT_ALLOCATED;
    VK_CHECK(vkAllocateCommandBuffers(
        VkAPI->Device.LogicalDevice,
        &AllocateInfo,
        &OutCommandBuffer->handle));
    OutCommandBuffer->state = COMMAND_BUFFER_STATE_READY;
}

void VulkanCommandBufferFree(VulkanAPI *VkAPI, VkCommandPool pool, VulkanCommandBuffer *CommandBuffer)
{
    vkFreeCommandBuffers(
        VkAPI->Device.LogicalDevice,
        pool,
        1,
        &CommandBuffer->handle);

    CommandBuffer->handle = 0;
    CommandBuffer->state = COMMAND_BUFFER_STATE_NOT_ALLOCATED;
}

void VulkanCommandBufferBegin(VulkanCommandBuffer *CommandBuffer, bool IsSingleUse, bool IsRenderpassContinue, bool IsSimultaneousUse)
{
    VkCommandBufferBeginInfo BeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    BeginInfo.flags = 0;
    if (IsSingleUse) {
        BeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;     // указывает, что каждая запись командного буфера будет отправлена только один раз, и командный буфер будет сбрасываться и записываться снова между каждой отправкой.
    }
    if (IsRenderpassContinue) {
        BeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;// указывает, что дополнительный командный буфер считается полностью находящимся внутри прохода рендеринга. Если это основной командный буфер, то этот бит игнорируется.
    }
    if (IsSimultaneousUse) {
        BeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;    // указывает, что буфер команд может быть повторно отправлен в любую очередь того же семейства очередей, пока он находится в состоянии ожидания, и записан в несколько первичных буферов команд.
    }

    VK_CHECK(vkBeginCommandBuffer(CommandBuffer->handle, &BeginInfo));
    CommandBuffer->state = COMMAND_BUFFER_STATE_RECORDING;
}

void VulkanCommandBufferEnd(VulkanCommandBuffer *CommandBuffer)
{
    VK_CHECK(vkEndCommandBuffer(CommandBuffer->handle));
    CommandBuffer->state = COMMAND_BUFFER_STATE_RECORDING_ENDED;
}

void VulkanCommandBufferUpdateSubmitted(VulkanCommandBuffer *CommandBuffer)
{
    CommandBuffer->state = COMMAND_BUFFER_STATE_SUBMITTED;
}

void VulkanCommandBufferReset(VulkanCommandBuffer *CommandBuffer)
{
    CommandBuffer->state = COMMAND_BUFFER_STATE_READY;
}

void VulkanCommandBufferAllocateAndBeginSingleUse(VulkanAPI *VkAPI, VkCommandPool pool, VulkanCommandBuffer *OutCommandBuffer)
{
    VulkanCommandBufferAllocate(VkAPI, pool, true, OutCommandBuffer);
    VulkanCommandBufferBegin(OutCommandBuffer, true, false, false);
}

void VulkanCommandBufferEndSingleUse(VulkanAPI *VkAPI, VkCommandPool pool, VulkanCommandBuffer *CommandBuffer, VkQueue queue)
{
    // Завершить буфер команд.
    VulkanCommandBufferEnd(CommandBuffer);

    // Отправить очередь
    VkSubmitInfo SubmitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &CommandBuffer->handle;
    VK_CHECK(vkQueueSubmit(queue, 1, &SubmitInfo, 0));

    // Подождите, пока это закончится
    VK_CHECK(vkQueueWaitIdle(queue));

    // Освободите буфер команд.v
    VulkanCommandBufferFree(VkAPI, pool, CommandBuffer);
}
