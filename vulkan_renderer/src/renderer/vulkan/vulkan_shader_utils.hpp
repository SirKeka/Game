#pragma once

#include "vulkan_shader.hpp"

namespace VulkanShadersUtil
{
    bool CreateShaderModule(
    class VulkanAPI* VkAPI,
    const char* name,
    const char* TypeStr,
    VkShaderStageFlagBits ShaderStageFlag,
    u32 StageIndex,
    VulkanShaderStage* ShaderStages
    );
    
} // namespace VulkanShadersUtil