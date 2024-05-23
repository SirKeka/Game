#include "vulkan_ui_shader.hpp"
#include "renderer/vulkan/vulkan_api.hpp"
#include "renderer/vulkan/vulkan_shader_utils.hpp"

#define BUILTIN_SHADER_NAME_UI "Builtin.UIShader"

bool VulkanUI_Shader::Create(VulkanAPI *VkAPI)
{
    // Инициализация шейдерного модуля для каждого этапа.
    char StageTypeStrs[UI_SHADER_STAGE_COUNT][5] = {"vert", "frag"};
    VkShaderStageFlagBits StageTypes[UI_SHADER_STAGE_COUNT] = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};

    for (u32 i = 0; i < UI_SHADER_STAGE_COUNT; ++i) {
        if (!VulkanShadersUtil::CreateShaderModule(VkAPI, BUILTIN_SHADER_NAME_UI, StageTypeStrs[i], StageTypes[i], i, this->stages)) {
            MERROR("Невозможно создать шейдерный модуль %s для '%s'.", StageTypeStrs[i], BUILTIN_SHADER_NAME_UI);
            return false;
        }
    }

    // Глобальные дескрипторы
    VkDescriptorSetLayoutBinding GlobalUniformObjectLayoutBinding;
    GlobalUniformObjectLayoutBinding.binding = 0;
    GlobalUniformObjectLayoutBinding.descriptorCount = 1;
    GlobalUniformObjectLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    GlobalUniformObjectLayoutBinding.pImmutableSamplers = 0;
    GlobalUniformObjectLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo GlobalLayoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    GlobalLayoutInfo.bindingCount = 1;
    GlobalLayoutInfo.pBindings = &GlobalUniformObjectLayoutBinding;
    VK_CHECK(vkCreateDescriptorSetLayout(VkAPI->Device.LogicalDevice, &GlobalLayoutInfo, VkAPI->allocator, &this->GlobalDescriptorSetLayout));

    // Глобальный пул дескрипторов: используется для глобальных элементов, таких как матрица вида/проекции.
    VkDescriptorPoolSize GlobalPoolSize;
    GlobalPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    GlobalPoolSize.descriptorCount = VkAPI->swapchain.ImageCount;

    VkDescriptorPoolCreateInfo GlobalPoolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    GlobalPoolInfo.poolSizeCount = 1;
    GlobalPoolInfo.pPoolSizes = &GlobalPoolSize;
    GlobalPoolInfo.maxSets = VkAPI->swapchain.ImageCount;
    VK_CHECK(vkCreateDescriptorPool(VkAPI->Device.LogicalDevice, &GlobalPoolInfo, VkAPI->allocator, &this->GlobalDescriptorPool));

    // Sampler uses.
    this->SamplerUses[0] = TextureUse::MapDiffuse;

    // Локальные/объектные дескрипторы
    VkDescriptorType DescriptorTypes[VULKAN_UI_SHADER_DESCRIPTOR_COUNT] = {
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,          // Binding 0 - uniform buffer
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  // Binding 1 - Diffuse sampler layout.
    };
    VkDescriptorSetLayoutBinding bindings[VULKAN_UI_SHADER_DESCRIPTOR_COUNT] {};
    for (u32 i = 0; i < VULKAN_UI_SHADER_DESCRIPTOR_COUNT; ++i) {
        bindings[i].binding = i;
        bindings[i].descriptorCount = 1;
        bindings[i].descriptorType = DescriptorTypes[i];
        bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    VkDescriptorSetLayoutCreateInfo LayoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    LayoutInfo.bindingCount = VULKAN_UI_SHADER_DESCRIPTOR_COUNT;
    LayoutInfo.pBindings = bindings;
    VK_CHECK(vkCreateDescriptorSetLayout(VkAPI->Device.LogicalDevice, &LayoutInfo, 0, &this->ObjectDescriptorSetLayout));

    // Пул дескрипторов локальных/объектных: используется для элементов, специфичных для объекта, таких как диффузный цвет.
    VkDescriptorPoolSize ObjectPoolSizes[2];
    // Первый раздел будет использоваться для универсальных буферов.
    ObjectPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ObjectPoolSizes[0].descriptorCount = VULKAN_MAX_UI_COUNT;
    // Второй раздел будет использоваться для сэмплеров изображений.
    ObjectPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ObjectPoolSizes[1].descriptorCount = VULKAN_UI_SHADER_SAMPLER_COUNT * VULKAN_MAX_UI_COUNT;

    VkDescriptorPoolCreateInfo ObjectPoolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    ObjectPoolInfo.poolSizeCount = 2;
    ObjectPoolInfo.pPoolSizes = ObjectPoolSizes;
    ObjectPoolInfo.maxSets = VULKAN_MAX_UI_COUNT;
    ObjectPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    // Создайте пул дескрипторов объектов.
    VK_CHECK(vkCreateDescriptorPool(VkAPI->Device.LogicalDevice, &ObjectPoolInfo, VkAPI->allocator, &this->ObjectDescriptorPool));

    // Создание конвейера
    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = static_cast<f32>(VkAPI->FramebufferHeight);
    viewport.width = static_cast<f32>(VkAPI->FramebufferWidth);
    viewport.height = -(static_cast<f32>(VkAPI->FramebufferHeight));
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
    // Position, texcoord
    VkFormat formats[ATTRIBUTE_COUNT] = {
        VK_FORMAT_R32G32_SFLOAT,
        VK_FORMAT_R32G32_SFLOAT};
    u64 sizes[ATTRIBUTE_COUNT] = {
        sizeof(Vector2D<f32>),
        sizeof(Vector2D<f32>)};
    for (u32 i = 0; i < ATTRIBUTE_COUNT; ++i) {
        AttributeDescriptions[i].binding = 0;   // индекс привязки - должен соответствовать описанию привязки
        AttributeDescriptions[i].location = i;  // местоположение атрибута
        AttributeDescriptions[i].format = formats[i];
        AttributeDescriptions[i].offset = offset;
        offset += sizes[i];
    }

    // Макеты набора дескрипторов.
    const i32 DescriptorSetLayoutCount = 2;
    VkDescriptorSetLayout layouts[2] = {
        this->GlobalDescriptorSetLayout,
        this->ObjectDescriptorSetLayout};

    // Этапы
    // ПРИМЕЧАНИЕ. Должно совпадать с количеством this->stages.
    VkPipelineShaderStageCreateInfo StageCreateInfos[UI_SHADER_STAGE_COUNT]{};
    for (u32 i = 0; i < UI_SHADER_STAGE_COUNT; ++i) {
        StageCreateInfos[i].sType = this->stages[i].ShaderStageCreateInfo.sType;
        StageCreateInfos[i] = this->stages[i].ShaderStageCreateInfo;
    }

    if (!this->pipeline.Create(
            VkAPI,
            &VkAPI->UI_Renderpass,
            sizeof(Vertex2D),
            ATTRIBUTE_COUNT,
            AttributeDescriptions,
            DescriptorSetLayoutCount,
            layouts,
            UI_SHADER_STAGE_COUNT,
            StageCreateInfos,
            viewport,
            scissor,
            false,
            false)) {
        MERROR("Не удалось загрузить графический конвейер для объектного шейдера.");
        return false;
    }

    // Создайте универсальный буфер.
    if (!this->GlobalUniformBuffer.Create(
            VkAPI,
            sizeof(VulkanUI_ShaderGlobalUniformObject),
            static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT),
            /*VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | */VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            true)) {
        MERROR("Не удалось создать буфер Vulkan для шейдера объекта.");
        return false;
    }

    // Выделение наборов глобальных дескрипторов.
    VkDescriptorSetLayout GlobalLayouts[3] = {
        this->GlobalDescriptorSetLayout,
        this->GlobalDescriptorSetLayout,
        this->GlobalDescriptorSetLayout};

    VkDescriptorSetAllocateInfo AllocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    AllocInfo.descriptorPool = this->GlobalDescriptorPool;
    AllocInfo.descriptorSetCount = VkAPI->swapchain.ImageCount;
    AllocInfo.pSetLayouts = GlobalLayouts;
    VK_CHECK(vkAllocateDescriptorSets(VkAPI->Device.LogicalDevice, &AllocInfo, this->GlobalDescriptorSets));

    // Создайте универсальный буфер объекта.
    if (!this->ObjectUniformBuffer.Create(
            VkAPI,
            sizeof(VulkanUI_ShaderInstanceUniformObject) * VULKAN_MAX_UI_COUNT,
            static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT),
            /*VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | */VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            true)) {
        MERROR("Не удалось создать буфер экземпляра пользовательского интерфейса для шейдера.");
        return false;
    }

    return true;
}

void VulkanUI_Shader::Destroy(VulkanAPI *VkAPI)
{
    VkDevice& LogicalDevice = VkAPI->Device.LogicalDevice;

    vkDestroyDescriptorPool(LogicalDevice, this->ObjectDescriptorPool, VkAPI->allocator);
    vkDestroyDescriptorSetLayout(LogicalDevice, this->ObjectDescriptorSetLayout, VkAPI->allocator);

    // Уничтожьте универсальные буферы.
    this->GlobalUniformBuffer.Destroy(VkAPI);
    this->ObjectUniformBuffer.Destroy(VkAPI);

    // Уничтожение конвейера.
    this->pipeline.Destroy(VkAPI);

    // Уничтожение пула глобальных дескрипторов.
    vkDestroyDescriptorPool(LogicalDevice, this->GlobalDescriptorPool, VkAPI->allocator);

    // Уничтожение макетов наборов дескрипторов.
    vkDestroyDescriptorSetLayout(LogicalDevice, this->GlobalDescriptorSetLayout, VkAPI->allocator);

    // Уничтожьте эти модули.
    for (u32 i = 0; i < UI_SHADER_STAGE_COUNT; ++i) {
        vkDestroyShaderModule(VkAPI->Device.LogicalDevice, this->stages[i].handle, VkAPI->allocator);
        this->stages[i].handle = 0;
    }
}

void VulkanUI_Shader::Use(VulkanAPI *VkAPI)
{
    u32& ImageIndex = VkAPI->ImageIndex;
    this->pipeline.Bind(&VkAPI->GraphicsCommandBuffers[ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS);
}

void VulkanUI_Shader::UpdateGlobalState(VulkanAPI *VkAPI, f32 DeltaTime)
{
    u32& ImageIndex = VkAPI->ImageIndex;
    VkCommandBuffer& CommandBuffer = VkAPI->GraphicsCommandBuffers[ImageIndex].handle;
    VkDescriptorSet& GlobalDescriptor = this->GlobalDescriptorSets[ImageIndex];

    // Настройте дескрипторы для данного индекса.
    u32 range = sizeof(VulkanUI_ShaderGlobalUniformObject);
    u64 offset = 0;

    // Копировать данные в буфер
    this->GlobalUniformBuffer.LoadData(VkAPI, offset, range, 0, &this->GlobalUbo);

    VkDescriptorBufferInfo bufferInfo;
    bufferInfo.buffer = this->GlobalUniformBuffer.handle;
    bufferInfo.offset = offset;
    bufferInfo.range = range;

    // Обновить наборы дескрипторов.
    VkWriteDescriptorSet DescriptorWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    DescriptorWrite.dstSet = this->GlobalDescriptorSets[ImageIndex];
    DescriptorWrite.dstBinding = 0;
    DescriptorWrite.dstArrayElement = 0;
    DescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    DescriptorWrite.descriptorCount = 1;
    DescriptorWrite.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(VkAPI->Device.LogicalDevice, 1, &DescriptorWrite, 0, 0);

    // Привяжите набор глобальных дескрипторов для обновления.
    vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline.PipelineLayout, 0, 1, &GlobalDescriptor, 0, 0);
}

void VulkanUI_Shader::SetModel(VulkanAPI *VkAPI, Matrix4D model)
{
    if (VkAPI) {
        u32& ImageIndex = VkAPI->ImageIndex;
        VkCommandBuffer CommandBuffer = VkAPI->GraphicsCommandBuffers[ImageIndex].handle;

        vkCmdPushConstants(CommandBuffer, this->pipeline.PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Matrix4D), &model);
    }
}

void VulkanUI_Shader::ApplyMaterial(VulkanAPI *VkAPI, Material *material)
{
    if (VkAPI) {
        u32& ImageIndex = VkAPI->ImageIndex;
        VkCommandBuffer& CommandBuffer = VkAPI->GraphicsCommandBuffers[ImageIndex].handle;

        // Получить данные о материале.
        VulkanUI_ShaderInstanceState* ObjectState = &this->InstanceStates[material->InternalId];
        VkDescriptorSet& ObjectDescriptorSet = ObjectState->DescriptorSets[ImageIndex];

        // TODO: если требуется обновление
        VkWriteDescriptorSet DescriptorWrites[VULKAN_UI_SHADER_DESCRIPTOR_COUNT]{};
        u32 DescriptorCount = 0;
        u32 DescriptorIndex = 0;

        // Дескриптор 0 — универсальный буфер
        u32 range = sizeof(VulkanUI_ShaderInstanceUniformObject);
        u64 offset = sizeof(VulkanUI_ShaderInstanceUniformObject) * material->InternalId;  // также индекс в массиве.
        VulkanUI_ShaderInstanceUniformObject InstanceUniformObject;

        // Получите диффузный цвет от материала.
        InstanceUniformObject.DiffuseColor = material->DiffuseColour;

        // Загрузите данные в буфер.
        this->ObjectUniformBuffer.LoadData(VkAPI, offset, range, 0, &InstanceUniformObject);

        // Делайте это только в том случае, если дескриптор еще не был обновлен.
        u32& GlobalUniformObjectGeneration = ObjectState->DescriptorStates[DescriptorIndex].generations[ImageIndex];
        if (GlobalUniformObjectGeneration == INVALID::ID || GlobalUniformObjectGeneration != material->generation) {
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
            GlobalUniformObjectGeneration = material->generation;
        }
        DescriptorIndex++;

        // Samplers.
        const u32 SamplerCount = 1;
        VkDescriptorImageInfo ImageInfos[1];
        for (u32 SamplerIndex = 0; SamplerIndex < SamplerCount; ++SamplerIndex) {
            TextureUse use = this->SamplerUses[SamplerIndex];
            Texture* t = nullptr;
            switch (use) {
                case TextureUse::MapDiffuse:
                    t = material->DiffuseMap.texture;
                    break;
                default:
                    MFATAL("Невозможно связать сэмплер с неизвестным использованием.");
                    return;
            }

            u32& DescriptorGeneration = ObjectState->DescriptorStates[DescriptorIndex].generations[ImageIndex];
            u32& DescriptorID = ObjectState->DescriptorStates[DescriptorIndex].ids[ImageIndex];

            // Если текстура еще не загружена, используйте значение по умолчанию.
            if (!t || t->generation == INVALID::ID) {
                t = TextureSystem::Instance()->GetDefaultTexture();

                // Сбросьте генерацию дескриптора, если используете текстуру по умолчанию.
                DescriptorGeneration = INVALID::ID;
            }

            // Сначала проверьте, нуждается ли дескриптор в обновлении.
            if (t && (DescriptorID != t->id || DescriptorGeneration != t->generation || DescriptorGeneration == INVALID::ID)) {
                VulkanTextureData* InternalData = reinterpret_cast<VulkanTextureData*>(t->Data);

                // Назначьте представление и сэмплер.
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
                if (t->generation != INVALID::ID) {
                    DescriptorGeneration = t->generation;
                    DescriptorID = t->id;
                }
                DescriptorIndex++;
            }
        }

        if (DescriptorCount > 0) {
            vkUpdateDescriptorSets(VkAPI->Device.LogicalDevice, DescriptorCount, DescriptorWrites, 0, 0);
        }

        // Привяжите набор дескрипторов для обновления или в случае его изменения.
        vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline.PipelineLayout, 1, 1, &ObjectDescriptorSet, 0, 0);
    }
}

bool VulkanUI_Shader::AcquireResources(VulkanAPI *VkAPI, Material *material)
{
    // TODO: free list
    material->InternalId = this->ObjectUniformBufferIndex;
    this->ObjectUniformBufferIndex++;

    VulkanUI_ShaderInstanceState* ObjectState = &this->InstanceStates[material->InternalId];
    for (u32 i = 0; i < VULKAN_UI_SHADER_DESCRIPTOR_COUNT; ++i) {
        for (u32 j = 0; j < 3; ++j) {
            ObjectState->DescriptorStates[i].generations[j] = INVALID::ID;
            ObjectState->DescriptorStates[i].ids[j] = INVALID::ID;
        }
    }

    // Выделите наборы дескрипторов.
    VkDescriptorSetLayout layouts[3] = {
        this->ObjectDescriptorSetLayout,
        this->ObjectDescriptorSetLayout,
        this->ObjectDescriptorSetLayout};

    VkDescriptorSetAllocateInfo AllocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    AllocInfo.descriptorPool = this->ObjectDescriptorPool;
    AllocInfo.descriptorSetCount = VkAPI->swapchain.ImageCount;  // один на кадр
    AllocInfo.pSetLayouts = layouts;
    VkResult result = vkAllocateDescriptorSets(VkAPI->Device.LogicalDevice, &AllocInfo, ObjectState->DescriptorSets);
    if (result != VK_SUCCESS) {
        MERROR("Ошибка при выделении наборов дескрипторов!");
        return false;
    }

    return true;
}

void VulkanUI_Shader::ReleaseResources(VulkanAPI *VkAPI, Material *material)
{
    VulkanUI_ShaderInstanceState& InstanceState = this->InstanceStates[material->InternalId];

    const u32 DescriptorSetCount = VkAPI->swapchain.ImageCount;

    // Дождитесь завершения любых ожидающих операций, использующих набор дескрипторов.
    vkDeviceWaitIdle(VkAPI->Device.LogicalDevice);

    // Release object descriptor sets.
    VkResult result = vkFreeDescriptorSets(VkAPI->Device.LogicalDevice, this->ObjectDescriptorPool, DescriptorSetCount, InstanceState.DescriptorSets);
    if (result != VK_SUCCESS) {
        MERROR("Ошибка при освобождении наборов дескрипторов объектных шейдеров!");
    }

    for (u32 i = 0; i < VULKAN_UI_SHADER_DESCRIPTOR_COUNT; ++i) {
        for (u32 j = 0; j < 3; ++j) {
            InstanceState.DescriptorStates[i].generations[j] = INVALID::ID;
            InstanceState.DescriptorStates[i].ids[j] = INVALID::ID;
        }
    }

    material->InternalId = INVALID::ID;

    // TODO: add the object_id to the free list
}
