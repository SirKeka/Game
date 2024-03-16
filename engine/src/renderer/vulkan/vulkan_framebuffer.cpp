#include "vulkan_framebuffer.hpp"
#include "vulkan_api.hpp"
#include "vulkan_device.hpp"

VulkanFramebuffer::VulkanFramebuffer(VulkanAPI *VkAPI, VulkanRenderPass *renderpass, u32 width, u32 height, u32 AttachmentCount, VkImageView *attachments)
{
    // Сделайте копию вложений, renderpass и количество вложений
    this->attachments = MMemory::TAllocate<VkImageView>(AttachmentCount, MEMORY_TAG_RENDERER);
    for (u32 i = 0; i < AttachmentCount; ++i) {
        this->attachments[i] = attachments[i];
    }
    this->renderpass = renderpass;
    this->AttachmentCount = AttachmentCount;

    // Creation info
    VkFramebufferCreateInfo FramebufferCreateInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    FramebufferCreateInfo.renderPass = renderpass->handle;
    FramebufferCreateInfo.attachmentCount = AttachmentCount;
    FramebufferCreateInfo.pAttachments = this->attachments;
    FramebufferCreateInfo.width = width;
    FramebufferCreateInfo.height = height;
    FramebufferCreateInfo.layers = 1;

    VK_CHECK(vkCreateFramebuffer(
        VkAPI->Device.LogicalDevice,
        &FramebufferCreateInfo,
        VkAPI->allocator,
        &this->handle));
}

void VulkanFramebuffer::Destroy(VulkanAPI *VkAPI)
{
    vkDestroyFramebuffer(VkAPI->Device.LogicalDevice, this->handle, VkAPI->allocator);
    if (this->attachments) { 
        MMemory::Free(this->attachments, sizeof(VkImageView) * this->AttachmentCount, MEMORY_TAG_RENDERER);
        this->attachments = 0;
    }
    this->handle = 0;
    this->AttachmentCount = 0;
    this->renderpass = 0;
}
