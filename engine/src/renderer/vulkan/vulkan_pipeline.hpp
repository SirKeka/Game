#pragma once

#include "defines.hpp"
#include "vulkan_renderpass.hpp"

class VulkanAPI;
class VulkanCommandBuffer;

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
    VulkanRenderPass* renderpass,
    u32 AttributeCount,
    VkVertexInputAttributeDescription* attributes,
    u32 DescriptorSetLayoutCount,
    VkDescriptorSetLayout* DescriptorSetLayouts,
    u32 StageCount,
    VkPipelineShaderStageCreateInfo* stages,
    VkViewport viewport,
    VkRect2D scissor,
    bool IsWireframe);

    void Destroy(VulkanAPI* VkAPI);

    void Bind(VulkanCommandBuffer* CommandBuffer, VkPipelineBindPoint BindPoint);
};
