#include "vulkan_fence.hpp"
#include "vulkan_device.hpp"

void VulkanFenceCreate(VulkanAPI *VkAPI, bool CreateSignaled, VulkanFence *OutFence)
{
    // Make sure to signal the fence if required.
    OutFence->IsSignaled = CreateSignaled;
    VkFenceCreateInfo FenceCreateInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    if (OutFence->IsSignaled) {
        FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    }

    VK_CHECK(vkCreateFence(
        VkAPI->Device.LogicalDevice,
        &FenceCreateInfo,
        VkAPI->allocator,
        &OutFence->handle));
}

void VulkanFenceDestroy(VulkanAPI *VkAPI, VulkanFence *fence)
{
    if (fence->handle) {
        vkDestroyFence(
            VkAPI->Device.LogicalDevice,
            fence->handle,
            VkAPI->allocator);
        fence->handle = 0;
    }
    fence->IsSignaled = false;
}

bool VulkanFenceWait(VulkanAPI *VkAPI, VulkanFence *fence, u64 TimeoutNs)
{
    if (!fence->IsSignaled) {
        VkResult result = vkWaitForFences(
            VkAPI->Device.LogicalDevice,
            1,
            &fence->handle,
            true,
            TimeoutNs);
        switch (result) {
            case VK_SUCCESS:
                fence->IsSignaled = true;
                return true;
            case VK_TIMEOUT:
                MWARN("vk_fence_wait - Таймаут");
                break;
            case VK_ERROR_DEVICE_LOST:
                MERROR("vk_fence_wait - VK_ERROR_DEVICE_LOST.");//
                break;
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                MERROR("vk_fence_wait - VK_ERROR_OUT_OF_HOST_MEMORY.");
                break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                MERROR("vk_fence_wait - VK_ERROR_OUT_OF_DEVICE_MEMORY.");
                break;
            default:
                MERROR("vk_fence_wait - Произошла неизвестная ошибка.");
                break;
        }
    } else {
        // Если сигнал уже подан, не ждите.
        return true;
    }

    return false;
}

void VulkanFenceReset(VulkanAPI *VkAPI, VulkanFence *fence)
{
    if (fence->IsSignaled) {
        VK_CHECK(vkResetFences(VkAPI->Device.LogicalDevice, 1, &fence->handle));
        fence->IsSignaled = false;
    }
}
