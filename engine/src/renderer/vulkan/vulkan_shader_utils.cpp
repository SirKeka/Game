#include "vulkan_shader_utils.hpp"

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
        char file_name[512];
        StringFormat(file_name, "assets/shaders/%s.%s.spv", name, TypeStr);

        MMemory::ZeroMem(&ShaderStages[StageIndex].CreateInfo, sizeof(VkShaderModuleCreateInfo));
        ShaderStages[StageIndex].CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

        // Получить дескриптор файла.
        file_handle handle;
        if (!filesystem_open(file_name, FILE_MODE_READ, true, &handle)) {
            MERROR("Unable to read shader module: %s.", file_name);
            return false;
        }

    // Прочитайте весь файл как двоичный.
    u64 size = 0;
    u8* file_buffer = 0;
    if (!filesystem_read_all_bytes(&handle, &file_buffer, &size)) {
        MERROR("Unable to binary read shader module: %s.", file_name);
        return false;
    }
    ShaderStages[StageIndex].CreateInfo.codeSize = size;
    ShaderStages[StageIndex].CreateInfo.pCode = (u32*)file_buffer;

    // Закройте файл.
    filesystem_close(&handle);

    VK_CHECK(vkCreateShaderModule(
        context->device.logical_device,
        &ShaderStages[StageIndex].CreateInfo,
        context->allocator,
        &ShaderStages[StageIndex].handle));

    // Информация о стадии шейдера
    MMemory::ZeroMem(&ShaderStages[StageIndex].ShaderStageCreateInfo, sizeof(VkPipelineShaderStageCreateInfo));
    ShaderStages[StageIndex].ShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStages[StageIndex].ShaderStageCreateInfo.stage = ShaderStageFlag;
    ShaderStages[StageIndex].ShaderStageCreateInfo.module = ShaderStages[StageIndex].handle;
    ShaderStages[StageIndex].ShaderStageCreateInfo.pName = "main";

    if (file_buffer) {
        MMemory::TFree<u8>(file_buffer, size, MEMORY_TAG_STRING);
        file_buffer = 0;
    }

    return true;
    }   

} // namespace VulkanShadersUtil
