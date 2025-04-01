#include "vulkan_pipeline.hpp"

#include "vulkan_device.hpp"
#include "vulkan_utils.hpp"
#include "vulkan_renderpass.hpp"
#include "vulkan_command_buffer.hpp"

bool VulkanPipeline::Create(VulkanAPI* VkAPI, const Config& config)
{
    // Состояние видового экрана
    VkPipelineViewportStateCreateInfo ViewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    ViewportState.viewportCount = 1;
    ViewportState.pViewports = &config.viewport;
    ViewportState.scissorCount = 1;
    ViewportState.pScissors = &config.scissor;

    // Растеризатор
    VkPipelineRasterizationStateCreateInfo RasterizerCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    RasterizerCreateInfo.depthClampEnable = VK_FALSE;
    RasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    RasterizerCreateInfo.polygonMode = config.IsWireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    RasterizerCreateInfo.lineWidth = 1.0f;
    switch (config.CullMode) {
        case FaceCullMode::None:
            RasterizerCreateInfo.cullMode = VK_CULL_MODE_NONE;
            break;
        case FaceCullMode::Front:
            RasterizerCreateInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
            break;
        default:
        case FaceCullMode::Back:
            RasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
            break;
        case FaceCullMode::FrontAndBack:
            RasterizerCreateInfo.cullMode = VK_CULL_MODE_FRONT_AND_BACK;
            break;
    }
    RasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    RasterizerCreateInfo.depthBiasEnable = VK_FALSE;
    RasterizerCreateInfo.depthBiasConstantFactor = 0.0f;
    RasterizerCreateInfo.depthBiasClamp = 0.0f;
    RasterizerCreateInfo.depthBiasSlopeFactor = 0.0f;

    // Мультисэмплинг.
    VkPipelineMultisampleStateCreateInfo MultisamplingCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    MultisamplingCreateInfo.sampleShadingEnable = VK_FALSE;
    MultisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    MultisamplingCreateInfo.minSampleShading = 1.0f;
    MultisamplingCreateInfo.pSampleMask = 0;
    MultisamplingCreateInfo.alphaToCoverageEnable = VK_FALSE;
    MultisamplingCreateInfo.alphaToOneEnable = VK_FALSE;

    // Проверка глубины и трафарета.
    VkPipelineDepthStencilStateCreateInfo DepthStencil = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    if (config.ShaderFlags & Shader::DepthTestFlag) {
        DepthStencil.depthTestEnable = VK_TRUE;
        if (config.ShaderFlags & Shader::DepthWriteFlag) {
           DepthStencil.depthWriteEnable = VK_TRUE;
        }
        DepthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        DepthStencil.depthBoundsTestEnable = VK_FALSE;
        DepthStencil.stencilTestEnable = VK_FALSE;
    }

    VkPipelineColorBlendAttachmentState ColorBlendAttachmentState;
    MemorySystem::ZeroMem(&ColorBlendAttachmentState, sizeof(VkPipelineColorBlendAttachmentState));
    ColorBlendAttachmentState.blendEnable = VK_TRUE;
    ColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    ColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    ColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    ColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    ColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    ColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

    ColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                               VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo ColorBlendStateCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    ColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
    ColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    ColorBlendStateCreateInfo.attachmentCount = 1;
    ColorBlendStateCreateInfo.pAttachments = &ColorBlendAttachmentState;

    // Динамическое состояние
    const u32 DynamicStateCount = 3;
    VkDynamicState DynamicStates[DynamicStateCount] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH};

    VkPipelineDynamicStateCreateInfo DynamicStateCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    DynamicStateCreateInfo.dynamicStateCount = DynamicStateCount;
    DynamicStateCreateInfo.pDynamicStates = DynamicStates;

    // Вершинный ввод
    VkVertexInputBindingDescription BindingDescription;
    BindingDescription.binding = 0;  // Индекс привязки
    BindingDescription.stride = config.stride;
    BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;  // Переходите к следующему вводу данных для каждой вершины.

    // Атрибуты
    VkPipelineVertexInputStateCreateInfo VertexInputInfo = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    VertexInputInfo.vertexBindingDescriptionCount = 1;
    VertexInputInfo.pVertexBindingDescriptions = &BindingDescription;
    VertexInputInfo.vertexAttributeDescriptionCount = config.AttributeCount;
    VertexInputInfo.pVertexAttributeDescriptions = config.attributes;

    // Входной узел
    VkPipelineInputAssemblyStateCreateInfo InputAssembly = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    InputAssembly.primitiveRestartEnable = VK_FALSE;

    // Макет конвейера
    VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

    // Push-константы
    VkPushConstantRange ranges[32]{};
    if (config.PushConstantRangeCount > 0) {
        if (config.PushConstantRangeCount > 32) {
            MERROR("VulkanPipeline::Create — не может иметь более 32 диапазонов push-констант. Пройдено количество: %i", config.PushConstantRangeCount);
            return false;
        }

        // ПРИМЕЧАНИЕ: 32 — это максимальное количество диапазонов, которое мы можем когда-либо иметь, поскольку спецификация гарантирует только 128 байтов с 4-байтовым выравниванием.
        for (u32 i = 0; i < config.PushConstantRangeCount; ++i) {
            ranges[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            ranges[i].offset = config.PushConstantRanges[i].offset;
            ranges[i].size = config.PushConstantRanges[i].size;
        }
        PipelineLayoutCreateInfo.pushConstantRangeCount = config.PushConstantRangeCount;
        PipelineLayoutCreateInfo.pPushConstantRanges = ranges;
    } else {
        PipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        PipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
    }

    // Макеты набора дескрипторов
    PipelineLayoutCreateInfo.setLayoutCount = config.DescriptorSetLayoutCount;
    PipelineLayoutCreateInfo.pSetLayouts = config.DescriptorSetLayouts;

    // Создание макета конвейера.
    VK_CHECK(vkCreatePipelineLayout(
        VkAPI->Device.LogicalDevice,
        &PipelineLayoutCreateInfo,
        VkAPI->allocator,
        &PipelineLayout)
    );

    char PipelineLayoutNameBuf[512]{};
    MString::Format(PipelineLayoutNameBuf, "pipeline_shader_%s", config.name.c_str());
    if (!VulkanSetDebugObjectName(VkAPI, VK_OBJECT_TYPE_PIPELINE_LAYOUT, PipelineLayout, PipelineLayoutNameBuf)) {
        MWARN("Невозможно настроить имя объекта отладки для %s.", PipelineLayoutNameBuf);
    }

    // Создание конвейера
    VkGraphicsPipelineCreateInfo PipelineCreateInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    PipelineCreateInfo.stageCount = config.StageCount;
    PipelineCreateInfo.pStages = config.stages;
    PipelineCreateInfo.pVertexInputState = &VertexInputInfo;
    PipelineCreateInfo.pInputAssemblyState = &InputAssembly;

    PipelineCreateInfo.pViewportState = &ViewportState;
    PipelineCreateInfo.pRasterizationState = &RasterizerCreateInfo;
    PipelineCreateInfo.pMultisampleState = &MultisamplingCreateInfo;
    PipelineCreateInfo.pDepthStencilState = (config.ShaderFlags & Shader::DepthTestFlag) ? &DepthStencil : nullptr;
    PipelineCreateInfo.pColorBlendState = &ColorBlendStateCreateInfo;
    PipelineCreateInfo.pDynamicState = &DynamicStateCreateInfo;
    PipelineCreateInfo.pTessellationState = nullptr;

    PipelineCreateInfo.layout = PipelineLayout;

    PipelineCreateInfo.renderPass = config.renderpass->handle;
    PipelineCreateInfo.subpass = 0;
    PipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    PipelineCreateInfo.basePipelineIndex = -1;

    VkResult result = vkCreateGraphicsPipelines(
        VkAPI->Device.LogicalDevice,
        VK_NULL_HANDLE,
        1,
        &PipelineCreateInfo,
        VkAPI->allocator,
        &handle
    );

    char PipelineNameBuf[512] = "pipeline_shader_";
    MString::Format(PipelineNameBuf, config.name.c_str());
    if (!VulkanSetDebugObjectName(VkAPI, VK_OBJECT_TYPE_PIPELINE, handle, PipelineNameBuf)) {
        MWARN("Невозможно настроить имя объекта отладки для %s.", PipelineNameBuf);
    }

    if (VulkanResultIsSuccess(result)) {
        MDEBUG("Графический конвейер создан!");
        return true;
    }

    MERROR("Не удалось создать vkCreateGraphicsPipelines с помощью %s.", VulkanResultString(result, true));

    return false;
}

void VulkanPipeline::Destroy(VulkanAPI *VkAPI)
{
    // Уничтожить конвейер
    if (handle) {
        vkDestroyPipeline(VkAPI->Device.LogicalDevice, handle, VkAPI->allocator);
        handle = 0;
    }
    // Уничтожить макет
    if (PipelineLayout) {
        vkDestroyPipelineLayout(VkAPI->Device.LogicalDevice, PipelineLayout, VkAPI->allocator);
        PipelineLayout = 0;
    }
}

void VulkanPipeline::Bind(VulkanCommandBuffer &CommandBuffer, VkPipelineBindPoint BindPoint)
{
    vkCmdBindPipeline(CommandBuffer.handle, BindPoint, handle);
}
