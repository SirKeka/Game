#pragma once

#include "renderer/renderer_types.hpp"
#include "renderer/vulkan/vulkan_pipeline.hpp"
#include "renderer/vulkan/vulkan_buffer.hpp"
#include "systems/texture_system.hpp"

struct VulkanShaderStage
{
    VkShaderModuleCreateInfo CreateInfo;
    VkShaderModule handle;
    VkPipelineShaderStageCreateInfo ShaderStageCreateInfo;
};

constexpr int MATERIAL_SHADER_STAGE_COUNT = 2;
constexpr int VULKAN_MATERIAL_SHADER_SAMPLER_COUNT = 1;

struct VulkanDescriptorState {
    // По одному на кадр
    u32 generations[3];
    u32 ids[3];
};

constexpr int VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT = 2;
struct VulkanMaterialShaderInstanceState {
    // За кадр
    VkDescriptorSet DescriptorSets[3];

    // Для каждого дескриптора
    VulkanDescriptorState DescriptorStates[VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT];
};

// Максимальное количество экземпляров материала
// TODO: сделать настраиваемым
constexpr int VULKAN_MAX_MATERIAL_COUNT = 1024;

class VulkanMaterialShader
{
public:
    VulkanShaderStage stages[MATERIAL_SHADER_STAGE_COUNT];
    VkDescriptorPool GlobalDescriptorPool;
    VkDescriptorSetLayout GlobalDescriptorSetLayout;
    VkDescriptorSet GlobalDescriptorSets[3]; // Один набор дескрипторов на кадр - максимум 3 для тройной буферизации.
    VulkanMaterialShaderGlobalUniformObject GlobalUObj;
    VulkanBuffer GlobalUniformBuffer;
    VkDescriptorPool ObjectDescriptorPool;
    VkDescriptorSetLayout ObjectDescriptorSetLayout;
    VulkanBuffer ObjectUniformBuffer; // Универсальный буфер объектов.
    // TODO: Вместо этого создать здесь какой-нибудь свободный список.
    u32 ObjectUniformBufferIndex = 0;

    TextureUse SamplerUses[VULKAN_MATERIAL_SHADER_SAMPLER_COUNT];

    // TODO: сделать динамическим
    VulkanMaterialShaderInstanceState InstanceStates[VULKAN_MAX_MATERIAL_COUNT];

    VulkanPipeline pipeline;
    
public:
    VulkanMaterialShader() = default;
    ~VulkanMaterialShader() = default;

    bool Create(class VulkanAPI* VkAPI);
    void DestroyShaderModule(class VulkanAPI* VkAPI);
    void Use(class VulkanAPI* VkAPI);
    void UpdateGlobalState(class VulkanAPI* VkAPI, f32 DeltaTime);
    void SetModel(class VulkanAPI* VkAPI, Matrix4D model);
    void ApplyMaterial(class VulkanAPI* VkAPI, class Material* material);
    bool AcquireResources(class VulkanAPI* VkAPI, class Material* material);
    void ReleaseResources(class VulkanAPI* VkAPI, class Material* material);
};
