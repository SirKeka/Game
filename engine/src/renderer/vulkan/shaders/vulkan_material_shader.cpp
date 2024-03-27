#include "vulkan_material_shader.hpp"
#include "renderer/vulkan/vulkan_api.hpp"
#include "renderer/vulkan/vulkan_shader_utils.hpp"
#include "systems/material_system.hpp"

#define BUILTIN_SHADER_NAME_OBJECT "Builtin.MaterialShader"

bool VulkanMaterialShader::Create(VulkanAPI *VkAPI)
{
    // Инициализация модуля шейдера на каждом этапе.
    char StageTypeStrs[MATERIAL_SHADER_STAGE_COUNT][5] = {"vert", "frag"};
    VkShaderStageFlagBits StageTypes[MATERIAL_SHADER_STAGE_COUNT] = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};

    for (u32 i = 0; i < MATERIAL_SHADER_STAGE_COUNT; ++i) {
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
    VK_CHECK(vkCreateDescriptorSetLayout(VkAPI->Device.LogicalDevice, &GlobalLayoutInfo, VkAPI->allocator, &GlobalDescriptorSetLayout));

    // Глобальный пул дескрипторов: Используется для глобальных элементов, таких как матрица вида/проекции.
    VkDescriptorPoolSize GlobalPoolSize;
    GlobalPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    GlobalPoolSize.descriptorCount = VkAPI->swapchain.ImageCount;

    VkDescriptorPoolCreateInfo GlobalPoolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    GlobalPoolInfo.poolSizeCount = 1;
    GlobalPoolInfo.pPoolSizes = &GlobalPoolSize;
    GlobalPoolInfo.maxSets = VkAPI->swapchain.ImageCount;
    VK_CHECK(vkCreateDescriptorPool(VkAPI->Device.LogicalDevice, &GlobalPoolInfo, VkAPI->allocator, &GlobalDescriptorPool));

    this->SamplerUses[0] = TextureUse::MapDiffuse;

    // Локальные/объектные дескрипторы
    VkDescriptorType DescriptorTypes[VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT] = {
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,          // Привязка 0 - однородный буфер(uniform buffer)
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  // Привязка 1 - Diffuse sampler layout.
    };
    VkDescriptorSetLayoutBinding bindings[VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT] {};
    for (u32 i = 0; i < VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT; ++i) {
        bindings[i].binding = i;
        bindings[i].descriptorCount = 1;
        bindings[i].descriptorType = DescriptorTypes[i];
        bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    VkDescriptorSetLayoutCreateInfo LayoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    LayoutInfo.bindingCount = VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT;
    LayoutInfo.pBindings = bindings;
    VK_CHECK(vkCreateDescriptorSetLayout(VkAPI->Device.LogicalDevice, &LayoutInfo, 0, &this->ObjectDescriptorSetLayout));

    // Пул локальных/объектных дескрипторов: Используется для специфичных для объекта элементов, таких как рассеянный цвет
    VkDescriptorPoolSize ObjectPoolSizes[2];
    // Первый раздел будет использоваться для универсальных буферов.
    ObjectPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ObjectPoolSizes[0].descriptorCount = VULKAN_MAX_MATERIAL_COUNT;
    // Второй раздел будет использоваться для образцов изображений.
    ObjectPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ObjectPoolSizes[1].descriptorCount = VULKAN_MATERIAL_SHADER_SAMPLER_COUNT * VULKAN_MAX_MATERIAL_COUNT;

    VkDescriptorPoolCreateInfo ObjectPoolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    ObjectPoolInfo.poolSizeCount = 2;
    ObjectPoolInfo.pPoolSizes = ObjectPoolSizes;
    ObjectPoolInfo.maxSets = VULKAN_MAX_MATERIAL_COUNT;
    ObjectPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    // Создание пула дескрипторов объектов.
    VK_CHECK(vkCreateDescriptorPool(VkAPI->Device.LogicalDevice, &ObjectPoolInfo, VkAPI->allocator, &this->ObjectDescriptorPool));

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
#define ATTRIBUTE_COUNT 2
    VkVertexInputAttributeDescription AttributeDescriptions[ATTRIBUTE_COUNT];
    // Позиция, координаты текстуры
    VkFormat formats[ATTRIBUTE_COUNT] = {
        VK_FORMAT_R32G32B32_SFLOAT,
        VK_FORMAT_R32G32_SFLOAT};
    u64 sizes[ATTRIBUTE_COUNT] = {
        sizeof(Vector3D<f32>),
        sizeof(Vector2D<f32>)
    };
    for (u32 i = 0; i < ATTRIBUTE_COUNT; ++i) {
        AttributeDescriptions[i].binding = 0;   // индекс привязки - должен соответствовать описанию привязки
        AttributeDescriptions[i].location = i;  // атрибут местоположения
        AttributeDescriptions[i].format = formats[i];
        AttributeDescriptions[i].offset = offset;
        offset += sizes[i];
    }

    // Макеты набора дескрипторов.
    const i32 DescriptorSetLayoutCount = 2;
    VkDescriptorSetLayout layouts[2] = {
        GlobalDescriptorSetLayout,
        ObjectDescriptorSetLayout};

    // Этапы
    // ПРИМЕЧАНИЕ: Должно соответствовать количеству shader->stages.
    VkPipelineShaderStageCreateInfo StageCreateInfos[MATERIAL_SHADER_STAGE_COUNT]{};
    //MMemory::ZeroMem(StageCreateInfos, sizeof(StageCreateInfos));
    for (u32 i = 0; i < MATERIAL_SHADER_STAGE_COUNT; ++i) {
        StageCreateInfos[i].sType = stages[i].ShaderStageCreateInfo.sType;
        StageCreateInfos[i] = stages[i].ShaderStageCreateInfo;
    }

    if (!pipeline.Create(
            VkAPI,
            &VkAPI->MainRenderpass,
            ATTRIBUTE_COUNT,
            AttributeDescriptions,
            DescriptorSetLayoutCount,
            layouts,
            MATERIAL_SHADER_STAGE_COUNT,
            StageCreateInfos,
            viewport,
            scissor,
            false)) {
        MERROR("Не удалось загрузить графический конвейер для объектного шейдера.");
        return false;
    }

    // Создание единого буфер.
    u32 DeviceLocalBits = VkAPI->Device.SupportsDeviceLocalHostVisible ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0;
    if (!GlobalUniformBuffer.Create(
            VkAPI,
            sizeof(GlobalUniformObject),
            static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // | DeviceLocalBits,
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
    VK_CHECK(vkAllocateDescriptorSets(VkAPI->Device.LogicalDevice, &AllocInfo, GlobalDescriptorSets));

    // Создание object uniform buffer.
    if (!ObjectUniformBuffer.Create(
            VkAPI,
            sizeof(MaterialUniformObject) * VULKAN_MAX_MATERIAL_COUNT,
            static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT),
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            true)) {
        MERROR("Не удалось создать буфер экземпляра материала для шейдера.");
        return false;
    }

    return true;
}

void VulkanMaterialShader::DestroyShaderModule(VulkanAPI *VkAPI)
{
    vkDestroyDescriptorPool(VkAPI->Device.LogicalDevice, ObjectDescriptorPool, VkAPI->allocator);
    vkDestroyDescriptorSetLayout(VkAPI->Device.LogicalDevice, ObjectDescriptorSetLayout, VkAPI->allocator);

    // Уничтожьте единый буфер.
    GlobalUniformBuffer.Destroy(VkAPI);
    ObjectUniformBuffer.Destroy(VkAPI);

    // Уничтожение конвейера.
    pipeline.Destroy(VkAPI);

    // Уничтожение глобального пула дескриптора.
    vkDestroyDescriptorPool(VkAPI->Device.LogicalDevice, GlobalDescriptorPool, VkAPI->allocator);

    // Уничтожение макетов набора дескриптора.
    vkDestroyDescriptorSetLayout(VkAPI->Device.LogicalDevice, GlobalDescriptorSetLayout, VkAPI->allocator);
    
    // Уничтожьте шейдерные модули.
    for (u32 i = 0; i < MATERIAL_SHADER_STAGE_COUNT; ++i) {
        vkDestroyShaderModule(VkAPI->Device.LogicalDevice, stages[i].handle, VkAPI->allocator);
        stages[i].handle = 0;
    }
}

void VulkanMaterialShader::Use(VulkanAPI *VkAPI)
{
    u32 ImageIndex = VkAPI->ImageIndex;
    pipeline.Bind(&VkAPI->GraphicsCommandBuffers[ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS);
}

void VulkanMaterialShader::UpdateGlobalState(VulkanAPI *VkAPI, f32 DeltaTime)
{
    u32 ImageIndex = VkAPI->ImageIndex;
    VkCommandBuffer CommandBuffer = VkAPI->GraphicsCommandBuffers[ImageIndex].handle;
    VkDescriptorSet GlobalDescriptor = GlobalDescriptorSets[ImageIndex];

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

    vkUpdateDescriptorSets(VkAPI->Device.LogicalDevice, 1, &DescriptorWrite, 0, 0);

    // Привяжите глобальный набор дескрипторов, подлежащий обновлению.
    vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.PipelineLayout, 0, 1, &GlobalDescriptor, 0, 0);
}

void VulkanMaterialShader::UpdateObject(VulkanAPI *VkAPI, const GeometryRenderData& data)
{
    u32 ImageIndex = VkAPI->ImageIndex;
    VkCommandBuffer CommandBuffer = VkAPI->GraphicsCommandBuffers[ImageIndex].handle;

    vkCmdPushConstants(CommandBuffer, pipeline.PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Matrix4D), &data.model);

    // Получить данные о материале.
    VulkanMaterialShaderInstanceState* ObjectState = &InstanceStates[data.material->InternalId];
    VkDescriptorSet ObjectDescriptorSet = ObjectState->DescriptorSets[ImageIndex];

    // TODO: если требуется обновление
    VkWriteDescriptorSet DescriptorWrites[VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT] {};
    u32 DescriptorCount = 0;
    u32 DescriptorIndex = 0;

    // Дескриптор 0 - Uniform buffer
    u32 range = sizeof(MaterialUniformObject);
    u64 offset = sizeof(MaterialUniformObject) * data.material->InternalId;  // а также индекс массива.
    MaterialUniformObject obo;

    // TODO: получить рассеянный цвет от материала.
    // static f32 accumulator = 0.0f;
    // accumulator += VkAPI->FrameDeltaTime;
    // f32 s = (Math::sin(accumulator) + 1.0f) / 2.0f;  // масштаб от -1, 1 до 0, 1
    // obo.DiffuseColor = Vector4D<f32>(s, s, s, 1.0f);
    obo.DiffuseColor = data.material->DiffuseColour;

    // Загрузите данные в буфер.
    ObjectUniformBuffer.LoadData(VkAPI, offset, range, 0, &obo);

    // Делайте это только в том случае, если дескриптор еще не был обновлен.
    u32* GlobalUBOGeneration = &ObjectState->DescriptorStates[DescriptorIndex].generations[ImageIndex];
    if (*GlobalUBOGeneration == INVALID_ID || *GlobalUBOGeneration != data.material->generation) {
        VkDescriptorBufferInfo BufferInfo;
        BufferInfo.buffer = this->ObjectUniformBuffer.handle;
        BufferInfo.offset = offset;
        BufferInfo.range = range;

        VkWriteDescriptorSet descriptor = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        descriptor.dstSet = ObjectDescriptorSet;
        descriptor.dstBinding = DescriptorIndex;
        descriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor.descriptorCount = 1;
        descriptor.pBufferInfo = &BufferInfo;

        DescriptorWrites[DescriptorCount] = descriptor;
        DescriptorCount++;

        // Обновите генерацию кадра. В данном случае он нужен только один раз, поскольку это буфер.
        GlobalUBOGeneration = &data.material->generation;
    }
    DescriptorIndex++;

    // Samplers.
    const u32 SamplerCount = 1;
    VkDescriptorImageInfo ImageInfos[1];
    for (u32 SamplerIndex = 0; SamplerIndex < SamplerCount; ++SamplerIndex) {
        TextureUse use = SamplerUses[SamplerIndex];
        Texture* t = nullptr;

        switch (use)
        {
        case TextureUse::MapDiffuse:
            t = data.material->DiffuseMap.texture;
            break;
        
        default: MFATAL("Невозможно связать сэмплер с неизвестным использованием.");
            break;
        }

        u32* DescriptorGeneration = &ObjectState->DescriptorStates[DescriptorIndex].generations[ImageIndex];
        u32* DescriptorID = &ObjectState->DescriptorStates[DescriptorIndex].ids[ImageIndex];

        // Если текстура еще не была загружена, используйте значение по умолчанию.
        if (t->generation == INVALID_ID) {
            t = TextureSystem::Instance()->GetDefaultTexture();
            // Сбросьте генерацию дескриптора, если используется текстура по умолчанию.
            *DescriptorGeneration = INVALID_ID;

        }

        // Сначала проверьте, нуждается ли дескриптор в обновлении.
        if (t && (*DescriptorID != t->id || *DescriptorGeneration != t->generation || *DescriptorGeneration == INVALID_ID)) {
            VulkanTextureData* InternalData = /*(vulkan_texture_data*)*/t->Data;

            // Назначьте вид и сэмплер.
            ImageInfos[SamplerIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            ImageInfos[SamplerIndex].imageView = InternalData->image.view;
            ImageInfos[SamplerIndex].sampler = InternalData->sampler;

            VkWriteDescriptorSet descriptor = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            descriptor.dstSet = ObjectDescriptorSet;
            descriptor.dstBinding = DescriptorIndex;
            descriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptor.descriptorCount = 1;
            descriptor.pImageInfo = &ImageInfos[SamplerIndex];

            DescriptorWrites[DescriptorCount] = descriptor;
            DescriptorCount++;

            // Синхронизировать генерацию кадров, если не используется текстура по умолчанию.
            if (t->generation != INVALID_ID) {
                *DescriptorGeneration = t->generation;
                *DescriptorID = t->id; 
            }
            DescriptorIndex++;
        }
    }

    if (DescriptorCount > 0) {
        vkUpdateDescriptorSets(VkAPI->Device.LogicalDevice, DescriptorCount, DescriptorWrites, 0, 0);
    }

    // Привяжите набор дескрипторов, который будет обновляться или на случай изменения шейдера.
    vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.PipelineLayout, 1, 1, &ObjectDescriptorSet, 0, 0);
}

bool VulkanMaterialShader::AcquireResources(VulkanAPI *VkAPI, Material* material)
{
    // TODO: free list
    material->InternalId = this->ObjectUniformBufferIndex;
    this->ObjectUniformBufferIndex++;

    VulkanMaterialShaderInstanceState* InstanceState = &this->InstanceStates[material->InternalId];
    for (u32 i = 0; i < VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT; ++i) {
        for (u32 j = 0; j < 3; ++j) {
            InstanceState->DescriptorStates[i].generations[j] = INVALID_ID;
            InstanceState->DescriptorStates[i].ids[j] = INVALID_ID;
        }
    }

    // Распределение наборов дескрипторов.
    VkDescriptorSetLayout layouts[3] = {
        this->ObjectDescriptorSetLayout,
        this->ObjectDescriptorSetLayout,
        this->ObjectDescriptorSetLayout};

    VkDescriptorSetAllocateInfo AllocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    AllocInfo.descriptorPool = this->ObjectDescriptorPool;
    AllocInfo.descriptorSetCount = 3;  // один за кадр
    AllocInfo.pSetLayouts = layouts;
    VkResult result = vkAllocateDescriptorSets(VkAPI->Device.LogicalDevice, &AllocInfo, InstanceState->DescriptorSets);
    if (result != VK_SUCCESS) {
        MERROR("Ошибка распределения наборов дескрипторов в шейдере!");
        return false;
    }

    return true;
}

void VulkanMaterialShader::ReleaseResources(VulkanAPI *VkAPI, Material* material)
{
    VulkanMaterialShaderInstanceState* InstanceState = &InstanceStates[material->InternalId];

    const u32 DescriptorSetCount = 3;
    // Освободите наборы дескрипторов объектов.
    VkResult result = vkFreeDescriptorSets(VkAPI->Device.LogicalDevice, ObjectDescriptorPool, DescriptorSetCount, InstanceState->DescriptorSets);
    if (result != VK_SUCCESS) {
        MERROR("Ошибка при освобождении наборов дескрипторов объектных шейдеров!");
    }
    for (u32 i = 0; i < VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT; ++i) {
        for (u32 j = 0; j < 3; ++j) {
            InstanceState->DescriptorStates[i].generations[j] = INVALID_ID;
            InstanceState->DescriptorStates[i].ids[j] = INVALID_ID;
        }
    }
    material->InternalId = INVALID_ID;
}
