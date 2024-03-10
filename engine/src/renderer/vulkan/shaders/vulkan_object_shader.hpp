#pragma once

#include "renderer/vulkan/vulkan_api.hpp"
#include "renderer/vulkan/vulkan_pipeline.hpp"
#include "renderer/vulkan/vulkan_buffer.hpp"

#define OBJECT_SHADER_STAGE_COUNT 2

struct VulkanShaderStage
{
    VkShaderModuleCreateInfo CreateInfo;
    VkShaderModule handle;
    VkPipelineShaderStageCreateInfo ShaderStageCreateInfo;
};


class VulkanObjectShader
{
public:
    VulkanShaderStage stages[OBJECT_SHADER_STAGE_COUNT];
    VkDescriptorPool GlobalDescriptorPool;
    VkDescriptorSetLayout GlobalDescriptorSetLayout;
    VkDescriptorSet GlobalDescriptorSets[3]; // Один набор дескрипторов на кадр - максимум 3 для тройной буферизации.
    GlobalUniformObject GlobalUObj;
    VulkanBuffer GlobalUniformBuffer;
    VulkanPipeline pipeline;
    
public:
    VulkanObjectShader() = default;
    ~VulkanObjectShader() = default;

    bool Create(VulkanAPI* VkAPI);
    void DestroyShaderModule(VulkanAPI* VkAPI);
    void Use(VulkanAPI* VkAPI);
    void UpdateGlobalState(VulkanAPI* VkAPI);
};
