#include "vulkan_object_shader.hpp"

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

    // Дескрипторы

    return true;
}

void VulkanObjectShader::Use(VulkanAPI *VkAPI)
{
}
