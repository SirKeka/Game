#pragma once

#include "renderer/vulkan/vulkan_api.hpp"

#define OBJECT_SHADER_STAGE_COUNT 2

struct VulkanPipeline
{
    VkPipeline handle;
    VkPipelineLayout PipelineLayout;
};


struct VulkanShaderStage
{
    VkShaderModuleCreateInfo CreateInfo;
    VkShaderModule handle;
    VkPipelineShaderStageCreateInfo ShaderStageCreateInfo;
};


class VulkanObjectShader
{
private:
    VulkanShaderStage stages[OBJECT_SHADER_STAGE_COUNT];

    VulkanPipeline pipeline;
    
public:
    VulkanObjectShader() = default;
    ~VulkanObjectShader() = default;

    bool Create(VulkanAPI* VkAPI);
    void Use(VulkanAPI* VkAPI);
};
