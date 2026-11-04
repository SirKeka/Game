#include "vulkan_pipeline.h"

#include "vulkan_device.h"
#include "vulkan_utils.h"
#include "vulkan_renderpass.h"
#include "vulkan_command_buffer.h"

#include "resources/shader.h"

bool VulkanPipeline::Create(VulkanAPI* VkAPI, const Config& config)
{
    // Состояние видового экрана
    VkPipelineViewportStateCreateInfo ViewportState {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports    = &config.viewport,
        .scissorCount  = 1,
        .pScissors     = &config.scissor
    };

    // Растеризатор
    VkPipelineRasterizationStateCreateInfo RasterizerCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    RasterizerCreateInfo.depthClampEnable = VK_FALSE;
    RasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    RasterizerCreateInfo.polygonMode = (config.ShaderFlags & Shader::Wireframe) ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    RasterizerCreateInfo.lineWidth = 1.F;
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

    if (config.winding == RendererWinding::Clockwise) {
        RasterizerCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    } else if (config.winding == RendererWinding::CounterClockwise) {
        RasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    } else {
        MWARN("Указан неверный порядок намотки на лицевой стороне, по умолчанию против часовой стрелки.");
        RasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    }
    RasterizerCreateInfo.depthBiasEnable = VK_FALSE;
    RasterizerCreateInfo.depthBiasConstantFactor = 0.F;
    RasterizerCreateInfo.depthBiasClamp = 0.F;
    RasterizerCreateInfo.depthBiasSlopeFactor = 0.F;

    // Плавная растеризация линий, если поддерживается.
    VkPipelineRasterizationLineStateCreateInfoEXT LineRasterizationExt{};
    if (VkAPI->device.supportFlags & VulkanDevice::LineSmoothRasterisationBit) {
        LineRasterizationExt.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_LINE_STATE_CREATE_INFO_EXT;
        LineRasterizationExt.lineRasterizationMode = VK_LINE_RASTERIZATION_MODE_RECTANGULAR_SMOOTH_EXT;
        RasterizerCreateInfo.pNext = &LineRasterizationExt;
    }

    // Мультисэмплинг.
    VkPipelineMultisampleStateCreateInfo MultisamplingCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.F,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };

    // Проверка глубины и трафарета.
    VkPipelineDepthStencilStateCreateInfo DepthStencil = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    if (config.ShaderFlags & Shader::DepthTest) {
        DepthStencil.depthTestEnable = VK_TRUE;
        if (config.ShaderFlags & Shader::DepthWrite) {
           DepthStencil.depthWriteEnable = VK_TRUE;
        }
        DepthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        DepthStencil.depthBoundsTestEnable = VK_FALSE;
    }
    
    DepthStencil.stencilTestEnable =  (config.ShaderFlags & Shader::StencilTest) ? VK_TRUE : VK_FALSE;
    if (config.ShaderFlags & Shader::StencilTest) {
        // Эквивалент glStencilFunc(func, ref, mask)
        DepthStencil.back.compareOp = VK_COMPARE_OP_ALWAYS;
        DepthStencil.back.reference = 1;
        DepthStencil.back.compareMask = 0xFF;

        // Эквивалент glStencilOp(stencilFail, deepFail, deepPass)pipelin
        DepthStencil.back.failOp = VK_STENCIL_OP_ZERO;
        DepthStencil.back.depthFailOp = VK_STENCIL_OP_ZERO;
        DepthStencil.back.passOp = VK_STENCIL_OP_REPLACE;
        // Эквивалент glStencilMask(mask)

        // Задняя поверхность
        DepthStencil.back.writeMask = (config.ShaderFlags & Shader::StencilWrite) ? 0xFF : 0x00;

        // Передняя поверхность. Просто используйте те же настройки для передней/задней поверхности.
        DepthStencil.front = DepthStencil.back;
    }

    VkPipelineColorBlendAttachmentState ColorBlendAttachmentState {
        .blendEnable         = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp        = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .alphaBlendOp        = VK_BLEND_OP_ADD,
        .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                               VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo ColorBlendStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &ColorBlendAttachmentState
    };

    // Динамическое состояние
    DArray<VkDynamicState> DynamicStates {10, AllocatorType::Temp};
    DynamicStates.PushBack(VK_DYNAMIC_STATE_VIEWPORT);
    DynamicStates.PushBack(VK_DYNAMIC_STATE_SCISSOR);
    // Динамическое состояние, если поддерживается.
    if ((VkAPI->device.supportFlags & VulkanDevice::NativeDynamicStateBit) || (VkAPI->device.supportFlags & VulkanDevice::DynamicStateBit)) {
        DynamicStates.PushBack(VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY);
        DynamicStates.PushBack(VK_DYNAMIC_STATE_FRONT_FACE);
        DynamicStates.PushBack(VK_DYNAMIC_STATE_STENCIL_OP);
        DynamicStates.PushBack(VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE_EXT);
        DynamicStates.PushBack(VK_DYNAMIC_STATE_STENCIL_WRITE_MASK);
        DynamicStates.PushBack(VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK);
        DynamicStates.PushBack(VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE);
        DynamicStates.PushBack(VK_DYNAMIC_STATE_STENCIL_REFERENCE);
        // DynamicStates.PushBack(VK_DYNAMIC_STATE_COLOR_WRITE_ENABLE_EXT);
        // DynamicStates.PushBack(VK_DYNAMIC_STATE_COLOR_WRITE_MASK_EXT);
    }

    VkPipelineDynamicStateCreateInfo DynamicStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = DynamicStates.Length(),
        .pDynamicStates = DynamicStates.Data()
    };

    // Вершинный ввод
    VkVertexInputBindingDescription BindingDescription {
        .binding = 0, // Индекс привязки
        .stride = config.stride,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX  // Переходите к следующему вводу данных для каждой вершины.
    };

    // Атрибуты
    VkPipelineVertexInputStateCreateInfo VertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &BindingDescription,
        .vertexAttributeDescriptionCount = config.AttributeCount,
        .pVertexAttributeDescriptions = config.attributes
    };

    // Входной узел
    VkPipelineInputAssemblyStateCreateInfo InputAssembly = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    // Создаваемый конвейер уже имеет доступные типы, поэтому просто выберите первый из них.
    for (u32 i = 1; i < PrimitiveTopology::Type::MAX; i = i << 1) {
        if (SupportedTopologyTypes & i) {
            PrimitiveTopology::Type ptt = (PrimitiveTopology::Type)i;

            switch (ptt) {
                case PrimitiveTopology::Type::PointList:
                    InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
                    break;
                case PrimitiveTopology::Type::LineList:
                    InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
                    break;
                case PrimitiveTopology::Type::LineStrip:
                    InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
                    break;
                case PrimitiveTopology::Type::TriangleList:
                    InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                    break;
                case PrimitiveTopology::Type::TriangleStrip:
                    InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
                    break;
                case PrimitiveTopology::Type::TriangleFan:
                    InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
                    break;
                default:
                    MWARN("Примитивная топология '%u' не поддерживается. Пропуск.", ptt);
                    break;
            }
            break;
        }
    }
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
        VkAPI->device.LogicalDevice,
        &PipelineLayoutCreateInfo,
        VkAPI->allocator,
        &PipelineLayout)
    );

    char PipelineLayoutNameBuf[512]{};
    MString::Format(PipelineLayoutNameBuf, "pipeline_layout_%s", config.name.c_str());
    VulkanSetDebugObjectName(VkAPI, VK_OBJECT_TYPE_PIPELINE_LAYOUT, PipelineLayout, PipelineLayoutNameBuf);

    // Создание конвейера
    VkGraphicsPipelineCreateInfo PipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = config.StageCount,
        .pStages = config.stages,
        .pVertexInputState = &VertexInputInfo,
        .pInputAssemblyState = &InputAssembly,
        .pTessellationState = nullptr,
        .pViewportState = &ViewportState,
        .pRasterizationState = &RasterizerCreateInfo,
        .pMultisampleState = &MultisamplingCreateInfo,
        .pDepthStencilState = ((config.ShaderFlags & Shader::DepthTest) || (config.ShaderFlags & Shader::StencilTest)) ? &DepthStencil : nullptr,
        .pColorBlendState = &ColorBlendStateCreateInfo,
        .pDynamicState = &DynamicStateCreateInfo,
        .layout = PipelineLayout,
        .renderPass = config.renderpass->handle,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    VkResult result = vkCreateGraphicsPipelines(
        VkAPI->device.LogicalDevice,
        VK_NULL_HANDLE,
        1,
        &PipelineCreateInfo,
        VkAPI->allocator,
        &handle
    );

    char PipelineNameBuf[512]{};
    MString::Format(PipelineNameBuf, "pipeline_%s", config.name.c_str());
    VulkanSetDebugObjectName(VkAPI, VK_OBJECT_TYPE_PIPELINE, handle, PipelineNameBuf);

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
        vkDestroyPipeline(VkAPI->device.LogicalDevice, handle, VkAPI->allocator);
        handle = 0;
    }
    // Уничтожить макет
    if (PipelineLayout) {
        vkDestroyPipelineLayout(VkAPI->device.LogicalDevice, PipelineLayout, VkAPI->allocator);
        PipelineLayout = 0;
    }
}

void VulkanPipeline::Bind(VulkanCommandBuffer &CommandBuffer, VkPipelineBindPoint BindPoint)
{
    vkCmdBindPipeline(CommandBuffer.handle, BindPoint, handle);
}
