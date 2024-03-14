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

    void ViewCreate(
        VulkanAPI* VkAPI,
        VkFormat format,
        VkImageAspectFlags AspectFlags);

        //Преобразует предоставленное изображение из OldLayout в NewLayout.
    void TransitionLayout(
        VulkanAPI* VkAPI,
        VulkanCommandBuffer* CommandBuffer,
        VkFormat format,
        VkImageLayout OldLayout,
        VkImageLayout NewLayout);

    /// @brief Копирует данные из буфера в предоставленное изображение.
    /// @param VkAPI указатель на объект отрисовщика типа VulkanAPI.
    /// @param image изображение, в которое нужно скопировать данные буфера.
    /// @param buffer буфер, данные которого будут скопированы.
    /// @param CommandBuffer 
    void CopyFromBuffer(
        VulkanAPI* VkAPI,
        VkBuffer buffer,
        VulkanCommandBuffer* CommandBuffer);

    void Destroy(VulkanAPI* VkAPI);

    void* operator new(u64 size);
};
