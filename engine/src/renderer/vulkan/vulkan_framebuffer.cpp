#include "vulkan_framebuffer.hpp"

void VulkanFramebufferCreate(
    VulkanAPI *VkAPI, 
    VulkanRenderpass *renderpass, 
    u32 width, u32 height, u32 AttachmentCount, 
    VkImageView *attachments, 
    VulkanFramebuffer *OutFramebuffer)
{
    // Сделайте копию вложений, renderpass и количество вложений
    OutFramebuffer->attachments = MMemory::TAllocate<VkImageView>(AttachmentCount, MEMORY_TAG_RENDERER);
    for (u32 i = 0; i < AttachmentCount; ++i) {
        OutFramebuffer->attachments[i] = attachments[i];
    }
    OutFramebuffer->renderpass = renderpass;
    OutFramebuffer->AttachmentCount = AttachmentCount;

    // Creation info
    VkFramebufferCreateInfo FramebufferCreateInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    FramebufferCreateInfo.renderPass = renderpass->handle;
    FramebufferCreateInfo.attachmentCount = AttachmentCount;
    FramebufferCreateInfo.pAttachments = OutFramebuffer->attachments;
    FramebufferCreateInfo.width = width;
    FramebufferCreateInfo.height = height;
    FramebufferCreateInfo.layers = 1;

    VK_CHECK(vkCreateFramebuffer(
        VkAPI->Device.LogicalDevice,
        &FramebufferCreateInfo,
        VkAPI->allocator,
        &OutFramebuffer->handle));
}

void VulkanFramebufferDestroy(VulkanAPI *VkAPI, VulkanFramebuffer *framebuffer)
{
    vkDestroyFramebuffer(VkAPI->Device.LogicalDevice, framebuffer->handle, VkAPI->allocator);
    if (framebuffer->attachments) { 
        MMemory::Free(framebuffer->attachments, sizeof(VkImageView) * framebuffer->AttachmentCount, MEMORY_TAG_RENDERER);
        framebuffer->attachments = 0;
    }
    framebuffer->handle = 0;
    framebuffer->AttachmentCount = 0;
    framebuffer->renderpass = 0;
}
