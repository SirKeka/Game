#include "vulkan_command_buffer.h"
#include "vulkan_api.h"

//#include "core/mmemory.hpp"

void VulkanCommandBuffer::Allocate(VulkanAPI *VkAPI, VkCommandPool pool, bool IsPrimary)
{
    // MMemory::ZeroMem(this, sizeof(VulkanCommandBuffer));

    VkCommandBufferAllocateInfo AllocateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = pool,
        .level = IsPrimary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY,
        .commandBufferCount = 1
    };

    state = VulkanCommandBufferState::NOT_ALLOCATED;
    VK_CHECK(vkAllocateCommandBuffers(VkAPI->device.LogicalDevice, &AllocateInfo, &handle));
    state = VulkanCommandBufferState::READY;
}

void VulkanCommandBuffer::Free(VulkanAPI *VkAPI, VkCommandPool pool)
{
    vkFreeCommandBuffers(VkAPI->device.LogicalDevice, pool, 1, &handle);

    handle = VK_NULL_HANDLE;
    state = VulkanCommandBufferState::NOT_ALLOCATED;
}

void VulkanCommandBuffer::Begin(bool IsSingleUse, bool IsRenderpassContinue, bool IsSimultaneousUse)
{
    VkCommandBufferBeginInfo BeginInfo {
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

    VK_CHECK(vkBeginCommandBuffer(handle, &BeginInfo));
    state = VulkanCommandBufferState::RECORDING;
}

void VulkanCommandBuffer::End()
{
    VK_CHECK(vkEndCommandBuffer(handle));
    state = VulkanCommandBufferState::RECORDING_ENDED;
}

void VulkanCommandBuffer::Reset()
{
    //VK_CHECK(vkResetCommandBuffer(handle, 0));
    state = VulkanCommandBufferState::READY;
}

void VulkanCommandBuffer::AllocateAndBeginSingleUse(VulkanAPI *VkAPI, VkCommandPool pool)
{
    Allocate(VkAPI, pool, true);
    Begin(true, false, false);
}

void VulkanCommandBuffer::EndSingleUse(VulkanAPI *VkAPI, VkCommandPool pool, VkQueue queue)
{
    // Завершить буфер команд.
    End();

    // Отправить очередь
    VkSubmitInfo SubmitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &handle
    };

    VK_CHECK(vkQueueSubmit(queue, 1, &SubmitInfo, 0));

    // Подождите, пока это закончится
    VK_CHECK(vkQueueWaitIdle(queue));

    // Освободите буфер команд.v
    Free(VkAPI, pool);
}
