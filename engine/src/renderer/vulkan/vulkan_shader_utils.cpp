#include "vulkan_shader_utils.hpp"

#include "vulkan_api.hpp"

#include "platform/filesystem.hpp"

namespace VulkanShadersUtil
{
    bool CreateShaderModule(VulkanAPI *VkAPI, 
                            const char *name, 
                            const char *TypeStr, 
                            VkShaderStageFlagBits ShaderStageFlag, 
                            u32 StageIndex, 
                            VulkanShaderStage *ShaderStages)
    {
        // Имя файла сборки.
        char FileName[512];
        MString::Format(FileName, "assets/shaders/%s.%s.spv", name, TypeStr);

        MMemory::ZeroMem(&ShaderStages[StageIndex].CreateInfo, sizeof(VkShaderModuleCreateInfo));
        ShaderStages[StageIndex].CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

        // Получить дескриптор файла.
        FileHandle handle;
        if (!Filesystem::Open(FileName, FILE_MODE_READ, true, &handle)) {
            MERROR("Невозможно прочитать шейдерный модуль: %s.", FileName);
            return false;
        }

    // Прочитайте весь файл как двоичный.
    u64 size = 0;
    u8* FileBuffer = 0;
    if (!Filesystem::ReadAllBytes(&handle, FileBuffer, &size)) {
        MERROR("Невозможно выполнить двоичное чтение модуля шейдера.: %s.", FileName);
        return false;
    }
    ShaderStages[StageIndex].CreateInfo.codeSize = size;
    ShaderStages[StageIndex].CreateInfo.pCode = (u32*)FileBuffer;

    // Закройте файл.
    Filesystem::Close(&handle);

    VK_CHECK(vkCreateShaderModule(
        VkAPI->Device.LogicalDevice,
        &ShaderStages[StageIndex].CreateInfo,
        VkAPI->allocator,
        &ShaderStages[StageIndex].handle));

    // Информация о стадии шейдера
    MMemory::ZeroMem(&ShaderStages[StageIndex].ShaderStageCreateInfo, sizeof(VkPipelineShaderStageCreateInfo));
    ShaderStages[StageIndex].ShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStages[StageIndex].ShaderStageCreateInfo.stage = ShaderStageFlag;
    ShaderStages[StageIndex].ShaderStageCreateInfo.module = ShaderStages[StageIndex].handle;
    ShaderStages[StageIndex].ShaderStageCreateInfo.pName = "main";

    if (FileBuffer) {
        MMemory::TFree<u8>(FileBuffer, size, MEMORY_TAG_STRING);
        FileBuffer = 0;
    }

    return true;
    }   

} // namespace VulkanShadersUtil
