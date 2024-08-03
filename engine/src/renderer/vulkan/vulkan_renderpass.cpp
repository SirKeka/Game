#include "vulkan_renderpass.hpp"
#include "vulkan_api.hpp"

//#include "core/mmemory.hpp"

constexpr VulkanRenderpass::VulkanRenderpass(f32 depth, u32 stencil, bool HasPrevPass, bool HasNextPass, VulkanAPI* VkAPI)
: handle(), depth(depth), stencil(stencil), HasPrevPass(HasPrevPass), HasNextPass(HasNextPass), state()
{
    // Главный подпроход
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    // Вложения ЗАДАЧА: сделать это настраиваемым.
    u32 AttachmentDescriptionCount = 0;
    VkAttachmentDescription AttachmentDescriptions[2];

    // Цветное вложение
    bool DoClearColour = (this->ClearFlags & RenderpassClearFlag::ColourBuffer) != 0;
    VkAttachmentDescription ColorAttachment;
    ColorAttachment.format = VkAPI->swapchain.ImageFormat.format; // ЗАДАЧА: настроить
    ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    ColorAttachment.loadOp = DoClearColour ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // Если исходит из предыдущего прохода, он уже должен быть VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL. В противном случае неопределенно.
    ColorAttachment.initialLayout = HasPrevPass ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;

    // Если собираетесь на другой проход, используйте VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL. В противном случае VK_IMAGE_LAYOUT_PRESENT_SRC_KHR.
    ColorAttachment.finalLayout = HasNextPass ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // Переход к этапу после рендеринга
    ColorAttachment.flags = 0;

    AttachmentDescriptions[AttachmentDescriptionCount] = ColorAttachment;
    AttachmentDescriptionCount++;

    VkAttachmentReference ColorAttachmentReference;
    ColorAttachmentReference.attachment = 0;  // Индекс массива описания вложения
    ColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &ColorAttachmentReference;

    // Приставка по глубине, если она есть.
    bool DoClearDepth = (this->ClearFlags & RenderpassClearFlag::DepthBuffer) != 0;
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

        // ЗАДАЧА: другие типы вложений (ввод, разрешение, сохранение)

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

    // Зависимости прохода рендеринга. ЗАДАЧА: сделать это настраиваемым.
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
        &this->handle)
    );
}

void VulkanRenderpass::Destroy(VulkanAPI *VkAPI)
{
    if (this->handle) {
        vkDestroyRenderPass(VkAPI->Device.LogicalDevice, this->handle, VkAPI->allocator);
        this->handle = 0;
    }
}
