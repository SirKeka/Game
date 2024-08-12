#pragma once

#include "defines.hpp"
#include "vulkan/vulkan.h"

class VulkanPipeline
{
public:
    VkPipeline handle{};
    VkPipelineLayout PipelineLayout;
public:
    constexpr VulkanPipeline() : handle(), PipelineLayout() {}
    ~VulkanPipeline() = default;

    /// @brief Создает новый конвейер Vulkan.
    /// @param VkAPI указатель на Vulkan.
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
    bool Create(
    class VulkanAPI* VkAPI,
    class VulkanRenderpass* renderpass,
    u32 stride,
    u32 AttributeCount,
    VkVertexInputAttributeDescription* attributes,
    u32 DescriptorSetLayoutCount,
    VkDescriptorSetLayout* DescriptorSetLayouts,
    u32 StageCount,
    VkPipelineShaderStageCreateInfo* stages,
    VkViewport viewport,
    VkRect2D scissor,
    FaceCullMode CullMode,
    bool IsWireframe,
    bool DepthTest,
    u32 PushConstantRangeCount,
    Range* PushConstantRanges);

    void Destroy(class VulkanAPI* VkAPI);

    /// @brief Связывает данный конвейер для использования. Это должно быть сделано в проходе рендеринга.
    /// @param CommandBuffer Буфер команд для назначения команды привязки.
    /// @param BindPoint Точка привязки конвейера (обычно BindPointGraphics)
    void Bind(class VulkanCommandBuffer& CommandBuffer, VkPipelineBindPoint BindPoint);
};
