#include "vulkan_object_shader.hpp"

#include "math/mvector3d.hpp"
#include "renderer/vulkan/vulkan_device.hpp"
#include "renderer/vulkan/vulkan_shader_utils.hpp"

#define BUILTIN_SHADER_NAME_OBJECT "Builtin.ObjectShader"

bool VulkanObjectShader::Create(VulkanAPI *VkAPI)
{
    // Инициализация модуля шейдера на каждом этапе.
    char StageTypeStrs[OBJECT_SHADER_STAGE_COUNT][5] = {"vert", "frag"};
    VkShaderStageFlagBits StageTypes[OBJECT_SHADER_STAGE_COUNT] = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};

    for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
        if (!VulkanShadersUtil::CreateShaderModule(VkAPI, BUILTIN_SHADER_NAME_OBJECT, StageTypeStrs[i], StageTypes[i], i, stages)) {
            MERROR("Невозможно создать %s шейдерный модуль для '%s'.", StageTypeStrs[i], BUILTIN_SHADER_NAME_OBJECT);
            return false;
        }
    }

    // TODO: Дескрипторы

    // Создание конвейера
    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = VkAPI->FramebufferHeight;
    viewport.width = VkAPI->FramebufferWidth;
    viewport.height = -VkAPI->FramebufferHeight;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Ножницы
    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = VkAPI->FramebufferWidth;
    scissor.extent.height = VkAPI->FramebufferHeight;

    // Атрибуты
    u32 offset = 0;
    const i32 AttributeCount = 1;
    VkVertexInputAttributeDescription AttributeDescriptions[AttributeCount];
    // Позиция
    VkFormat formats[AttributeCount] = {
        VK_FORMAT_R32G32B32_SFLOAT
    };
    u64 sizes[AttributeCount] = {
        sizeof(Vector3D<f32>)
    };
    for (u32 i = 0; i < AttributeCount; ++i) {
        AttributeDescriptions[i].binding = 0;   // индекс привязки - должен соответствовать описанию привязки
        AttributeDescriptions[i].location = i;  // атрибут местоположения
        AttributeDescriptions[i].format = formats[i];
        AttributeDescriptions[i].offset = offset;
        offset += sizes[i];
    }

    // TODO: Макеты набора дескрипторов.

    // Этапы
    // ПРИМЕЧАНИЕ: Должно соответствовать количеству shader->stages.
    VkPipelineShaderStageCreateInfo StageCreateInfos[OBJECT_SHADER_STAGE_COUNT];
    MMemory::ZeroMem(StageCreateInfos, sizeof(StageCreateInfos));
    for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
        StageCreateInfos[i].sType = stages[i].ShaderStageCreateInfo.sType;
        StageCreateInfos[i] = stages[i].ShaderStageCreateInfo;
    }

    if (!pipeline.Create(
            VkAPI,
            &VkAPI->MainRenderpass,
            AttributeCount,
            AttributeDescriptions,
            0,
            0,
            OBJECT_SHADER_STAGE_COUNT,
            StageCreateInfos,
            viewport,
            scissor,
            false/*,
            &OutShader->pipeline*/)) {
        MERROR("Не удалось загрузить графический конвейер для объектного шейдера.");
        return false;
    }

    return true;
}

void VulkanObjectShader::DestroyShaderModule(VulkanAPI *VkAPI)
{
    pipeline.Destroy(VkAPI);
    
    // Уничтожьте шейдерные модули.
    for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
        vkDestroyShaderModule(VkAPI->Device->LogicalDevice, stages[i].handle, VkAPI->allocator);
        stages[i].handle = 0;
    }
}

void VulkanObjectShader::Use(VulkanAPI *VkAPI)
{
}
