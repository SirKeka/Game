#pragma once

#include "vulkan_api.hpp"

class VulkanPipeline
{
public:
    VkPipeline handle;
    VkPipelineLayout PipelineLayout;
public:
    VulkanPipeline() = default;
    ~VulkanPipeline() = default;

    bool Create(
    VulkanAPI* VkAPI,
    VulkanRenderpass* renderpass,
    u32 AttributeCount,
    VkVertexInputAttributeDescription* attributes,
    u32 DescriptorSetLayoutCount,
    VkDescriptorSetLayout* DescriptorSetLayouts,
    u32 StageCount,
    VkPipelineShaderStageCreateInfo* stages,
    VkViewport viewport,
    VkRect2D scissor,
    bool IsWireframe/*,
    VulkanPipeline* OutPipeline*/);

    void Destroy(VulkanAPI* VkAPI/*, VulkanPipeline pipeline*/);

    void Bind(VulkanCommandBuffer* CommandBuffer, VkPipelineBindPoint BindPoint);
};
