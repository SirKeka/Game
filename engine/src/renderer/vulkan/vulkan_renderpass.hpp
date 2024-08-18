/*
 * Прежде чем завершить создание графического конвейера нужно сообщить Vulkan, 
 * какие буферы (attachments) будут использоваться во время рендеринга. 
 * Необходимо указать, сколько будет буферов цвета, буферов глубины и сэмплов 
 * для каждого буфера. Также нужно указать, как должно обрабатываться содержимое 
 * буферов во время рендеринга. Вся эта информация обернута в объект прохода рендера (render pass).
*/
// ЗАДАЧА: Перенести все это в фаил vulkan_api.cpp?
#pragma once

#include "defines.hpp"
#include "vulkan_api.hpp"

enum class VulkanRenderpassState 
{
    Ready,
    Recording,
    InRenderPass,
    RecordingEnded,
    Submitted,
    NotAllocated
};

struct VulkanRenderpass
{
    VkRenderPass handle;         // Внутренний дескриптор прохода рендеринга.

    f32 depth;                   // Значение очистки глубины.
    u32 stencil;                 // Значение очистки трафарета.
    bool HasPrevPass;            // Указывает, есть ли предыдущий проход рендеринга.
    bool HasNextPass;            // Указывает, есть ли следующий проход рендеринга.

    VulkanRenderpassState state; // Указывает состояние прохода рендеринга.

    constexpr VulkanRenderpass() : handle(), depth(), stencil(), HasPrevPass(), HasNextPass(), state() {}
    constexpr VulkanRenderpass(u8 ClearFlags, f32 depth, u32 stencil, bool HasPrevPass, bool HasNextPass, VulkanAPI* VkAPI)
    : handle(), depth(depth), stencil(stencil), HasPrevPass(HasPrevPass), HasNextPass(HasNextPass), state()
    {
        // Главный подпроход
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        // Вложения ЗАДАЧА: сделать это настраиваемым.
        u32 AttachmentDescriptionCount = 0;
        VkAttachmentDescription AttachmentDescriptions[2];

        // Цветное вложение
        bool DoClearColour = (ClearFlags & RenderpassClearFlag::ColourBuffer) != 0;
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
        bool DoClearDepth = (ClearFlags & RenderpassClearFlag::DepthBuffer) != 0;
        if (DoClearDepth){
            VkAttachmentDescription DepthAttachment = {};
            DepthAttachment.format = VkAPI->Device.DepthFormat;
            DepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            if (HasPrevPass) {
                DepthAttachment.loadOp = DoClearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
            } else {
                DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            }
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
            &handle)
        );
    }
    
    ~VulkanRenderpass() = default;

    void Destroy(VulkanAPI* VkAPI);

    void* operator new(u64 size) {
        return MMemory::Allocate(size, Memory::Renderer);
    }
    void operator delete(void* ptr, u64 size) {
        MMemory::Free(ptr, size, Memory::Renderer);
    }
};


