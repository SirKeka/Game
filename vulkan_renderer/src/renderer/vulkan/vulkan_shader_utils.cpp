#include "vulkan_shader_utils.hpp"

#include "vulkan_api.hpp"

#include "systems/resource_system.h"

namespace VulkanShadersUtil
{
    bool CreateShaderModule(VulkanAPI *VkAPI, 
                            const char *name, 
                            const char *TypeStr, 
                            VkShaderStageFlagBits ShaderStageFlag, 
                            u32 StageIndex, 
                            VulkanShaderStage *ShaderStages)
    {
        // Имя файла сборки, которое также будет использоваться в качестве имени ресурса.
        char FileName[512];
        MString::Format(FileName, "shaders/%s.%s.spv", name, TypeStr);

        // Чтение ресурса.
        BinaryResource BinRes;
        if (!ResourceSystem::Load(FileName, eResource::Type::Binary, nullptr, BinRes)) {
            MERROR("Невозможно прочитать шейдерный модуль: %s.", FileName);
            return false;
        }

        MemorySystem::ZeroMem(&ShaderStages[StageIndex].CreateInfo, sizeof(VkShaderModuleCreateInfo));
        ShaderStages[StageIndex].CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        // Используйте размер и данные ресурса напрямую.
        ShaderStages[StageIndex].CreateInfo.codeSize = BinRes.data.Length();
        ShaderStages[StageIndex].CreateInfo.pCode = reinterpret_cast<u32*>(BinRes.data.Data());

        VK_CHECK(vkCreateShaderModule(
            VkAPI->Device.LogicalDevice,
            &ShaderStages[StageIndex].CreateInfo,
            VkAPI->allocator,
            &ShaderStages[StageIndex].handle));

        // Информация о стадии шейдера
        MemorySystem::ZeroMem(&ShaderStages[StageIndex].ShaderStageCreateInfo, sizeof(VkPipelineShaderStageCreateInfo));
        ShaderStages[StageIndex].ShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStages[StageIndex].ShaderStageCreateInfo.stage = ShaderStageFlag;
        ShaderStages[StageIndex].ShaderStageCreateInfo.module = ShaderStages[StageIndex].handle;
        ShaderStages[StageIndex].ShaderStageCreateInfo.pName = "main";

        return true;
    }   

} // namespace VulkanShadersUtil
