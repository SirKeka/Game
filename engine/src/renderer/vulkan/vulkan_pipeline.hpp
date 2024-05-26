#pragma once

#include "defines.hpp"
#include "vulkan_command_buffer.hpp"

class VulkanPipeline
{
public:
    VkPipeline handle{};
    VkPipelineLayout PipelineLayout;
public:
    VulkanPipeline() = default;
    ~VulkanPipeline() = default;

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
    bool IsWireframe,
    bool DepthTest,
    u32 PushConstantRangeCount,
    Range* PushConstantRanges);

    void Destroy(class VulkanAPI* VkAPI);

    void Bind(VulkanCommandBuffer& CommandBuffer, VkPipelineBindPoint BindPoint);
};
