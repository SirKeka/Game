#include "vulkan_pipeline.hpp"

#include "vulkan_device.hpp"
#include "math/vertex3D.hpp"
#include "vulkan_utils.hpp"

bool VulkanPipeline::Create(
    VulkanAPI *VkAPI, 
    VulkanRenderpass *renderpass, 
    u32 AttributeCount, 
    VkVertexInputAttributeDescription *attributes, 
    u32 DescriptorSetLayoutCount, 
    VkDescriptorSetLayout *DescriptorSetLayouts, 
    u32 StageCount, VkPipelineShaderStageCreateInfo *stages, 
    VkViewport viewport, 
    VkRect2D scissor, 
    bool IsWireframe/*, 
    VulkanPipeline *OutPipeline*/)
{
    // Состояние видового экрана
    VkPipelineViewportStateCreateInfo ViewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    ViewportState.viewportCount = 1;
    ViewportState.pViewports = &viewport;
    ViewportState.scissorCount = 1;
    ViewportState.pScissors = &scissor;

    // Растеризатор
    VkPipelineRasterizationStateCreateInfo RasterizerCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    RasterizerCreateInfo.depthClampEnable = VK_FALSE;
    RasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    RasterizerCreateInfo.polygonMode = IsWireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    RasterizerCreateInfo.lineWidth = 1.0f;
    RasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
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
    DepthStencil.depthTestEnable = VK_TRUE;
    DepthStencil.depthWriteEnable = VK_TRUE;
    DepthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    DepthStencil.depthBoundsTestEnable = VK_FALSE;
    DepthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState ColorBlendAttachmentState;
    MMemory::ZeroMem(&ColorBlendAttachmentState, sizeof(VkPipelineColorBlendAttachmentState));
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
    BindingDescription.stride = sizeof(Vertex3D);
    BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;  // Переходите к следующему вводу данных для каждой вершины.

    // Атрибуты
    VkPipelineVertexInputStateCreateInfo VertexInputInfo = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    VertexInputInfo.vertexBindingDescriptionCount = 1;
    VertexInputInfo.pVertexBindingDescriptions = &BindingDescription;
    VertexInputInfo.vertexAttributeDescriptionCount = AttributeCount;
    VertexInputInfo.pVertexAttributeDescriptions = attributes;

    // Входной узел
    VkPipelineInputAssemblyStateCreateInfo InputAssembly = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    InputAssembly.primitiveRestartEnable = VK_FALSE;

    // Макет конвейера
    VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

    // Push-константы
    VkPushConstantRange PushConstant;
    PushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    PushConstant.offset = sizeof(Matrix4D) * 0;
    PushConstant.size = sizeof(Matrix4D) * 2;
    PipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    PipelineLayoutCreateInfo.pPushConstantRanges = &PushConstant;

    // Макеты набора дескрипторов
    PipelineLayoutCreateInfo.setLayoutCount = DescriptorSetLayoutCount;
    PipelineLayoutCreateInfo.pSetLayouts = DescriptorSetLayouts;

    // Создание макета конвейера.
    VK_CHECK(vkCreatePipelineLayout(
        VkAPI->Device->LogicalDevice,
        &PipelineLayoutCreateInfo,
        VkAPI->allocator,
        &PipelineLayout));

    // Создание конвейера
    VkGraphicsPipelineCreateInfo PipelineCreateInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    PipelineCreateInfo.stageCount = StageCount;
    PipelineCreateInfo.pStages = stages;
    PipelineCreateInfo.pVertexInputState = &VertexInputInfo;
    PipelineCreateInfo.pInputAssemblyState = &InputAssembly;

    PipelineCreateInfo.pViewportState = &ViewportState;
    PipelineCreateInfo.pRasterizationState = &RasterizerCreateInfo;
    PipelineCreateInfo.pMultisampleState = &MultisamplingCreateInfo;
    PipelineCreateInfo.pDepthStencilState = &DepthStencil;
    PipelineCreateInfo.pColorBlendState = &ColorBlendStateCreateInfo;
    PipelineCreateInfo.pDynamicState = &DynamicStateCreateInfo;
    PipelineCreateInfo.pTessellationState = 0;

    PipelineCreateInfo.layout = PipelineLayout;

    PipelineCreateInfo.renderPass = renderpass->handle;
    PipelineCreateInfo.subpass = 0;
    PipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    PipelineCreateInfo.basePipelineIndex = -1;

    VkResult result = vkCreateGraphicsPipelines(
        VkAPI->Device->LogicalDevice,
        VK_NULL_HANDLE,
        1,
        &PipelineCreateInfo,
        VkAPI->allocator,
        &handle);

    if (VulkanResultIsSuccess(result)) {
        MDEBUG("Графический конвейер создан!");
        return true;
    }

    MERROR("Не удалось создать vkCreateGraphicsPipelines с помощью %s.", VulkanResultString(result, true));

    return false;
}

void VulkanPipeline::Destroy(VulkanAPI *VkAPI/*, VulkanPipeline pipeline*/)
{
    // Уничтожить конвейер
    if (handle) {
        vkDestroyPipeline(VkAPI->Device->LogicalDevice, handle, VkAPI->allocator);
        handle = 0;
    }
    // Уничтожить макет
    if (PipelineLayout) {
        vkDestroyPipelineLayout(VkAPI->Device->LogicalDevice, PipelineLayout, VkAPI->allocator);
        PipelineLayout = 0;
    }
}

void VulkanPipeline::Bind(VulkanCommandBuffer *CommandBuffer, VkPipelineBindPoint BindPoint)
{
    vkCmdBindPipeline(CommandBuffer->handle, BindPoint, handle);
}
