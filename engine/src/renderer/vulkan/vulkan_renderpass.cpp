#include "vulkan_renderpass.hpp"

#include "core/mmemory.hpp"

void VulkanRenderpassCreate(VulkanAPI *VkAPI, VulkanRenderpass *OutRenderpass, f32 x, f32 y, f32 w, f32 h, f32 r, f32 g, f32 b, f32 a, f32 depth, u32 stencil)
{
    OutRenderpass->x = x;
    OutRenderpass->y = y;
    OutRenderpass->w = w;
    OutRenderpass->h = h;

    OutRenderpass->r = r;
    OutRenderpass->g = g;
    OutRenderpass->b = b;
    OutRenderpass->a = a;

    OutRenderpass->depth = depth;
    OutRenderpass->stencil = stencil;

    // Главный подпроход
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    // Вложения TODO: сделать это настраиваемым.
    const u32 AttachmentDescriptionCount = 2;
    VkAttachmentDescription AttachmentDescriptions[AttachmentDescriptionCount];

    // Цветное вложение
    VkAttachmentDescription ColorAttachmentcol;
    ColorAttachmentcol.format = VkAPI->swapchain.ImageFormat.format; // TODO: настроить
    ColorAttachmentcol.samples = VK_SAMPLE_COUNT_1_BIT;
    ColorAttachmentcol.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ColorAttachmentcol.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    ColorAttachmentcol.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ColorAttachmentcol.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ColorAttachmentcol.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;      // Не ожидайте какого-либо конкретного макета до начала этапа рендеринга.
    ColorAttachmentcol.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // Переход после прохождения рендеринга
    ColorAttachmentcol.flags = 0;

    AttachmentDescriptions[0] = ColorAttachmentcol;

    VkAttachmentReference ColorAttachmentReference;
    ColorAttachmentReference.attachment = 0;  // Индекс массива описания вложения
    ColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &ColorAttachmentReference;

    // Приставка по глубине, если она есть.
    VkAttachmentDescription DepthAttachment = {};
    DepthAttachment.format = VkAPI->Device.DepthFormat;
    DepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    DepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    DepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    DepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    DepthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    AttachmentDescriptions[1] = DepthAttachment;

    // Указание глубины
    VkAttachmentReference DepthAttachmentReference;
    DepthAttachmentReference.attachment = 1;
    DepthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // TODO: другие типы вложений (ввод, разрешение, сохранение)

    // Данные трафарета глубины.
    subpass.pDepthStencilAttachment = &DepthAttachmentReference;

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
        &OutRenderpass->handle));
}

void VulkanRenderpassDestroy(VulkanAPI *VkAPI, VulkanRenderpass *renderpass)
{
    if (renderpass && renderpass->handle) {
        vkDestroyRenderPass(VkAPI->Device.LogicalDevice, renderpass->handle, VkAPI->allocator);
        renderpass->handle = 0;
    }
}

void VulkanRenderpassBegin(VulkanCommandBuffer *CommandBuffer, VulkanRenderpass *renderpass, VkFramebuffer FrameBuffer)
{
    VkRenderPassBeginInfo BeginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    BeginInfo.renderPass = renderpass->handle;
    BeginInfo.framebuffer = FrameBuffer;
    BeginInfo.renderArea.offset.x = renderpass->x;
    BeginInfo.renderArea.offset.y = renderpass->y;
    BeginInfo.renderArea.extent.width = renderpass->w;
    BeginInfo.renderArea.extent.height = renderpass->h;

    VkClearValue ClearValues[2]; 
    MMemory::ZeroMem(ClearValues, sizeof(VkClearValue) * 2);
    ClearValues[0].color.float32[0] = renderpass->r;
    ClearValues[0].color.float32[1] = renderpass->g;
    ClearValues[0].color.float32[2] = renderpass->b;
    ClearValues[0].color.float32[3] = renderpass->a;
    ClearValues[1].depthStencil.depth = renderpass->depth;
    ClearValues[1].depthStencil.stencil = renderpass->stencil;

    BeginInfo.clearValueCount = 2;
    BeginInfo.pClearValues = ClearValues;

    vkCmdBeginRenderPass(CommandBuffer->handle, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    CommandBuffer->state = COMMAND_BUFFER_STATE_IN_RENDER_PASS;
}

void VulkanRenderpassEnd(VulkanCommandBuffer* CommandBuffer, VulkanRenderpass* renderpass) {
    vkCmdEndRenderPass(CommandBuffer->handle);
    CommandBuffer->state = COMMAND_BUFFER_STATE_RECORDING;
}
