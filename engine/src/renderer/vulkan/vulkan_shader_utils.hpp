#pragma once

#include "shaders/vulkan_material_shader.hpp"

namespace VulkanShadersUtil
{
    bool CreateShaderModule(
    VulkanAPI* VkAPI,
    const char* name,
    const char* TypeStr,
    VkShaderStageFlagBits ShaderStageFlag,
    u32 StageIndex,
    VulkanShaderStage* ShaderStages
    );
    
} // namespace VulkanShadersUtil