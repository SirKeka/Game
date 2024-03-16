#pragma once

#include "defines.hpp"
#include <vulkan/vulkan.h>

class VulkanAPI;
class VulkanRenderPass;

class VulkanFramebuffer
{
public:
    VkFramebuffer handle;
    u32 AttachmentCount;
    VkImageView* attachments;
    VulkanRenderPass* renderpass;
public:
    VulkanFramebuffer() = default;
    VulkanFramebuffer(VulkanAPI* VkAPI,
    VulkanRenderPass* renderpass,
    u32 width,
    u32 height,
    u32 AttachmentCount,
    VkImageView* attachments);
    ~VulkanFramebuffer() = default;
    void Destroy(VulkanAPI* VkAPI);
};

