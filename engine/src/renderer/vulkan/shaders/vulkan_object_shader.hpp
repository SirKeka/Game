#pragma once

#include "renderer/vulkan/vulkan_api.hpp"
#include "renderer/vulkan/vulkan_pipeline.hpp"

#define OBJECT_SHADER_STAGE_COUNT 2

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
    void DestroyShaderModule(VulkanAPI* VkAPI);
    void Use(VulkanAPI* VkAPI);
};
