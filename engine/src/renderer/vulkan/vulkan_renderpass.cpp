#include "vulkan_renderpass.hpp"
#include "vulkan_api.hpp"

//#include "core/mmemory.hpp"

VulkanRenderPass::VulkanRenderPass(
    VulkanAPI* VkAPI, 
    Vector4D<f32> RenderArea,
    Vector4D<f32> ClearColor,
    f32 depth,
    u32 stencil,
    u8 ClearFlags,
    bool HasPrevPass,
    bool HasNextPass)
{
    Create(VkAPI, RenderArea, ClearColor, depth, stencil, ClearFlags, HasPrevPass, HasNextPass);
}

void VulkanRenderPass::Create(VulkanAPI *VkAPI, Vector4D<f32> RenderArea, Vector4D<f32> ClearColor, f32 depth, u32 stencil, u8 ClearFlags, bool HasPrevPass, bool HasNextPass)
{
    this->RenderArea = RenderArea;
    this->ClearColour = ClearColor;
    this->depth = depth;
    this->stencil = stencil;
    this->ClearFlags = ClearFlags;
    this->HasPrevPass = HasPrevPass;
    this->HasNextPass = HasNextPass;

    // Главный подпроход
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    // Вложения TODO: сделать это настраиваемым.
    u32 AttachmentDescriptionCount = 0;
    VkAttachmentDescription AttachmentDescriptions[2];

    // Цветное вложение
    bool DoClearColour = (this->ClearFlags & RenderpassClearFlag::ColourBufferFlag) != 0;
    VkAttachmentDescription ColorAttachmentcol;
    ColorAttachmentcol.format = VkAPI->swapchain.ImageFormat.format; // TODO: настроить
    ColorAttachmentcol.samples = VK_SAMPLE_COUNT_1_BIT;
    ColorAttachmentcol.loadOp = DoClearColour ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    ColorAttachmentcol.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    ColorAttachmentcol.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ColorAttachmentcol.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // Если исходит из предыдущего прохода, он уже должен быть VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL. В противном случае неопределенно.
    ColorAttachmentcol.initialLayout = HasPrevPass ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;

    // Если собираетесь на другой проход, используйте VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL. В противном случае VK_IMAGE_LAYOUT_PRESENT_SRC_KHR.
    ColorAttachmentcol.finalLayout = HasNextPass ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // Переход к этапу после рендеринга
    ColorAttachmentcol.flags = 0;

    AttachmentDescriptions[AttachmentDescriptionCount] = ColorAttachmentcol;
    AttachmentDescriptionCount++;

    VkAttachmentReference ColorAttachmentReference;
    ColorAttachmentReference.attachment = 0;  // Индекс массива описания вложения
    ColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &ColorAttachmentReference;

    // Приставка по глубине, если она есть.
    bool DoClearDepth = (this->ClearFlags & RenderpassClearFlag::DepthBufferFlag) != 0;
    if (DoClearDepth){
        VkAttachmentDescription DepthAttachment = {};
        DepthAttachment.format = VkAPI->Device.DepthFormat;
        DepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        DepthAttachment.loadOp = DoClearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        DepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        DepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        DepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        DepthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        AttachmentDescriptions[AttachmentDescriptionCount] = DepthAttachment;
        AttachmentDescriptionCount++;

        // Указание глубины
        VkAttachmentReference DepthAttachmentReference;
        DepthAttachmentReference.attachment = 1;
        DepthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // TODO: другие типы вложений (ввод, разрешение, сохранение)

        // Данные трафарета глубины.
        subpass.pDepthStencilAttachment = &DepthAttachmentReference;
    } else {
        MMemory::ZeroMem(&AttachmentDescriptions[AttachmentDescriptionCount], sizeof(VkAttachmentDescription));
        subpass.pDepthStencilAttachment = 0;
    }
    
    // Ввод из шейдера
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = 0;

    // Вложения, используемые для вложений цвета с множественной выборкой
    subpass.pResolveAttachments = 0;

    // Вложения не используются в этом подпроходе, но должны быть сохранены для следующего.
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = 0;

    // Зависимости прохода рендеринга. TODO: сделать это настраиваемым.
    VkSubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = 0;

    // Создание прохода рендеринга.
    VkRenderPassCreateInfo RenderPassCreateInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    RenderPassCreateInfo.attachmentCount = AttachmentDescriptionCount;
    RenderPassCreateInfo.pAttachments = AttachmentDescriptions;
    RenderPassCreateInfo.subpassCount = 1;
    RenderPassCreateInfo.pSubpasses = &subpass;
    RenderPassCreateInfo.dependencyCount = 1;
    RenderPassCreateInfo.pDependencies = &dependency;
    RenderPassCreateInfo.pNext = 0;
    RenderPassCreateInfo.flags = 0;

    VK_CHECK(vkCreateRenderPass(
        VkAPI->Device.LogicalDevice,
        &RenderPassCreateInfo,
        VkAPI->allocator,
        &this->handle));
}

void VulkanRenderPass::Destroy(VulkanAPI *VkAPI)
{
    if (this->handle) {
        vkDestroyRenderPass(VkAPI->Device.LogicalDevice, this->handle, VkAPI->allocator);
        this->handle = 0;
    }
}

void VulkanRenderPass::Begin(VulkanCommandBuffer *CommandBuffer, VkFramebuffer FrameBuffer)
{
    VkRenderPassBeginInfo BeginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    BeginInfo.renderPass = this->handle;
    BeginInfo.framebuffer = FrameBuffer;
    BeginInfo.renderArea.offset.x = this->RenderArea.x;
    BeginInfo.renderArea.offset.y = this->RenderArea.y;
    BeginInfo.renderArea.extent.width = this->RenderArea.z;
    BeginInfo.renderArea.extent.height = this->RenderArea.w;

    BeginInfo.clearValueCount = 0;
    BeginInfo.pClearValues = nullptr;

    VkClearValue ClearValues[2]{};
    bool DoClearColour = (this->ClearFlags & RenderpassClearFlag::ColourBufferFlag) != 0;
    if (DoClearColour) {
        MMemory::CopyMem(ClearValues[BeginInfo.clearValueCount].color.float32, this->ClearColour.elements, sizeof(f32) * 4);
        BeginInfo.clearValueCount++;
    }

    bool DoClearDepth = (this->ClearFlags & RenderpassClearFlag::DepthBufferFlag) != 0;
    if (DoClearDepth) {
        MMemory::CopyMem(ClearValues[BeginInfo.clearValueCount].color.float32, this->ClearColour.elements, sizeof(f32) * 4);
        ClearValues[BeginInfo.clearValueCount].depthStencil.depth = this->depth;

        bool DoClearStencil = (this->ClearFlags & RenderpassClearFlag::StencilBufferFlag) != 0;
        ClearValues[BeginInfo.clearValueCount].depthStencil.stencil = DoClearStencil ? this->stencil : 0;
        BeginInfo.clearValueCount++;
    }

    BeginInfo.pClearValues = BeginInfo.clearValueCount > 0 ? ClearValues : 0;

    vkCmdBeginRenderPass(CommandBuffer->handle, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    CommandBuffer->state = COMMAND_BUFFER_STATE_IN_RENDER_PASS;
}

void VulkanRenderPass::End(VulkanCommandBuffer* CommandBuffer) {
    vkCmdEndRenderPass(CommandBuffer->handle);
    CommandBuffer->state = COMMAND_BUFFER_STATE_RECORDING;
}
