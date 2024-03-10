#include "vulkan_object_shader.hpp"

#include "math/vector3d.hpp"
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

    // Глобальные дескрипторы
    VkDescriptorSetLayoutBinding GlobalUboLayoutBinding;
    GlobalUboLayoutBinding.binding = 0;
    GlobalUboLayoutBinding.descriptorCount = 1;
    GlobalUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    GlobalUboLayoutBinding.pImmutableSamplers = 0;
    GlobalUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo GlobalLayoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    GlobalLayoutInfo.bindingCount = 1;
    GlobalLayoutInfo.pBindings = &GlobalUboLayoutBinding;
    VK_CHECK(vkCreateDescriptorSetLayout(VkAPI->Device->LogicalDevice, &GlobalLayoutInfo, VkAPI->allocator, &GlobalDescriptorSetLayout));

    // Глобальный пул дескрипторов: Используется для глобальных элементов, таких как матрица вида/проекции.
    VkDescriptorPoolSize GlobalPoolSize;
    GlobalPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    GlobalPoolSize.descriptorCount = VkAPI->swapchain.ImageCount;

    VkDescriptorPoolCreateInfo GlobalPoolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    GlobalPoolInfo.poolSizeCount = 1;
    GlobalPoolInfo.pPoolSizes = &GlobalPoolSize;
    GlobalPoolInfo.maxSets = VkAPI->swapchain.ImageCount;
    VK_CHECK(vkCreateDescriptorPool(VkAPI->Device->LogicalDevice, &GlobalPoolInfo, VkAPI->allocator, &GlobalDescriptorPool));

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

    // Макеты набора дескрипторов.
    const i32 DescriptorSetLayoutCount = 1;
    VkDescriptorSetLayout layouts[1] = {
        GlobalDescriptorSetLayout};

    // Этапы
    // ПРИМЕЧАНИЕ: Должно соответствовать количеству shader->stages.
    VkPipelineShaderStageCreateInfo StageCreateInfos[OBJECT_SHADER_STAGE_COUNT]{};
    //MMemory::ZeroMem(StageCreateInfos, sizeof(StageCreateInfos));
    for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
        StageCreateInfos[i].sType = stages[i].ShaderStageCreateInfo.sType;
        StageCreateInfos[i] = stages[i].ShaderStageCreateInfo;
    }

    if (!pipeline.Create(
            VkAPI,
            &VkAPI->MainRenderpass,
            AttributeCount,
            AttributeDescriptions,
            DescriptorSetLayoutCount,
            layouts,
            OBJECT_SHADER_STAGE_COUNT,
            StageCreateInfos,
            viewport,
            scissor,
            false)) {
        MERROR("Не удалось загрузить графический конвейер для объектного шейдера.");
        return false;
    }

    // Создание единого буфер.
    if (!GlobalUniformBuffer.Create(
            VkAPI,
            sizeof(GlobalUniformObject),
            static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
            /*VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | */VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            true)) {
        MERROR("Не удалось создать буфер Vulkan для объектного шейдера.");
        return false;
    }

    // Выделение глобальных макетов дескрипторов.
    VkDescriptorSetLayout GlobalLayouts[3] = {
        GlobalDescriptorSetLayout,
        GlobalDescriptorSetLayout,
        GlobalDescriptorSetLayout};

    VkDescriptorSetAllocateInfo AllocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    AllocInfo.descriptorPool = GlobalDescriptorPool;
    AllocInfo.descriptorSetCount = 3;
    AllocInfo.pSetLayouts = GlobalLayouts;
    VK_CHECK(vkAllocateDescriptorSets(VkAPI->Device->LogicalDevice, &AllocInfo, GlobalDescriptorSets));

    return true;
}

void VulkanObjectShader::DestroyShaderModule(VulkanAPI *VkAPI)
{
    // Уничтожьте единый буфер.
    GlobalUniformBuffer.Destroy(VkAPI);

    // Уничтожение конвейера.
    pipeline.Destroy(VkAPI);

    // Уничтожение глобального пула дескриптора.
    vkDestroyDescriptorPool(VkAPI->Device->LogicalDevice, GlobalDescriptorPool, VkAPI->allocator);

    // Уничтожение макетов набора дескриптора.
    vkDestroyDescriptorSetLayout(VkAPI->Device->LogicalDevice, GlobalDescriptorSetLayout, VkAPI->allocator);
    
    // Уничтожьте шейдерные модули.
    for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
        vkDestroyShaderModule(VkAPI->Device->LogicalDevice, stages[i].handle, VkAPI->allocator);
        stages[i].handle = 0;
    }
}

void VulkanObjectShader::Use(VulkanAPI *VkAPI)
{
    u32 ImageIndex = VkAPI->ImageIndex;
    pipeline.Bind(&VkAPI->GraphicsCommandBuffers[ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS);
}

void VulkanObjectShader::UpdateGlobalState(VulkanAPI *VkAPI)
{
    u32 ImageIndex = VkAPI->ImageIndex;
    VkCommandBuffer CommandBuffer = VkAPI->GraphicsCommandBuffers[ImageIndex].handle;
    VkDescriptorSet GlobalDescriptor = GlobalDescriptorSets[ImageIndex];

    // Привяжите глобальный набор дескрипторов, подлежащий обновлению.
    vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.PipelineLayout, 0, 1, &GlobalDescriptor, 0, 0);

    // Сконфигурируйте дескрипторы для данного индекса.
    u32 range = sizeof(GlobalUniformObject);
    u64 offset = 0;

    // Копирование данных в буфер
    GlobalUniformBuffer.LoadData(VkAPI, offset, range, 0, &GlobalUObj);

    VkDescriptorBufferInfo bufferInfo;
    bufferInfo.buffer = GlobalUniformBuffer.handle;
    bufferInfo.offset = offset;
    bufferInfo.range = range;

    // Обновление наборов дескрипторов.
    VkWriteDescriptorSet DescriptorWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    DescriptorWrite.dstSet = GlobalDescriptorSets[ImageIndex];
    DescriptorWrite.dstBinding = 0;
    DescriptorWrite.dstArrayElement = 0;
    DescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    DescriptorWrite.descriptorCount = 1;
    DescriptorWrite.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(VkAPI->Device->LogicalDevice, 1, &DescriptorWrite, 0, 0);
}
