#pragma once

#include "vulkan_api.hpp"

void VulkanImageCreate(
    VulkanAPI* VkAPI,
    VkImageType ImageType,
    u32 width,
    u32 height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags MemoryFlags,
    b32 CreateView,
    VkImageAspectFlags ViewAspectFlags,
    VulkanImage* OutImage);

void VulkanImageViewCreate(
    VulkanAPI* VkAPI,
    VkFormat format,
    VulkanImage* image,
    VkImageAspectFlags AspectFlags);

void VulkanImageDestroy(VulkanAPI* VkAPI, VulkanImage* image);