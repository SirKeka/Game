#pragma once

#include "renderer/renderer_types.hpp"
#include "renderer/vulkan/vulkan_pipeline.hpp"
#include "renderer/vulkan/vulkan_buffer.hpp"

class VulkanAPI;

struct VulkanShaderStage
{
    VkShaderModuleCreateInfo CreateInfo;
    VkShaderModule handle;
    VkPipelineShaderStageCreateInfo ShaderStageCreateInfo;
};

#define OBJECT_SHADER_STAGE_COUNT 2

struct VulkanDescriptorState {
    // По одному на кадр
    u32 generations[3];
    u32 ids[3];
};

#define VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT 2
struct VulkanObjectShaderObjectState {
    // За кадр
    VkDescriptorSet DescriptorSets[3];

    // Для каждого дескриптора
    VulkanDescriptorState DescriptorStates[VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT];
};

// Максимальное количество объектов
#define VULKAN_OBJECT_MAX_OBJECT_COUNT 1024

class VulkanMaterialShader
{
public:
    VulkanShaderStage stages[OBJECT_SHADER_STAGE_COUNT];
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

    // TODO: сделать динамическим
    VulkanObjectShaderObjectState ObjectStates[VULKAN_OBJECT_MAX_OBJECT_COUNT];

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
