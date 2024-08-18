#include "vulkan_renderpass.hpp"

void VulkanRenderpass::Destroy(VulkanAPI *VkAPI)
{
    if (this->handle) {
        vkDestroyRenderPass(VkAPI->Device.LogicalDevice, this->handle, VkAPI->allocator);
        this->handle = 0;
    }
}

void *VulkanRenderpass::operator new(u64 size)
{
    return MMemory::Allocate(size, Memory::Renderer);
}

void VulkanRenderpass::operator delete(void *ptr, u64 size)
{
    MMemory::Free(ptr, size, Memory::Renderer);
}
