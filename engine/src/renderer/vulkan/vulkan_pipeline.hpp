#pragma once

#include "defines.hpp"
#include "vulkan_renderpass.hpp"

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
    VulkanRenderPass* renderpass,
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
    bool DepthTest);

    void Destroy(class VulkanAPI* VkAPI);

    void Bind(class VulkanCommandBuffer* CommandBuffer, VkPipelineBindPoint BindPoint);
};
