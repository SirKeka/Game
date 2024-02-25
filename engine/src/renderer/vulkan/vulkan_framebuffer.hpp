#pragma once

#include "vulkan_api.hpp"

void VulkanFramebufferCreate(
    VulkanAPI* VkAPI,
    VulkanRenderpass* renderpass,
    u32 width,
    u32 height,
    u32 AttachmentCount,
    VkImageView* attachments,
    VulkanFramebuffer* OutFramebuffer);

void VulkanFramebufferDestroy(VulkanAPI* VkAPI, VulkanFramebuffer* framebuffer);