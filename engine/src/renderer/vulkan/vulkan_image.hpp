#pragma once

#include "vulkan_api.hpp"

class VulkanImage
{
using LinearAllocator = WrapLinearAllocator<VulkanImage>;
public:
    VkImage handle{};
    VkDeviceMemory memory{};
    VkImageView view{};
    u32 width = 0;
    u32 height = 0;
public:
    VulkanImage(VulkanAPI* VkAPI,
        VkImageType ImageType,
        u32 width,
        u32 height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags MemoryFlags,
        b32 CreateView,
        VkImageAspectFlags ViewAspectFlags);
    ~VulkanImage() = default;

    void Create(
        VulkanAPI* VkAPI,
        VkImageType ImageType,
        u32 width,
        u32 height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags MemoryFlags,
        b32 CreateView,
        VkImageAspectFlags ViewAspectFlags);

    void ViewCreate(
        VulkanAPI* VkAPI,
        VkFormat format,
        VkImageAspectFlags AspectFlags);

        //Преобразует предоставленное изображение из OldLayout в NewLayout.
    void TransitionLayout(
        VulkanAPI* VkAPI,
        VulkanCommandBuffer* CommandBuffer,
        VulkanImage* image,
        VkFormat format,
        VkImageLayout OldLayout,
        VkImageLayout NewLayout);

    /**
     * Copies data in buffer to provided image.
     * @param VkAPI The Vulkan context.
     * @param image The image to copy the buffer's data to.
     * @param buffer The buffer whose data will be copied.
     */
    void CopyFromBuffer(
        VulkanAPI* VkAPI,
        VulkanImage* image,
        VkBuffer buffer,
        VulkanCommandBuffer* CommandBuffer);

    void Destroy(VulkanAPI* VkAPI);

    void* operator new(u64 size);
};


