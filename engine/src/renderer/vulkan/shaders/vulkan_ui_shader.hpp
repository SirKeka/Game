#pragma once
#include "renderer/renderer_types.hpp"
#include "vulkan_material_shader.hpp"
#include "resources/texture.hpp"

constexpr int UI_SHADER_STAGE_COUNT = 2;
constexpr int VULKAN_UI_SHADER_DESCRIPTOR_COUNT = 2;
constexpr int VULKAN_UI_SHADER_SAMPLER_COUNT = 1;

// Max number of ui control instances
// TODO: make configurable
constexpr int VULKAN_MAX_UI_COUNT = 1024;

struct VulkanUI_ShaderInstanceState {
    // Для каждого кадра кадр
    VkDescriptorSet DescriptorSets[3];

    // Для каждого дескриптора
    VulkanDescriptorState DescriptorStates[VULKAN_UI_SHADER_DESCRIPTOR_COUNT];
};

class VulkanUI_Shader
{
friend class VulkanAPI;
private:
    // вершина, фрагмент
    VulkanShaderStage stages[UI_SHADER_STAGE_COUNT];

    VkDescriptorPool GlobalDescriptorPool;
    VkDescriptorSetLayout GlobalDescriptorSetLayout;

    // Один набор дескрипторов на кадр — максимум 3 для тройной буферизации.
    VkDescriptorSet GlobalDescriptorSets[3];

    // Global uniform object.
    VulkanUI_ShaderGlobalUniformObject GlobalUbo;

    // Global uniform buffer.
    VulkanBuffer GlobalUniformBuffer;

    VkDescriptorPool ObjectDescriptorPool;
    VkDescriptorSetLayout ObjectDescriptorSetLayout;
    // Object uniform buffers.
    VulkanBuffer ObjectUniformBuffer{};
    // TODO: вместо этого использовать список свободной памяти.
    u32 ObjectUniformBufferIndex;

    TextureUse SamplerUses[VULKAN_UI_SHADER_SAMPLER_COUNT];

    // TODO: Сделать динамичным
    VulkanUI_ShaderInstanceState InstanceStates[VULKAN_MAX_UI_COUNT];

    VulkanPipeline pipeline;
public:
    VulkanUI_Shader() {};
    ~VulkanUI_Shader() {};
    
    bool Create(VulkanAPI* VkAPI);

    void Destroy(VulkanAPI* VkAPI);

    void Use(VulkanAPI* VkAPI);

    void UpdateGlobalState(VulkanAPI* VkAPI, f32 DeltaTime);

    void SetModel(VulkanAPI* VkAPI, Matrix4D model);
    void ApplyMaterial(VulkanAPI* VkAPI, Material* material);

    bool AcquireResources(VulkanAPI* VkAPI, Material* material);
    void ReleaseResources(VulkanAPI* VkAPI, Material* material);
};
