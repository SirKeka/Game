#pragma once

#include "renderer/renderer_types.hpp"
#include "renderer/vulkan/vulkan_pipeline.hpp"
#include "renderer/vulkan/vulkan_buffer.hpp"
#include "systems/texture_system.hpp"

class VulkanAPI;

struct VulkanShaderStage
{
    VkShaderModuleCreateInfo CreateInfo;
    VkShaderModule handle;
    VkPipelineShaderStageCreateInfo ShaderStageCreateInfo;
};

#define MATERIAL_SHADER_STAGE_COUNT 2
#define VULKAN_MATERIAL_SHADER_SAMPLER_COUNT 1

struct VulkanDescriptorState {
    // По одному на кадр
    u32 generations[3];
    u32 ids[3];
};

#define VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT 2
struct VulkanMaterialShaderInstanceState {
    // За кадр
    VkDescriptorSet DescriptorSets[3];

    // Для каждого дескриптора
    VulkanDescriptorState DescriptorStates[VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT];
};

// Максимальное количество объектов
#define VULKAN_MAX_MATERIAL_OBJECT_COUNT 1024

class VulkanMaterialShader
{
public:
    VulkanShaderStage stages[MATERIAL_SHADER_STAGE_COUNT];
    VkDescriptorPool GlobalDescriptorPool;
    VkDescriptorSetLayout GlobalDescriptorSetLayout;
    VkDescriptorSet GlobalDescriptorSets[3]; // Один набор дескрипторов на кадр - максимум 3 для тройной буферизации.
    GlobalUniformObject GlobalUObj;
    VulkanBuffer GlobalUniformBuffer;
    VkDescriptorPool ObjectDescriptorPool;
    VkDescriptorSetLayout ObjectDescriptorSetLayout;
    VulkanBuffer ObjectUniformBuffer; // Универсальный буфер объектов.
    // TODO: Вместо этого создать здесь какой-нибудь свободный список.
    u32 ObjectUniformBufferIndex = 0;

    TextureUse SamplerUses[VULKAN_MATERIAL_SHADER_SAMPLER_COUNT];

    // TODO: сделать динамическим
    VulkanMaterialShaderInstanceState InstanceStates[VULKAN_MAX_MATERIAL_OBJECT_COUNT];

    VulkanPipeline pipeline;
    
public:
    VulkanMaterialShader() = default;
    ~VulkanMaterialShader() = default;

    bool Create(VulkanAPI* VkAPI);
    void DestroyShaderModule(VulkanAPI* VkAPI);
    void Use(VulkanAPI* VkAPI);
    void UpdateGlobalState(VulkanAPI* VkAPI, f32 DeltaTime);
    void UpdateObject(VulkanAPI* VkAPI, const GeometryRenderData& data);
    bool AcquireResources(VulkanAPI* VkAPI, u32& OutObjectID);
    void ReleaseResources(VulkanAPI* VkAPI, u32 ObjectID);
};
