/*
 * Прежде чем завершить создание графического конвейера нужно сообщить Vulkan, 
 * какие буферы (attachments) будут использоваться во время рендеринга. 
 * Необходимо указать, сколько будет буферов цвета, буферов глубины и сэмплов 
 * для каждого буфера. Также нужно указать, как должно обрабатываться содержимое 
 * буферов во время рендеринга. Вся эта информация обернута в объект прохода рендера (render pass).
*/
// ЗАДАЧА: Перенести все это в фаил vulkan_api.cpp?
#pragma once

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

    VulkanRenderpassState state; // Указывает состояние прохода рендеринга.

    constexpr VulkanRenderpass() : handle(), depth(), stencil(), state() {}
    VulkanRenderpass(u8 ClearFlags, const RenderpassConfig& config, VulkanAPI* VkAPI)
    : handle(), depth(config.depth), stencil(config.stencil), state()
    {
        // Главный подпроход
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        // Вложения ЗАДАЧА: сделать это настраиваемым.
        DArray<VkAttachmentDescription> AttachmentDescriptions;
        DArray<VkAttachmentDescription> ColourAttachmentDescs;
        DArray<VkAttachmentDescription> DepthAttachmentDescs;

        // Всегда можно просто посмотреть на первую цель, так как они все одинаковы (по одной на кадр).
        // auto target = targets[0];
        for (u32 i = 0; i < config.target.AttachmentCount; ++i) {
            auto& AttachmentConfig = config.target.attachments[i];

            VkAttachmentDescription AttachmentDesc = {};
            if (AttachmentConfig.type == RenderTargetAttachmentType::Colour) {
                // Цветовая приставка.
                bool DoClearColour = (ClearFlags & RenderpassClearFlag::ColourBuffer) != 0;

                if (AttachmentConfig.source == RenderTargetAttachmentSource::Default) {
                    AttachmentDesc.format = VkAPI->swapchain.ImageFormat.format;
                } else {
                    // ЗАДАЧА: настраиваемый формат?
                    AttachmentDesc.format = VK_FORMAT_R8G8B8A8_UNORM;
                }

                AttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
                // AttachmentDesc.loadOp = DoClearColour ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;

                // Определите, какую операцию загрузки использовать.
                if (AttachmentConfig.LoadOperation == RenderTargetAttachmentLoadOperation::DontCare) {
                    // Если нас это не волнует, единственное, что нужно проверить, — это очищается ли вложение.
                    AttachmentDesc.loadOp = DoClearColour ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                } else {
                    // Если мы загружаем, проверьте, очищаем ли мы его. Эта комбинация не имеет смысла и должна быть предупреждена.
                    if (AttachmentConfig.LoadOperation == RenderTargetAttachmentLoadOperation::Load) {
                        if (DoClearColour) {
                            MWARN("Операция загрузки цветного вложения установлена ​​на загрузку, но также установлена ​​на очистку. Эта комбинация недопустима и приведет к ошибке очистки. Проверьте конфигурацию вложения.");
                            AttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                        } else {
                            AttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                        }
                    } else {
                        MFATAL("Недопустимая и неподдерживаемая комбинация операции загрузки (0x%x) и флагов очистки (0x%x) для цветного вложения.", AttachmentDesc.loadOp, ClearFlags);
                        return;
                    }
                }

                // Определите, какую операцию сохранения использовать.
                if (AttachmentConfig.StoreOperation == RenderTargetAttachmentStoreOperation::DontCare){
                    AttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                } else if (AttachmentConfig.StoreOperation == RenderTargetAttachmentStoreOperation::Store) {
                    AttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                } else {
                    MFATAL("Неверная операция сохранения (0x%x) установлена ​​для присоединения глубины. Проверьте конфигурацию.", AttachmentConfig.StoreOperation);
                    return;
                }

                // ПРИМЕЧАНИЕ: они никогда не будут использоваться для цветного вложения.
                AttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                AttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                // Если загрузка, это означает, что она происходит из другого прохода, то есть формат должен быть VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL. В противном случае он не определен.
                AttachmentDesc.initialLayout = AttachmentConfig.LoadOperation == RenderTargetAttachmentLoadOperation::Load ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;

                // Если это последний проход, записывающий в это вложение, present after должно быть установлено в true.
                AttachmentDesc.finalLayout = AttachmentConfig.PresentAfter ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;  // Transitioned to after the render pass
                AttachmentDesc.flags = 0;

                // Push в массив цветных вложений.
                ColourAttachmentDescs.PushBack(AttachmentDesc);
            } else if (AttachmentConfig.type == RenderTargetAttachmentType::Depth) {
                // Глубина вложения.
                bool DoClearDepth = (ClearFlags & RenderpassClearFlag::DepthBuffer) != 0;

                if (AttachmentConfig.source == RenderTargetAttachmentSource::Default) {
                    AttachmentDesc.format = VkAPI->Device.DepthFormat;
                } else {
                    // ЗАДАЧА: Может быть более оптимальный формат для использования, если не задана глубина по умолчанию.
                    AttachmentDesc.format = VkAPI->Device.DepthFormat;
                }

                AttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
                // Определите, какую операцию загрузки использовать.
                if (AttachmentConfig.LoadOperation == RenderTargetAttachmentLoadOperation::DontCare) {
                    // Если нам все равно, единственное, что нужно проверить, — это очищается ли вложение.
                    AttachmentDesc.loadOp = DoClearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                } else {
                    // Если мы загружаем, проверьте, очищаемся ли мы также. Эта комбинация не имеет смысла и должна быть предупреждена.
                    if (AttachmentConfig.LoadOperation == RenderTargetAttachmentLoadOperation::Load) {
                        if (DoClearDepth) {
                            MWARN("Операция загрузки прикрепления глубины установлена ​​на загрузку, но также установлена ​​на очистку. Эта комбинация недопустима и приведет к ошибке очистки. Проверьте конфигурацию прикрепления.");
                            AttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                        } else {
                            AttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                        }
                    } else {
                        MFATAL("Недопустимая и неподдерживаемая комбинация операции загрузки (0x%x) и флагов очистки (0x%x) для прикрепления глубины.", AttachmentDesc.loadOp, ClearFlags);
                        return;
                    }
                }

                // Определите, какую операцию хранения использовать.
                if (AttachmentConfig.StoreOperation == RenderTargetAttachmentStoreOperation::DontCare) {
                    AttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                } else if (AttachmentConfig.StoreOperation == RenderTargetAttachmentStoreOperation::Store) {
                    AttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                } else {
                    MFATAL("Неверная операция сохранения (0x%x) установлена ​​для присоединения глубины. Проверьте конфигурацию.", AttachmentConfig.StoreOperation);
                    return;
                }

                // TODO: Настраиваемость для прикреплений трафарета.
                AttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                AttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                // Если исходит из предыдущего прохода, уже должно быть VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL. В противном случае не определено.
                AttachmentDesc.initialLayout = AttachmentConfig.LoadOperation == RenderTargetAttachmentLoadOperation::Load ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
                // Окончательный макет для прикреплений трафарета глубины всегда такой.
                AttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                // Отправить в массив цветных прикреплений.
                DepthAttachmentDescs.PushBack(AttachmentDesc);
            }
            // Отправить в общий массив.
            AttachmentDescriptions.PushBack(AttachmentDesc);
        }

        // Настроить ссылки на прикрепления.
        u32 AttachmentsAdded = 0;

        // Ссылка на цветное прикрепление.
        VkAttachmentReference* ColourAttachmentReferences = nullptr;
        u32 ColourAttachmentCount = ColourAttachmentDescs.Length();
        if (ColourAttachmentCount > 0) {
            ColourAttachmentReferences = MMemory::TAllocate<VkAttachmentReference>(Memory::Array, ColourAttachmentCount);
            for (u32 i = 0; i < ColourAttachmentCount; ++i) {
                ColourAttachmentReferences[i].attachment = AttachmentsAdded;  // Индекс массива описания вложения
                ColourAttachmentReferences[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                AttachmentsAdded++;
            }

            subpass.colorAttachmentCount = ColourAttachmentCount;
            subpass.pColorAttachments = ColourAttachmentReferences;
        } else {
            subpass.colorAttachmentCount = 0;
            subpass.pColorAttachments = 0;
        }

        // Ссылка на глубину прикрепления.
        VkAttachmentReference* DepthAttachmentReferences = nullptr;
        u32 DepthAttachmentCount = DepthAttachmentDescs.Length();
        if (DepthAttachmentCount > 0) {
            MASSERT_MSG(DepthAttachmentCount == 1, "Вложения с несколькими глубинами не поддерживаются.");
            DepthAttachmentReferences = MMemory::TAllocate<VkAttachmentReference>(Memory::Array, DepthAttachmentCount);
            for (u32 i = 0; i < DepthAttachmentCount; ++i) {
                DepthAttachmentReferences[i].attachment = AttachmentsAdded;  // Индекс массива описания прикрепления
                DepthAttachmentReferences[i].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                AttachmentsAdded++;
            }

            // ЗАДАЧА: другие типы вложений (ввод, разрешение, сохранение)

            // Данные трафарета глубины.
            subpass.pDepthStencilAttachment = DepthAttachmentReferences;
        } else {
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
        RenderPassCreateInfo.attachmentCount = AttachmentDescriptions.Length();
        RenderPassCreateInfo.pAttachments = AttachmentDescriptions.Data();
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

        if (ColourAttachmentReferences) {
            MMemory::Free(ColourAttachmentReferences, sizeof(VkAttachmentReference) * ColourAttachmentCount, Memory::Array);
        }

        if (DepthAttachmentReferences) {
            MMemory::Free(DepthAttachmentReferences, sizeof(VkAttachmentReference) * DepthAttachmentCount, Memory::Array);
        }

        //return true;
    }
    
    ~VulkanRenderpass() = default;

    void Destroy(VulkanAPI* VkAPI);

    void* operator new(u64 size);
    void operator delete(void* ptr, u64 size);
};


