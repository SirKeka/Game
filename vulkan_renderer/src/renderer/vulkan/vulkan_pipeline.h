#pragma once

#include "defines.hpp"
#include "vulkan/vulkan.h"
#include "resources/texture.hpp"

class VulkanAPI;
struct VulkanRenderpass;

namespace VulkanTopology
{
    enum Class {
        Point = 0,
        Line = 1,
        Triangle = 2,
        MAX = Triangle + 1
    };
} // namespace VulkanTopology

    

class VulkanPipeline
{
public:
    /// @brief Структура конфигурации конвеера Vulkan.
    struct Config {
        MString name;                                  // Имя конвейера. Используется в основном для отладки.
        VulkanRenderpass* renderpass;                  // Указатель на проход рендеринга для связывания с конвейером.
        u32 stride;                                    // Шаг данных вершин, которые будут использоваться (например: sizeof(Vertex3D))
        u32 AttributeCount;                            // Количество атрибутов.
        VkVertexInputAttributeDescription* attributes; // Массив атрибутов. attributes;
        u32 DescriptorSetLayoutCount;                  // Количество макетов набора дескрипторов.
        VkDescriptorSetLayout* DescriptorSetLayouts;   // Массив макетов набора дескрипторов.
        u32 StageCount;                                // Количество стадий (вершина, фрагмент и т.д.).
        VkPipelineShaderStageCreateInfo* stages;       // Массив стадий VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BITarray.
        VkViewport viewport;                           // Начальная конфигурация области просмотра.
        VkRect2D scissor;                              // Начальная конфигурация ножниц.
        FaceCullMode CullMode;                         // Режим отбраковки граней.
        bool IsWireframe;                              // Указывает, должен ли этот конвейер использовать каркасный режим.
        u32 ShaderFlags;                               // Флаги шейдера, используемые для создания конвейера.
        u32 PushConstantRangeCount;                    // Количество диапазонов данных констант push.
        Range* PushConstantRanges;                     // Массив диапазонов данных констант push.
        u32 TopologyTypes;                             // Коллекция типов топологий, которые будут поддерживаться на этом конвейере.

        constexpr Config(const MString& name, VulkanRenderpass* renderpass, u32 stride, u32 AttributeCount, VkVertexInputAttributeDescription* attributes, u32 DescriptorSetLayoutCount, VkDescriptorSetLayout* DescriptorSetLayouts, u32 StageCount, VkPipelineShaderStageCreateInfo* stages, VkViewport viewport, VkRect2D scissor, FaceCullMode CullMode, bool IsWireframe, u32 ShaderFlags, u32 PushConstantRangeCount, Range* PushConstantRanges) 
        : 
        name(name),
        renderpass(renderpass), 
        stride(stride), 
        AttributeCount(AttributeCount), 
        attributes(attributes), 
        DescriptorSetLayoutCount(DescriptorSetLayoutCount), 
        DescriptorSetLayouts(DescriptorSetLayouts), 
        StageCount(StageCount), 
        stages(stages), 
        viewport(viewport), 
        scissor(scissor), 
        CullMode(CullMode), 
        IsWireframe(IsWireframe), 
        ShaderFlags(ShaderFlags),
        PushConstantRangeCount(PushConstantRangeCount),
        PushConstantRanges(PushConstantRanges),
        TopologyTypes() {}
    };

    VkPipeline handle{};            // Внутренний дескриптор конвейера.
    VkPipelineLayout PipelineLayout;// Макет конвейера.
    u32 SupportedTopologyTypes;     // Указывает типы топологии, используемые этим конвейером. См. PrimitiveTopologyType.
public:
    constexpr VulkanPipeline() : handle(), PipelineLayout() {}
    ~VulkanPipeline() = default;

    /// @brief Создает новый конвейер Vulkan.
    /// @param VkAPI указатель на Vulkan.
    /// @param config константная ссылка на конфигурацию, которая будет использоваться при создании конвейера.
    /// @param renderpass указатель на проход рендеринга для связи с конвейером.
    /// @param stride шаг данных вершин, которые будут использоваться (например, sizeof(Vertex3D))
    /// @param AttributeCount количество атрибутов.
    /// @param attributes массив атрибутов.
    /// @param DescriptorSetLayoutCount количество макетов набора дескрипторов.
    /// @param DescriptorSetLayouts массив макетов набора дескрипторов.
    /// @param StageCount количество стадий (вершина, фрагмент и т. д.).
    /// @param stages массив стадий.
    /// @param viewport конфигурация области просмотра.
    /// @param scissor конфигурация ножниц.
    /// @param CullMode режим отбраковки лиц.
    /// @param IsWireframe режим отбраковки граней.
    /// @param DepthTest указывает, должен ли этот конвейер использовать каркасный режим.
    /// @param PushConstantRangeCount указывает, включено ли тестирование глубины для этого конвейера/
    /// @param PushConstantRanges указатель для хранения вновь созданного конвейера.
    /// @return true в случае успеха; в противном случае false.
    bool Create(VulkanAPI* VkAPI, const Config& config);
    // struct VulkanRenderpass* renderpass,
    // u32 stride,
    // u32 AttributeCount,
    // VkVertexInputAttributeDescription* attributes,
    // u32 DescriptorSetLayoutCount,
    // VkDescriptorSetLayout* DescriptorSetLayouts,
    // u32 StageCount,
    // VkPipelineShaderStageCreateInfo* stages,
    // VkViewport viewport,
    // VkRect2D scissor,
    // FaceCullMode CullMode,
    // bool IsWireframe,
    // bool DepthTest,
    // u32 PushConstantRangeCount,
    // Range* PushConstantRanges);

    void Destroy(VulkanAPI* VkAPI);

    /// @brief Связывает данный конвейер для использования. Это должно быть сделано в проходе рендеринга.
    /// @param CommandBuffer Буфер команд для назначения команды привязки.
    /// @param BindPoint Точка привязки конвейера (обычно BindPointGraphics)
    void Bind(struct VulkanCommandBuffer& CommandBuffer, VkPipelineBindPoint BindPoint);
};
