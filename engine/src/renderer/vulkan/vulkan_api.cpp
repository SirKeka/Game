#include "vulkan_api.hpp"
#include "memory/linear_allocator.hpp"
#include "vulkan_command_buffer.hpp"
#include "vulkan_swapchain.hpp"
#include "vulkan_image.hpp"
#include "vulkan_platform.hpp"
#include "vulkan_renderpass.hpp"
#include "systems/material_system.hpp"
#include "systems/resource_system.hpp"
#include "systems/texture_system.hpp"
#include "resources/geometry.hpp"

#include "math/vertex.hpp"

#include "vulkan_utils.hpp"

VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT MessageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
    void* UserData
);

void VulkanAPI::DrawGeometry(const GeometryRenderData& data)
{
    // Игнорировать незагруженные геометрии.
    if (data.gid && data.gid->InternalID == INVALID::ID) {
        return;
    }

    auto BufferData = geometries[data.gid->InternalID];
    auto& CommandBuffer = GraphicsCommandBuffers[ImageIndex];

    // Привязка буфера вершин к смещению.
    VkDeviceSize offsets[1] = {BufferData.VertexBufferOffset};
    vkCmdBindVertexBuffers(CommandBuffer.handle, 0, 1, &ObjectVertexBuffer.handle, static_cast<VkDeviceSize*>(offsets));

    // Рисовать индексированные или неиндексированные.
    if (BufferData.IndexCount > 0) {
        // Привязать индексный буфер по смещению.
        vkCmdBindIndexBuffer(CommandBuffer.handle, ObjectIndexBuffer.handle, BufferData.IndexBufferOffset, VK_INDEX_TYPE_UINT32);

        // Issue the draw.
        vkCmdDrawIndexed(CommandBuffer.handle, BufferData.IndexCount, 1, 0, 0, 0);
    } else {
        vkCmdDraw(CommandBuffer.handle, BufferData.VertexCount, 1, 0, 0);
    }
}

const u32 DESC_SET_INDEX_GLOBAL   = 0;  // Индекс набора глобальных дескрипторов.
const u32 DESC_SET_INDEX_INSTANCE = 1;  // Индекс набора дескрипторов экземпляра.

bool VulkanAPI::Load(Shader *shader, const ShaderConfig& config, Renderpass* renderpass, u8 StageCount, const DArray<MString>& StageFilenames, const ShaderStage *stages)
{
    // Этапы перевода
    VkShaderStageFlags VkStages[VulkanShaderConstants::MaxStages];
    for (u8 i = 0; i < StageCount; ++i) {
        switch (stages[i]) {
            case ShaderStage::Fragment:
                VkStages[i] = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            case ShaderStage::Vertex:
                VkStages[i] = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case ShaderStage::Geometry:
                MWARN("VulkanAPI::LoadShader: VK_SHADER_STAGE_GEOMETRY_BIT установлен, но еще не поддерживается.");
                VkStages[i] = VK_SHADER_STAGE_GEOMETRY_BIT;
                break;
            case ShaderStage::Compute:
                MWARN("VulkanAPI::LoadShader: SHADER_STAGE_COMPUTE установлен, но еще не поддерживается.");
                VkStages[i] = VK_SHADER_STAGE_COMPUTE_BIT;
                break;
            default:
                MERROR("Неподдерживаемый тип этапа: %d", stages[i]);
                break;
        }
    }

    // ЗАДАЧА: настраиваемый максимальный счетчик выделения дескриптора.

    u32 MaxDescriptorAllocateCount = 1024;

    // Скопируйте указатель на контекст.
    shader->ShaderData = new VulkanShader();
    auto OutShader = shader->ShaderData;

    OutShader->renderpass = reinterpret_cast<VulkanRenderpass*>(renderpass->InternalData);

    // Создайте конфигурацию.
    OutShader->config.MaxDescriptorSetCount = MaxDescriptorAllocateCount;

    // Этапы шейдера. Разбираем флаги.
    // MMemory::ZeroMem(OutShader->config.stages, sizeof(VulkanShaderStageConfig) * VulkanShaderConstants::MaxStages);
    OutShader->config.StageCount = 0;
    // Перебрать предоставленные этапы.
    for (u32 i = 0; i < StageCount; i++) {
        // Убедитесь, что достаточно места для добавления сцены.
        if (OutShader->config.StageCount + 1 > VulkanShaderConstants::MaxStages) {
            MERROR("Шейдеры могут иметь максимум %d стадий.", VulkanShaderConstants::MaxStages);
            return false;
        }

        // Убедитесь, что сцена поддерживается.
        VkShaderStageFlagBits StageFlag;
        switch (stages[i]) {
            case ShaderStage::Vertex:
                StageFlag = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case ShaderStage::Fragment:
                StageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            default:
                // Перейти к следующему типу.
                MERROR("VulkanAPI::LoadShader: Отмечена неподдерживаемая стадия шейдера: %d. Стадия проигнорирована.", stages[i]);
                continue;
        }

        // Подготовьте сцену и ударьте по счетчику.
        OutShader->config.stages[OutShader->config.StageCount].stage = StageFlag;
        MString::nCopy(OutShader->config.stages[OutShader->config.StageCount].FileName, StageFilenames[i], 255);
        OutShader->config.StageCount++;
    }

    OutShader->config.DescriptorSets[0].SamplerBindingIndex = INVALID::U8ID;
    OutShader->config.DescriptorSets[1].SamplerBindingIndex = INVALID::U8ID;

    // Получите данные по количеству униформы.
    OutShader->GlobalUniformCount = 0;
    OutShader->GlobalUniformSamplerCount = 0;
    OutShader->InstanceUniformCount = 0;
    OutShader->InstanceUniformSamplerCount = 0;
    OutShader->LocalUniformCount = 0;
    const u32& TotalCount = config.uniforms.Length();
    for (u32 i = 0; i < TotalCount; ++i) {
        switch (config.uniforms[i].scope) {
            case ShaderScope::Global:
                if (config.uniforms[i].type == ShaderUniformType::Sampler) {
                    OutShader->GlobalUniformSamplerCount++;
                } else {
                    OutShader->GlobalUniformCount++;
                }
                break;
            case ShaderScope::Instance:
                if (config.uniforms[i].type == ShaderUniformType::Sampler){
                    OutShader->InstanceUniformSamplerCount++;
                } else {
                    OutShader->InstanceUniformCount++;
                }
                break;
            case ShaderScope::Local:
                OutShader->LocalUniformCount++;
                break;
        }
    }

    // На данный момент шейдеры будут иметь только эти два типа пулов дескрипторов.
    OutShader->config.PoolSizes[0] = VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024};          // HACK: максимальное количество наборов дескрипторов ubo.
    OutShader->config.PoolSizes[1] = VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4096};  // HACK: максимальное количество наборов дескрипторов сэмплера изображений.

    // Конфигурация глобального набора дескрипторов.
    if (OutShader->GlobalUniformCount > 0 || OutShader->GlobalUniformSamplerCount > 0) {
    
        auto& SetConfig = OutShader->config.DescriptorSets[OutShader->config.DescriptorSetCount];

        // При наличии глобального UBO-привязки он является первым.
        if (OutShader->GlobalUniformCount > 0) {
            const u8& BindingIndex = SetConfig.BindingCount;
            SetConfig.bindings[BindingIndex].binding = BindingIndex;
            SetConfig.bindings[BindingIndex].descriptorCount = 1;
            SetConfig.bindings[BindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            SetConfig.bindings[BindingIndex].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            SetConfig.BindingCount++;
        }

        // Добавьте привязку для сэмплеров, если они используются.
        if (OutShader->GlobalUniformSamplerCount > 0) {
            const u8& BindingIndex = SetConfig.BindingCount;
            SetConfig.bindings[BindingIndex].binding = BindingIndex;
            SetConfig.bindings[BindingIndex].descriptorCount = OutShader->GlobalUniformSamplerCount;  // Один дескриптор на сэмплер.
            SetConfig.bindings[BindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            SetConfig.bindings[BindingIndex].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            SetConfig.SamplerBindingIndex = BindingIndex;
            SetConfig.BindingCount++;
        }

        // Увеличить установленный счетчик.
        OutShader->config.DescriptorSetCount++;
    }

    // Если используются униформы экземпляров, добавьте набор дескрипторов UBO.
    if (OutShader->InstanceUniformCount > 0 || OutShader->InstanceUniformSamplerCount > 0) {
        // В этом наборе добавьте привязку для UBO, если она используется.
        auto& SetConfig = OutShader->config.DescriptorSets[OutShader->config.DescriptorSetCount];

        if (OutShader->InstanceUniformCount > 0) {
            const u8& BindingIndex = SetConfig.BindingCount;
            SetConfig.bindings[BindingIndex].binding = BindingIndex;
            SetConfig.bindings[BindingIndex].descriptorCount = 1;
            SetConfig.bindings[BindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            SetConfig.bindings[BindingIndex].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            SetConfig.BindingCount++;
        }

        // Добавьте привязку для сэмплеров, если она используется.
        if (OutShader->InstanceUniformSamplerCount > 0) {
            const u8& BindingIndex = SetConfig.BindingCount;
            SetConfig.bindings[BindingIndex].binding = BindingIndex;
            SetConfig.bindings[BindingIndex].descriptorCount = OutShader->InstanceUniformSamplerCount;  // Один дескриптор на сэмплер.
            SetConfig.bindings[BindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            SetConfig.bindings[BindingIndex].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            SetConfig.SamplerBindingIndex = BindingIndex;
            SetConfig.BindingCount++;
        }

        // Увеличьте счетчик набора.
        OutShader->config.DescriptorSetCount++;
    }

   // Сохраните копию режима отбраковки.
    OutShader->config.CullMode = config.CullMode;

    return true;
}

void VulkanAPI::Unload(Shader *shader)
{
    if (shader && shader->ShaderData) {
        VulkanShader* VkShader = reinterpret_cast<VulkanShader*>(shader->ShaderData);
        if (!VkShader) {
            MERROR("VulkanAPI::UnloadShader требуется действительный указатель на шейдер.");
            return;
        }

        VkDevice& LogicalDevice = Device.LogicalDevice;
        //VkAllocationCallbacks* VkAllocator = allocator;

        // Макеты набора дескрипторов.
        for (u32 i = 0; i < VkShader->config.DescriptorSetCount; ++i) {
            if (VkShader->DescriptorSetLayouts[i]) {
                vkDestroyDescriptorSetLayout(LogicalDevice, VkShader->DescriptorSetLayouts[i], allocator);
                VkShader->DescriptorSetLayouts[i] = 0;
            }
        }

        // Пул дескрипторов
        if (VkShader->DescriptorPool) {
            vkDestroyDescriptorPool(LogicalDevice, VkShader->DescriptorPool, allocator);
        }

        // Однородный буфер.
        VkShader->UniformBuffer.UnlockMemory(this);
        VkShader->MappedUniformBufferBlock = 0;
        VkShader->UniformBuffer.Destroy(this);

        // Конвеер
        VkShader->pipeline.Destroy(this);

        // Шейдерные модули
        for (u32 i = 0; i < VkShader->config.StageCount; ++i) {
            vkDestroyShaderModule(Device.LogicalDevice, VkShader->stages[i].handle, allocator);
        }

        // Уничтожьте конфигурацию.
        MMemory::ZeroMem(&VkShader->config, sizeof(VulkanShaderConfig));

        // Освободите внутреннюю память данных.
        MMemory::Free(shader->ShaderData, sizeof(VulkanShader), Memory::Renderer);
        shader->ShaderData = 0;
    }
}

bool VulkanAPI::ShaderInitialize(Shader *shader)
{
    auto& LogicalDevice = Device.LogicalDevice;
    VulkanShader* VkShader = shader->ShaderData;

    // Создайте модуль для каждого этапа.
    //MMemory::ZeroMem(VkShader->stages, sizeof(VulkanShaderStage) * VulkanShaderConstants::MaxStages);
    for (u32 i = 0; i < VkShader->config.StageCount; ++i) {
        if (!CreateModule(VkShader, VkShader->config.stages[i], &VkShader->stages[i])) {
            MERROR("Невозможно создать шейдерный модуль %s для «%s». Шейдер будет уничтожен", VkShader->config.stages[i].FileName, shader->name.c_str());
            return false;
        }
    }

    // Статическая таблица поиска для наших типов -> Vulkan.
    static VkFormat* types = nullptr;
    static VkFormat t[11];
    if (!types) {
        t[EShaderAttribute::Float32]   = VK_FORMAT_R32_SFLOAT;
        t[EShaderAttribute::Float32_2] = VK_FORMAT_R32G32_SFLOAT;
        t[EShaderAttribute::Float32_3] = VK_FORMAT_R32G32B32_SFLOAT;
        t[EShaderAttribute::Float32_4] = VK_FORMAT_R32G32B32A32_SFLOAT;
        t[EShaderAttribute::Int8]      = VK_FORMAT_R8_SINT;
        t[EShaderAttribute::UInt8]     = VK_FORMAT_R8_UINT;
        t[EShaderAttribute::Int16]     = VK_FORMAT_R16_SINT;
        t[EShaderAttribute::UInt16]    = VK_FORMAT_R16_UINT;
        t[EShaderAttribute::Int32]     = VK_FORMAT_R32_SINT;
        t[EShaderAttribute::UInt32]    = VK_FORMAT_R32_UINT;
        types = t;
    }

    // Атрибуты процесса
    const u32& AttributeCount = shader->attributes.Length();
    u32 offset = 0;
    for (u32 i = 0; i < AttributeCount; ++i) {
        // Настройте новый атрибут.
        VkVertexInputAttributeDescription attribute;
        attribute.location = i;
        attribute.binding = 0;
        attribute.offset = offset;
        attribute.format = types[static_cast<int>(shader->attributes[i].type)];

        // Вставьте коллекцию атрибутов конфигурации и добавьте к шагу.
        VkShader->config.attributes[i] = attribute;

        offset += shader->attributes[i].size;
    }

    // Пул дескрипторов.
    VkDescriptorPoolCreateInfo PoolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    PoolInfo.poolSizeCount = 2;
    PoolInfo.pPoolSizes = VkShader->config.PoolSizes;
    PoolInfo.maxSets = VkShader->config.MaxDescriptorSetCount;
    PoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    // Создайте пул дескрипторов.
    VkResult result = vkCreateDescriptorPool(LogicalDevice, &PoolInfo, allocator, &VkShader->DescriptorPool);
    if (!VulkanResultIsSuccess(result)) {
        MERROR("VulkanAPI::ShaderInitialize — не удалось создать пул дескрипторов: '%s'", VulkanResultString(result, true));
        return false;
    }

    // Создайте макеты наборов дескрипторов.
    for (u32 i = 0; i < VkShader->config.DescriptorSetCount; ++i) {
        VkDescriptorSetLayoutCreateInfo LayoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        LayoutInfo.bindingCount = VkShader->config.DescriptorSets[i].BindingCount;
        LayoutInfo.pBindings = VkShader->config.DescriptorSets[i].bindings;
        result = vkCreateDescriptorSetLayout(LogicalDevice, &LayoutInfo, allocator, &VkShader->DescriptorSetLayouts[i]);
        if (!VulkanResultIsSuccess(result)) {
            MERROR("VulkanAPI::ShaderInitialize — не удалось создать пул дескрипторов: '%s'", VulkanResultString(result, true));
            return false;
        }
    }

    // ЗАДАЧА: Кажется неправильным иметь их здесь, по крайней мере, в таком виде. 
    // Вероятно, следует настроить получение из какого-либо места вместо области просмотра.
    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = (f32)FramebufferHeight;
    viewport.width = (f32)FramebufferWidth;
    viewport.height = -(f32)FramebufferHeight;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Scissor
    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = FramebufferWidth;
    scissor.extent.height = FramebufferHeight;

    VkPipelineShaderStageCreateInfo StageCreateIfos[VulkanShaderConstants::MaxStages]{};
    for (u32 i = 0; i < VkShader->config.StageCount; ++i) {
        StageCreateIfos[i] = VkShader->stages[i].ShaderStageCreateInfo;
    }

    bool PipelineResult = VkShader->pipeline.Create(
        this,
        VkShader->renderpass,
        shader->AttributeStride,
        shader->attributes.Length(),
        VkShader->config.attributes,  // shader->attributes,
        VkShader->config.DescriptorSetCount,
        VkShader->DescriptorSetLayouts,
        VkShader->config.StageCount,
        StageCreateIfos,
        viewport,
        scissor,
        VkShader->config.CullMode,
        false,
        true,
        shader->PushConstantRangeCount,
        shader->PushConstantRanges);

    if (!PipelineResult) {
        MERROR("Не удалось загрузить графический конвейер для объектного шейдера.");
        return false;
    }

    // Получите требования к выравниванию UBO с устройства.
    shader->RequiredUboAlignment = Device.properties.limits.minUniformBufferOffsetAlignment;

    // Убедитесь, что UBO выровнен в соответствии с требованиями устройства.
    shader->GlobalUboStride = Range::GetAligned(shader->GlobalUboSize, shader->RequiredUboAlignment);
    shader->UboStride = Range::GetAligned(shader->UboSize, shader->RequiredUboAlignment);

    // Однородный буфер.
    // u32 DeviceLocalBits = Device.SupportsDeviceLocalHostVisible ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0;
    // ЗАДАЧА: Максимальное количество должно быть настраиваемым или, возможно, иметь долгосрочную поддержку изменения размера буфера.
    u64 TotalBufferSize = shader->GlobalUboStride + (shader->UboStride * VULKAN_MAX_MATERIAL_COUNT);  // global + (locals)
    if (!VkShader->UniformBuffer.Create(
            this,
            TotalBufferSize,
            static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT),
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // | DeviceLocalBits,
            true,
            true)) {
        MERROR("VulkanAPI::ShaderInitialize — не удалось создать буфер Vulkan для шейдера объекта.");
        return false;
    }

    // Выделите место для глобального UBO, которое должно занимать пространство _шага_, а не фактически используемый размер.
    if (!VkShader->UniformBuffer.Allocate(shader->GlobalUboStride, shader->GlobalUboOffset)) {
        MERROR("Не удалось выделить место для универсального буфера!");
        return false;
    }

    // Отобразите всю память буфера.
    VkShader->MappedUniformBufferBlock = VkShader->UniformBuffer.LockMemory(this, 0, VK_WHOLE_SIZE /*total_buffer_size*/, 0);

    // Выделите наборы глобальных дескрипторов, по одному на кадр. Global всегда является первым набором.
    VkDescriptorSetLayout GlobalLayouts[3] = {
        VkShader->DescriptorSetLayouts[DESC_SET_INDEX_GLOBAL],
        VkShader->DescriptorSetLayouts[DESC_SET_INDEX_GLOBAL],
        VkShader->DescriptorSetLayouts[DESC_SET_INDEX_GLOBAL]};

    VkDescriptorSetAllocateInfo AllocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    AllocInfo.descriptorPool = VkShader->DescriptorPool;
    AllocInfo.descriptorSetCount = 3;
    AllocInfo.pSetLayouts = GlobalLayouts;
    VK_CHECK(vkAllocateDescriptorSets(Device.LogicalDevice, &AllocInfo, VkShader->GlobalDescriptorSets));

    return true;
}

bool VulkanAPI::ShaderUse(Shader *shader)
{
    shader->ShaderData->pipeline.Bind(GraphicsCommandBuffers[ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS);
    return true;
}

bool VulkanAPI::ShaderApplyGlobals(Shader *shader)
{
    auto VkShader = shader->ShaderData;
    auto& CommandBuffer = GraphicsCommandBuffers[ImageIndex].handle;
    auto& GlobalDescriptor = VkShader->GlobalDescriptorSets[ImageIndex];

    // Сначала примените UBO
    VkDescriptorBufferInfo BufferInfo;
    BufferInfo.buffer = VkShader->UniformBuffer.handle;
    BufferInfo.offset = shader->GlobalUboOffset;
    BufferInfo.range = shader->GlobalUboStride;

    // Обновить наборы дескрипторов.
    VkWriteDescriptorSet UboWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    UboWrite.dstSet = VkShader->GlobalDescriptorSets[ImageIndex];
    UboWrite.dstBinding = 0;
    UboWrite.dstArrayElement = 0;
    UboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    UboWrite.descriptorCount = 1;
    UboWrite.pBufferInfo = &BufferInfo;

    VkWriteDescriptorSet DescriptorWrites[2];
    DescriptorWrites[0] = UboWrite;

    u8& GlobalSetBindingCount = VkShader->config.DescriptorSets[DESC_SET_INDEX_GLOBAL].BindingCount;
    if (GlobalSetBindingCount > 1) {
        // ЗАДАЧА: Есть семплеры, которые нужно написать. Поддержите это.
        GlobalSetBindingCount = 1;
        MERROR("Глобальные образцы изображений пока не поддерживаются.");

        // VkWriteDescriptorSet sampler_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        // descriptor_writes[1] = ...
    }

    vkUpdateDescriptorSets(Device.LogicalDevice, GlobalSetBindingCount, DescriptorWrites, 0, 0);

    // Привяжите набор глобальных дескрипторов для обновления.
    vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, VkShader->pipeline.PipelineLayout, 0, 1, &GlobalDescriptor, 0, 0);
    return true;
}

bool VulkanAPI::ShaderApplyInstance(Shader *shader, bool NeedsUpdate)
{
    VulkanShader* VkShader = shader->ShaderData;
    if (VkShader->InstanceUniformCount < 1 && VkShader->InstanceUniformSamplerCount < 1) {
        MERROR("Этот шейдер не использует экземпляры.");
        return false;
    }
    
    const VkCommandBuffer& CommandBuffer = GraphicsCommandBuffers[ImageIndex].handle;

    // Получите данные экземпляра.
    VulkanShaderInstanceState& ObjectState = VkShader->InstanceStates[shader->BoundInstanceID];
    const VkDescriptorSet& ObjectDescriptorSet = ObjectState.DescriptorSetState.DescriptorSets[ImageIndex];

    if(NeedsUpdate) {
        VkWriteDescriptorSet DescriptorWrites[2] {};  // Всегда максимум два набора дескрипторов.
        u32 DescriptorCount = 0;
        u32 DescriptorIndex = 0;
    
        // Дескриптор 0 — универсальный буфер
        if (VkShader->InstanceUniformCount > 0) {
            // Делайте это только в том случае, если дескриптор еще не был обновлен.
            u8& InstanceUboGeneration = ObjectState.DescriptorSetState.DescriptorStates[DescriptorIndex].generations[ImageIndex];
            // ЗАДАЧА: определить, требуется ли обновление.
            if (InstanceUboGeneration == INVALID::U8ID /*|| *global_ubo_generation != material->generation*/) {
                VkDescriptorBufferInfo BufferInfo;
                BufferInfo.buffer = VkShader->UniformBuffer.handle;
                BufferInfo.offset = ObjectState.offset;
                BufferInfo.range = shader->UboStride;
        
                VkWriteDescriptorSet UboDescriptor = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
                UboDescriptor.dstSet = ObjectDescriptorSet;
                UboDescriptor.dstBinding = DescriptorIndex;
                UboDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                UboDescriptor.descriptorCount = 1;
                UboDescriptor.pBufferInfo = &BufferInfo;
        
                DescriptorWrites[DescriptorCount] = UboDescriptor;
                DescriptorCount++;
        
                // Обновите генерацию кадра. В данном случае он нужен только один раз, поскольку это буфер.
                InstanceUboGeneration = 1;  // material->generation; ЗАДАЧА: какое-то поколение откуда-то...
            }
            DescriptorIndex++;
        }

        // Итерация сэмплеров.
        if (VkShader->InstanceUniformSamplerCount > 0) {
            const u8& SamplerBindingIndex = VkShader->config.DescriptorSets[DESC_SET_INDEX_INSTANCE].SamplerBindingIndex;
            const u32& TotalSamplerCount = VkShader->config.DescriptorSets[DESC_SET_INDEX_INSTANCE].bindings[SamplerBindingIndex].descriptorCount;
            u32 UpdateSamplerCount = 0;
            VkDescriptorImageInfo ImageInfos[VulkanShaderConstants::MaxGlobalTextures]{};
            for (u32 i = 0; i < TotalSamplerCount; ++i) {
                // ЗАДАЧА: обновляйте список только в том случае, если оно действительно необходимо.
                TextureMap* map = VkShader->InstanceStates[shader->BoundInstanceID].InstanceTexturesMaps[i];
                Texture* t = map->texture;
                ImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                ImageInfos[i].imageView = t->Data->view;
                ImageInfos[i].sampler = reinterpret_cast<VkSampler>(map->sampler);
    
                // ЗАДАЧА: измените состояние дескриптора, чтобы справиться с этим должным образом.
                // Синхронизировать генерацию кадров, если не используется текстура по умолчанию.
                // if (t->generation != INVALID_ID) {
                //     *descriptor_generation = t->generation;
                //     *descriptor_id = t->id;
                // }
    
                UpdateSamplerCount++;
            }
    
            VkWriteDescriptorSet SamplerDescriptor = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            SamplerDescriptor.dstSet = ObjectDescriptorSet;
            SamplerDescriptor.dstBinding = DescriptorIndex;
            SamplerDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            SamplerDescriptor.descriptorCount = UpdateSamplerCount;
            SamplerDescriptor.pImageInfo = ImageInfos;
    
            DescriptorWrites[DescriptorCount] = SamplerDescriptor;
            DescriptorCount++;
        }
    
        if (DescriptorCount > 0) {
            vkUpdateDescriptorSets(Device.LogicalDevice, DescriptorCount, DescriptorWrites, 0, nullptr);
        }
    }

    // Привяжите набор дескрипторов для обновления или на случай изменения шейдера.
    vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, VkShader->pipeline.PipelineLayout, 1, 1, &ObjectDescriptorSet, 0, 0/*nullptr*/);
    return true;
}

VkSamplerAddressMode ConvertRepeatType (const char* axis, TextureRepeat repeat) {
    switch (repeat) {
        case TextureRepeat::Repeat:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case TextureRepeat::MirroredRepeat:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case TextureRepeat::ClampToEdge:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case TextureRepeat::ClampToBorder:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        default:
            MWARN("ConvertRepeatType(ось = «%s») Тип «%x» не поддерживается, по умолчанию используется повтор.", axis, repeat);
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
}

VkFilter ConvertFilterType(const char* op, TextureFilter filter) {
    switch (filter) {
        case TextureFilter::ModeNearest:
            return VK_FILTER_NEAREST;
        case TextureFilter::ModeLinear:
            return VK_FILTER_LINEAR;
        default:
            MWARN("ConvertFilterType(op='%s'): Неподдерживаемый тип фильтра «%x», по умолчанию — линейный.", op, filter);
            return VK_FILTER_LINEAR;
    }
}

bool VulkanAPI::ShaderAcquireInstanceResources(Shader *shader, TextureMap** maps, u32 &OutInstanceID)
{
    auto VkShader = shader->ShaderData;
    // ЗАДАЧА: динамическим
    OutInstanceID = INVALID::ID;
    for (u32 i = 0; i < 1024; ++i) {
        if (VkShader->InstanceStates[i].id == INVALID::ID) {
            VkShader->InstanceStates[i].id = i;
            OutInstanceID = i;
            break;
        }
    }
    if (OutInstanceID == INVALID::ID) {
        MERROR("VulkanShader::AcquireInstanceResources — не удалось получить новый идентификатор");
        return false;
    }

    auto& InstanceState = VkShader->InstanceStates[OutInstanceID];
    const u8& SamplerBindingIndex = VkShader->config.DescriptorSets[DESC_SET_INDEX_INSTANCE].SamplerBindingIndex;
    const u32& InstanceTextureCount = VkShader->config.DescriptorSets[DESC_SET_INDEX_INSTANCE].bindings[SamplerBindingIndex].descriptorCount;
    // Очистите память всего массива, даже если она не вся использована.
    InstanceState.InstanceTexturesMaps = MMemory::TAllocate<TextureMap*>(Memory::Array, shader->InstanceTextureCount);
    Texture* DefaultTexture = TextureSystem::Instance()->GetDefaultTexture(ETexture::Default);
    MMemory::CopyMem(InstanceState.InstanceTexturesMaps, maps, sizeof(TextureMap*) * shader->InstanceTextureCount);
    // Установите для всех указателей текстур значения по умолчанию, пока они не будут назначены.
    for (u32 i = 0; i < InstanceTextureCount; ++i) {
        if (!maps[i]->texture) {
            InstanceState.InstanceTexturesMaps[i]->texture = DefaultTexture;
        }
    }

    // Выделите немного места в УБО — по шагу, а не по размеру.
    const u64& size = shader->UboStride;
    if (size > 0) {
        if (!VkShader->UniformBuffer.Allocate(size, InstanceState.offset)) {
            MERROR("VulkanAPI::ShaderAcquireInstanceResources — не удалось получить пространство UBO");
            return false;
        }
    }

    auto& SetState = InstanceState.DescriptorSetState;

    // Привязка каждого дескриптора в наборе
    const u32& BindingCount = VkShader->config.DescriptorSets[DESC_SET_INDEX_INSTANCE].BindingCount;
    // MMemory::ZeroMem(SetState.DescriptorStates, sizeof(VulkanDescriptorState) * VulkanShaderConstants::MaxBindings);
    for (u32 i = 0; i < BindingCount; ++i) {
        for (u32 j = 0; j < 3; ++j) {
            SetState.DescriptorStates[i].generations[j] = INVALID::U8ID;
            SetState.DescriptorStates[i].ids[j] = INVALID::ID;
        }
    }

    // Выделите 3 набора дескрипторов (по одному на кадр).
    VkDescriptorSetLayout layouts[3] = {
        VkShader->DescriptorSetLayouts[DESC_SET_INDEX_INSTANCE],
        VkShader->DescriptorSetLayouts[DESC_SET_INDEX_INSTANCE],
        VkShader->DescriptorSetLayouts[DESC_SET_INDEX_INSTANCE]};

    VkDescriptorSetAllocateInfo AllocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    AllocInfo.descriptorPool = VkShader->DescriptorPool;
    AllocInfo.descriptorSetCount = 3;
    AllocInfo.pSetLayouts = layouts;
    VkResult result = vkAllocateDescriptorSets(
        Device.LogicalDevice,
        &AllocInfo,
        InstanceState.DescriptorSetState.DescriptorSets);
    if (result != VK_SUCCESS) {
        MERROR("Ошибка при выделении наборов дескрипторов экземпляра в шейдере: '%s'.", VulkanResultString(result, true));
        return false;
    }

    return true;
}

bool VulkanAPI::ShaderReleaseInstanceResources(Shader *shader, u32 InstanceID)
{
    VulkanShader* VkShader = shader->ShaderData;
    VulkanShaderInstanceState& InstanceState = VkShader->InstanceStates[InstanceID];

    // Дождитесь завершения любых ожидающих операций, использующих набор дескрипторов.
    vkDeviceWaitIdle(Device.LogicalDevice);

    // 3 свободных набора дескрипторов (по одному на кадр)
    VkResult result = vkFreeDescriptorSets(
        Device.LogicalDevice,
        VkShader->DescriptorPool,
        3,
        InstanceState.DescriptorSetState.DescriptorSets);
    if (result != VK_SUCCESS) {
        MERROR("Ошибка при освобождении наборов дескрипторов объекта шейдера!");
    }

    // Уничтожить состояния дескриптора.
    MMemory::ZeroMem(InstanceState.DescriptorSetState.DescriptorStates, sizeof(VulkanDescriptorState) * VulkanShaderConstants::MaxBindings);

    if (InstanceState.InstanceTexturesMaps) {
        MMemory::Free(InstanceState.InstanceTexturesMaps, sizeof(TextureMap*) * shader->InstanceTextureCount, Memory::Array);
        InstanceState.InstanceTexturesMaps = nullptr;
    }

    VkShader->UniformBuffer.Free(shader->UboStride, InstanceState.offset);
    InstanceState.offset = INVALID::ID;
    InstanceState.id = INVALID::ID;

    return true;
}

VulkanAPI::VulkanAPI(MWindow *window,  const RendererConfig& config, u8& OutWindowRenderTargetCount)
: FrameDeltaTime(),
// Просто установите некоторые значения по умолчанию для буфера кадра на данный момент.
// На самом деле неважно, что это, потому что они будут переопределены, но они необходимы для создания цепочки обмена.
FramebufferWidth(800), FramebufferHeight(600), 
FramebufferSizeGeneration(),
FramebufferSizeLastGeneration(),
instance(), allocator(nullptr), surface(),
Device(), swapchain(),
RenderpassTableBlock(MMemory::Allocate(sizeof(u32) * VULKAN_MAX_REGISTERED_RENDERPASSES, Memory::Renderer)),
RenderpassTable(VULKAN_MAX_REGISTERED_RENDERPASSES, false, reinterpret_cast<u32*>(RenderpassTableBlock), true, INVALID::ID), 
RegisteredPasses(),
OnRendertargetRefreshRequired(config.OnRendertargetRefreshRequired)
{
    // ЗАДАЧА: пользовательский allocator.
    allocator = NULL;

    // Общая структура информации о приложении.
    VkApplicationInfo AppInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    AppInfo.apiVersion = VK_API_VERSION_1_2;
    AppInfo.pApplicationName = config.ApplicationName;
    AppInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    AppInfo.pEngineName = "Moon Engine";
    AppInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0); 
    // Создание экземпляра.
    VkInstanceCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    CreateInfo.pApplicationInfo = &AppInfo;

    // Получите список необходимых расширений
    
    DArray<const char*>RequiredExtensions;                              // Создаем массив указателей
    RequiredExtensions.PushBack(VK_KHR_SURFACE_EXTENSION_NAME);         // Общее расширение поверхности
    PlatformGetRequiredExtensionNames(RequiredExtensions);              // Расширения для конкретной платформы
#if defined(_DEBUG)
    RequiredExtensions.PushBack(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);     // утилиты отладки

    MDEBUG("Необходимые расширения:");
    u32 length = RequiredExtensions.Length();
    for (u32 i = 0; i < length; ++i) {
        MDEBUG(RequiredExtensions[i]);
    }
#endif

    CreateInfo.enabledExtensionCount = RequiredExtensions.Length();
    CreateInfo.ppEnabledExtensionNames = RequiredExtensions.Data(); //ЗАДАЧА: указателю ppEnabledExtensionNames присваевается адрес указателя массива после выхода из функции данные стираются

    // Уровни проверки.
    DArray<const char*> RequiredValidationLayerNames; // указатель на массив символов ЗАДАЧА: придумать как использовать строки или другой способ отображать занятую память
    u32 RequiredValidationLayerCount = 0;

// Если необходимо выполнить проверку, получите список имен необходимых слоев проверки 
// и убедитесь, что они существуют. Слои проверки следует включать только в нерелизных сборках.
#if defined(_DEBUG)
    MINFO("Уровни проверки включены. Перечисление...");

    // Список требуемых уровней проверки.
    RequiredValidationLayerNames.PushBack("VK_LAYER_KHRONOS_validation");
    RequiredValidationLayerCount = RequiredValidationLayerNames.Length();

    // Получите список доступных уровней проверки.
    u32 AvailableLayerCount = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&AvailableLayerCount, 0));
    VkLayerProperties prop;
    DArray<VkLayerProperties> AvailableLayers(AvailableLayerCount, prop);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&AvailableLayerCount, AvailableLayers.Data())); 

    // Убедитесь, что доступны все необходимые слои.
    for (u32 i = 0; i < RequiredValidationLayerCount; ++i) {
        MINFO("Поиск слоя: %shader...", RequiredValidationLayerNames[i]);
        bool found = false;
        for (u32 j = 0; j < AvailableLayerCount; ++j) {
            if (MString::Equal(RequiredValidationLayerNames[i], AvailableLayers[j].layerName)) {
                found = true;
                MINFO("Найдено.");
                break;
            }
        }

        if (!found) {
            MFATAL("Необходимый уровень проверки отсутствует: %shader", RequiredValidationLayerNames[i]);
            return;
        }
    }
    MINFO("Присутствуют все необходимые уровни проверки.");
#endif

    CreateInfo.enabledLayerCount = RequiredValidationLayerCount;
    CreateInfo.ppEnabledLayerNames = RequiredValidationLayerNames.Data();

    VK_CHECK(vkCreateInstance(&CreateInfo, allocator, &instance));
    MINFO("Создан экземпляр Vulkan.");

     // Debugger
#if defined(_DEBUG)
    MDEBUG("Создание отладчика Vulkan...");
    u32 LogSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;  //|
                                                                      //    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT DebugCreateInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    DebugCreateInfo.messageSeverity = LogSeverity;
    DebugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    DebugCreateInfo.pfnUserCallback = VkDebugCallback;

    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    MASSERT_MSG(func, "Не удалось создать debug messenger!");
    VK_CHECK(func(instance, &DebugCreateInfo, allocator, &DebugMessenger));
    MDEBUG("Создан отладчик Vulkan.");
#endif

    // Поверхность
    MDEBUG("Создание Vulkan поверхности...");
    if (!PlatformCreateVulkanSurface(window, this)) {
        MERROR("Не удалось создать поверхность платформы!");
        return;
    }
    MDEBUG("Поверхность Vulkan создана.");

    // Создание устройства
    if (!Device.Create(this)) {
        MERROR("Не удалось создать устройство!");
        return;
    }

    // Swapchain
    swapchain.Create(this, FramebufferWidth, FramebufferHeight);

    // Сохраните количество имеющихся у нас изображений в качестве необходимого количества целей рендеринга.
    OutWindowRenderTargetCount = swapchain.ImageCount;

   // Проходы рендеринга
    for (u32 i = 0; i < config.RenderpassCount; ++i) {
        // ЗАДАЧА: перейти к функции для возможности повторного использования.
        // Сначала убедитесь, что нет конфликтов с именем.
        u32 id = INVALID::ID;
        RenderpassTable.Get(config.PassConfigs[i].name, &id);
        if (id != INVALID::ID) {
            MERROR("Столкновение с renderpass с именем '%s'. Инициализация не удалась.", config.PassConfigs[i].name);
            return;
        }
        // Вырежьте новый идентификатор.
        for (u32 j = 0; j < VULKAN_MAX_REGISTERED_RENDERPASSES; ++j) {
            if (RegisteredPasses[j].id == INVALID::U16ID) {
                // Нашли его.
                RegisteredPasses[j].id = j;
                id = j;
                break;
            }
        }

        // Убедитесь, что мы получили идентификатор
        if (id == INVALID::ID) {
            MERROR("Не найдено места для нового рендерпасса. Увеличьте VULKAN_MAX_REGISTERED_RENDERPASSES. Инициализация не удалась.");
            return;
        }

        // Настройте renderpass.
        RegisteredPasses[id].ClearFlags = config.PassConfigs[i].ClearFlags;
        RegisteredPasses[id].ClearColour = config.PassConfigs[i].ClearColour;
        RegisteredPasses[id].RenderArea = config.PassConfigs[i].RenderArea;

        RenderpassCreate(&RegisteredPasses[id], 1.0f, 0, config.PassConfigs[i].PrevName != 0, config.PassConfigs[i].NextName != 0);

        // Обновите таблицу с новым идентификатором.
        RenderpassTable.Set(config.PassConfigs[i].name, id);
    }

    // Создайте буферы команд.
    CreateCommandBuffers();

    // Создайте объекты синхронизации.
    ImageAvailableSemaphores.Resize(swapchain.MaxFramesInFlight);
    QueueCompleteSemaphores.Resize(swapchain.MaxFramesInFlight);

    for (u8 i = 0; i < swapchain.MaxFramesInFlight; ++i) {
        VkSemaphoreCreateInfo SemaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(Device.LogicalDevice, &SemaphoreCreateInfo, allocator, &ImageAvailableSemaphores[i]);
        vkCreateSemaphore(Device.LogicalDevice, &SemaphoreCreateInfo, allocator, &QueueCompleteSemaphores[i]);

        // Создайте ограждение в сигнальном состоянии, указывая, что первый кадр уже «отрисован».
        // Это не позволит приложению бесконечно ждать рендеринга первого кадра, 
        // поскольку он не может быть отрисован до тех пор, пока кадр не будет "отрисован" перед ним.
        VkFenceCreateInfo FenceCreateInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VK_CHECK(vkCreateFence(Device.LogicalDevice, &FenceCreateInfo, allocator, &InFlightFences[i]));
    }

    // На этом этапе ограждений в полете еще не должно быть, поэтому очистите список. 
    // Они хранятся в указателях, поскольку начальное состояние должно быть 0 и будет 0, 
    // когда не используется. Актуальные заборы не входят в этот список.
    for (u32 i = 0; i < swapchain.ImageCount; ++i) {
        ImagesInFlight[i] = 0;
    }

    if (!CreateBuffers()) {
        MERROR("Не удалось создать буферы.")
        return;
    }

    // Отметить все геометрии как недействительные
    for (u32 i = 0; i < VULKAN_MAX_GEOMETRY_COUNT; ++i) {
        geometries[i].id = INVALID::ID;
    }

    MINFO("Средство визуализации Vulkan успешно инициализировано.");
}

VulkanAPI::~VulkanAPI()
{
    vkDeviceWaitIdle(Device.LogicalDevice);

    // Уничтожать в порядке, обратном порядку создания.

    ObjectVertexBuffer.Destroy(this);
    ObjectIndexBuffer.Destroy(this);

    // Синхронизация объектов
    for (u8 i = 0; i < swapchain.MaxFramesInFlight; ++i) {
        if (ImageAvailableSemaphores[i]) {
            vkDestroySemaphore(
                Device.LogicalDevice,
                ImageAvailableSemaphores[i],
                allocator);
            ImageAvailableSemaphores[i] = 0;
        }
        if (QueueCompleteSemaphores[i]) {
            vkDestroySemaphore(
                Device.LogicalDevice,
                QueueCompleteSemaphores[i],
                allocator);
            QueueCompleteSemaphores[i] = 0;
        }
        vkDestroyFence(Device.LogicalDevice, InFlightFences[i], allocator);
    }

    // Буферы команд
    for (u32 i = 0; i < swapchain.ImageCount; ++i) {
        if (GraphicsCommandBuffers[i].handle) {
            VulkanCommandBufferFree(
                this,
                Device.GraphicsCommandPool,
                &GraphicsCommandBuffers[i]);
                GraphicsCommandBuffers[i].handle = 0;
        }
    }

    // Проход рендеринга (Renderpass)
    for (u64 i = 0; i < VULKAN_MAX_REGISTERED_RENDERPASSES; i++) {
        RenderpassDestroy(RegisteredPasses + i);
    }

    // Цепочка подкачки (Swapchain)
    swapchain.Destroy(this);

    MDEBUG("Уничтожение устройства Вулкан...");
    Device.Destroy(this);

    MDEBUG("Уничтожение поверхности Вулкана...");
    if (surface) {
        vkDestroySurfaceKHR(instance, surface, allocator);
        surface = 0;
    }

    MDEBUG("Уничтожение отладчика Vulkan...");
    if (DebugMessenger) {
        PFN_vkDestroyDebugUtilsMessengerEXT func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        func(instance, DebugMessenger, allocator);
    }
    MDEBUG("Уничтожение экземпляра Vulkan...");
    vkDestroyInstance(instance, allocator);
}

void VulkanAPI::ShutDown()
{
    this->~VulkanAPI();
}

void VulkanAPI::Resized(u16 width, u16 height)
{
    // Обновите «генерацию размера кадрового буфера», счетчик, 
    // который указывает, когда размер кадрового буфера был обновлен.
    FramebufferWidth = width;
    FramebufferHeight = height;
    FramebufferSizeGeneration++;

    MINFO("API рендеринга Vulkan-> изменен размер: w/h/gen: %i/%i/%llu", width, height, FramebufferSizeGeneration);
}

bool VulkanAPI::BeginFrame(f32 Deltatime)
{
    FrameDeltaTime = Deltatime;

    // Проверьте, не воссоздается ли цепочка подкачки заново, и загрузитесь.
    if (RecreatingSwapchain) {
        VkResult result = vkDeviceWaitIdle(Device.LogicalDevice);
        if (!VulkanResultIsSuccess(result)) {
            MERROR("Ошибка VulkanBeginFrame vkDeviceWaitIdle (1): '%shader'", VulkanResultString(result, true));
            return false;
        }
        MINFO("Воссоздание цепочки подкачки, загрузка.");
        return false;
    }

    // Проверьте, был ли изменен размер фреймбуфера. Если это так, необходимо создать новую цепочку подкачки.
    if (FramebufferSizeGeneration != FramebufferSizeLastGeneration) {
        VkResult result = vkDeviceWaitIdle(Device.LogicalDevice);
        if (!VulkanResultIsSuccess(result)) {
            MERROR("Ошибка VulkanBeginFrame vkDeviceWaitIdle (2): '%shader'", VulkanResultString(result, true));
            return false;
        }

        // Если воссоздание цепочки обмена не удалось (например, из-за того, 
        // что окно было свернуто), загрузитесь, прежде чем снимать флаг.
        if (!RecreateSwapchain()) {
            return false;
        }

        MINFO("Измененние размера, загрузка.");
        return false;
    }

    // Дождитесь завершения выполнения текущего кадра. Поскольку ограждение свободен, он сможет двигаться дальше.
    VkResult result = vkWaitForFences(Device.LogicalDevice, 1, &InFlightFences[CurrentFrame], true, UINT64_MAX);
    if (!VulkanResultIsSuccess(result)) {
        MERROR("Ошибка ожидания в полете! ошибка: %shader", VulkanResultString(result, true));
    }

    // Получаем следующее изображение из цепочки обмена. Передайте семафор, который должен сигнализировать, когда это завершится.
    // Этот же семафор позже будет ожидаться при отправке в очередь, чтобы убедиться, что это изображение доступно.
    if (!swapchain.AcquireNextImageIndex(
            this,
            UINT64_MAX,
            ImageAvailableSemaphores[CurrentFrame],
            0,
            &ImageIndex)) {
        return false;
    }

    // Начните запись команд.
    VulkanCommandBufferReset(&GraphicsCommandBuffers[ImageIndex]);
    VulkanCommandBufferBegin(&GraphicsCommandBuffers[ImageIndex], false, false, false);

    // Динамическое состояние
    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = (f32)FramebufferHeight;
    viewport.width = (f32)FramebufferWidth;
    viewport.height = -(f32)FramebufferHeight;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Ножницы
    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = FramebufferWidth;
    scissor.extent.height = FramebufferHeight;

    vkCmdSetViewport(GraphicsCommandBuffers[ImageIndex].handle, 0, 1, &viewport);
    vkCmdSetScissor(GraphicsCommandBuffers[ImageIndex].handle, 0, 1, &scissor);

    return true;
}

bool VulkanAPI::EndFrame(f32 DeltaTime)
{
    VulkanCommandBufferEnd(&GraphicsCommandBuffers[ImageIndex]);

    // Убедитесь, что предыдущий кадр не использует это изображение (т.е. его ограждение находится в режиме ожидания).
    if (ImagesInFlight[ImageIndex] != VK_NULL_HANDLE) { // был кадр
        VkResult result = vkWaitForFences(Device.LogicalDevice, 1, &ImagesInFlight[ImageIndex], true, UINT64_MAX);
        if (!VulkanResultIsSuccess(result)) {
            MFATAL("vkWaitForFences ошибка: %shader", VulkanResultString(result, true));
        }
    }

    // Отметьте ограждение изображения как используемое этим кадром.
    ImagesInFlight[ImageIndex] = InFlightFences[CurrentFrame];

    // Сбросьте ограждение для использования на следующем кадре
    VK_CHECK(vkResetFences(Device.LogicalDevice, 1, &InFlightFences[CurrentFrame]));

    // Отправьте очередь и дождитесь завершения операции.
    // Начать отправку в очередь
    VkSubmitInfo SubmitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};

    // Буфер(ы) команд, которые должны быть выполнены.
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &GraphicsCommandBuffers[ImageIndex].handle;

    // Семафор(ы), который(ы) будет сигнализирован при заполнении очереди.
    SubmitInfo.signalSemaphoreCount = 1;
    SubmitInfo.pSignalSemaphores = &QueueCompleteSemaphores[CurrentFrame];

    // Семафор ожидания гарантирует, что операция не может начаться до тех пор, пока изображение не будет доступно.
    SubmitInfo.waitSemaphoreCount = 1;
    SubmitInfo.pWaitSemaphores = &ImageAvailableSemaphores[CurrentFrame];

    // Каждый семафор ожидает завершения соответствующего этапа конвейера. Соотношение 1:1.
    // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT предотвращает выполнение последующих 
    // записей вложений цвета до тех пор, пока не поступит сигнал семафора (т. е. за раз будет представлен один кадр)
    VkPipelineStageFlags flags[1] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    SubmitInfo.pWaitDstStageMask = flags;

    VkResult result = vkQueueSubmit(
        Device.GraphicsQueue,
        1,
        &SubmitInfo,
        InFlightFences[CurrentFrame]);
    if (result != VK_SUCCESS) {
        MERROR("vkQueueSubmit не дал результата: %shader", VulkanResultString(result, true));
        return false;
    }

    VulkanCommandBufferUpdateSubmitted(&GraphicsCommandBuffers[ImageIndex]);
    // Отправка в конечную очередь

    // Верните изображение обратно в swapchain.
    swapchain.Present(
        this,
        Device.GraphicsQueue,
        Device.PresentQueue,
        QueueCompleteSemaphores[CurrentFrame],
        ImageIndex
    );

    return true;
}

bool VulkanAPI::RenderpassBegin(Renderpass* pass, RenderTarget& target)
{
    auto& CommandBuffer = GraphicsCommandBuffers[ImageIndex];

    // Начало этапа рендеринга.
    auto VkRenderpass = reinterpret_cast<VulkanRenderpass*>(pass->InternalData);

    VkRenderPassBeginInfo BeginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    BeginInfo.renderPass = VkRenderpass->handle;
    BeginInfo.framebuffer = reinterpret_cast<VkFramebuffer>(target.InternalFramebuffer);
    BeginInfo.renderArea.offset.x = pass->RenderArea.x;
    BeginInfo.renderArea.offset.y = pass->RenderArea.y;
    BeginInfo.renderArea.extent.width = pass->RenderArea.z;
    BeginInfo.renderArea.extent.height = pass->RenderArea.w;

    BeginInfo.clearValueCount = 0;
    BeginInfo.pClearValues = 0;

    VkClearValue ClearValues[2]{};
    bool DoClearColour = (pass->ClearFlags & RenderpassClearFlag::ColourBuffer) != 0;
    if (DoClearColour) {
        MMemory::CopyMem(ClearValues[BeginInfo.clearValueCount].color.float32, pass->ClearColour.elements, sizeof(f32) * 4);
        BeginInfo.clearValueCount++;
    } else {
        // Все равно добавьте его, но не беспокойтесь о копировании данных, так как они будут проигнорированы.
        BeginInfo.clearValueCount++;
    }

    bool DoClearDepth = (pass->ClearFlags & RenderpassClearFlag::DepthBuffer) != 0;
    if (DoClearDepth) {
        MMemory::CopyMem(ClearValues[BeginInfo.clearValueCount].color.float32, pass->ClearColour.elements, sizeof(f32) * 4);
        ClearValues[BeginInfo.clearValueCount].depthStencil.depth = VkRenderpass->depth;

        bool DoClearStencil = (pass->ClearFlags & RenderpassClearFlag::StencilBuffer) != 0;
        ClearValues[BeginInfo.clearValueCount].depthStencil.stencil = DoClearStencil ? VkRenderpass->stencil : 0;
        BeginInfo.clearValueCount++;
    }

    BeginInfo.pClearValues = BeginInfo.clearValueCount > 0 ? ClearValues : 0;

    vkCmdBeginRenderPass(CommandBuffer.handle, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    CommandBuffer.state = COMMAND_BUFFER_STATE_IN_RENDER_PASS;

    return true;
}

bool VulkanAPI::RenderpassEnd(Renderpass* pass)
{
    VulkanCommandBuffer& CommandBuffer = GraphicsCommandBuffers[ImageIndex];

    // Завершение рендеринга.
    vkCmdEndRenderPass(CommandBuffer.handle);
    CommandBuffer.state = COMMAND_BUFFER_STATE_RECORDING;
    return true;
}

Renderpass *VulkanAPI::GetRenderpass(const MString& name)
{
    if (!name || name[0] == '\0') {
        MERROR("VulkanAPI::GetRenderpass требует имя. Ничего не будет возвращено.");
        return nullptr;
    }

    u32 id = INVALID::ID;
    RenderpassTable.Get(name, &id);
    if (id == INVALID::ID) {
        MWARN("Нет зарегистрированного рендер-пасса с именем «%s».", name.c_str());
        return nullptr;
    }

    return &RegisteredPasses[id];
}

void VulkanAPI::Load(const u8* pixels, Texture *texture)
{
    // Создание внутренних данных.
    u32 ImageSize = texture->width * texture->height * texture->ChannelCount;

    // ПРИМЕЧАНИЕ: Предполагается, что на канал приходится 8 бит.
    VkFormat ImageFormat = VK_FORMAT_R8G8B8A8_UNORM;

    // ПРИМЕЧАНИЕ. Здесь много предположений, для разных типов текстур потребуются разные параметры.
    texture->Data = new VulkanImage(
        this,
        texture->type,
        texture->width,
        texture->height,
        ImageFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        true,
        VK_IMAGE_ASPECT_COLOR_BIT);

    TextureWriteData(texture, 0, ImageSize, pixels);
    texture->generation++;
}

VkFormat ChannelCountToFormat(u8 ChannelCount, VkFormat DefaultFormat) {
    switch (ChannelCount) {
        case 1:
            return VK_FORMAT_R8_UNORM;
        case 2:
            return VK_FORMAT_R8G8_UNORM;
        case 3:
            return VK_FORMAT_R8G8B8_UNORM;
        case 4:
            return VK_FORMAT_R8G8B8A8_UNORM;
        default:
            return DefaultFormat;
    }
}

void VulkanAPI::LoadTextureWriteable(Texture *texture)
{
    VkFormat ImageFormat = ChannelCountToFormat(texture->ChannelCount, VK_FORMAT_R8G8B8A8_UNORM);
    // ЗАДАЧА: здесь много предположений, разные типы текстур потребуют разных опций.
    texture->Data = new VulkanImage(
        this,
        texture->type,
        texture->width,
        texture->height,
        ImageFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        true,
        VK_IMAGE_ASPECT_COLOR_BIT);
    texture->generation++;
}

void VulkanAPI::TextureResize(Texture *texture, u32 NewWidth, u32 NewHeight)
{
    if (texture && texture->Data) {
        // Изменение размера на самом деле просто разрушает старое изображение и создает новое. 
        // Данные не сохраняются, поскольку не существует надежного способа сопоставить старые 
        // данные с новыми, поскольку объем данных различается.
        texture->Data->Destroy(this);

        VkFormat ImageFormat = ChannelCountToFormat(texture->ChannelCount, VK_FORMAT_R8G8B8A8_UNORM);

        // ЗАДАЧА: здесь много предположений, разные типы текстур потребуют разных опций.
        texture->Data->Create(
        this,
        texture->type,
        NewWidth,
        NewHeight,
        ImageFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        true,
        VK_IMAGE_ASPECT_COLOR_BIT);

        texture->generation++;
    }
}

void VulkanAPI::TextureWriteData(Texture *texture, u32 offset, u32 size, const u8 *pixels)
{
    VkDeviceSize ImageSize = texture->width * texture->height * texture->ChannelCount * (texture->type == TextureType::Cube ? 6 : 1);

    VkFormat ImageFormat = ChannelCountToFormat(texture->ChannelCount, VK_FORMAT_R8G8B8A8_UNORM);

    // Создайте промежуточный буфер и загрузите в него данные.
    VkMemoryPropertyFlags MemoryPropFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VulkanBuffer staging{this, ImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, MemoryPropFlags, true, false};

    staging.LoadData(this, 0, ImageSize, 0, pixels);

    VulkanCommandBuffer TempBuffer;
    const VkCommandPool& pool = Device.GraphicsCommandPool;
    const VkQueue& queue = Device.GraphicsQueue;
    VulkanCommandBufferAllocateAndBeginSingleUse(this, pool, &TempBuffer);

    // Переведите макет от текущего к оптимальному для получения данных.
    texture->Data->TransitionLayout(this, texture->type, &TempBuffer, ImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Скопируйте данные из буфера.
    texture->Data->CopyFromBuffer(this, texture->type, staging.handle, &TempBuffer);

    // Переход от оптимального для приема данных к оптимальному макету, доступному только для чтения шейдеров.
    texture->Data->TransitionLayout(
        this, 
        texture->type,
        &TempBuffer, 
        ImageFormat, 
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    VulkanCommandBufferEndSingleUse(this, pool, &TempBuffer, queue);

    staging.Destroy(this);

    texture->generation++;
}

void VulkanAPI::Unload(Texture *texture)
{
    vkDeviceWaitIdle(Device.LogicalDevice);

    if (texture->Data) {
        texture->Data->Destroy(this);

        delete texture->Data;
    }
    //kzero_memory(texture, sizeof(struct texture));
}

bool VulkanAPI::Load(GeometryID *gid, u32 VertexSize, u32 VertexCount, const void *vertices, u32 IndexSize, u32 IndexCount, const void *indices)
{
    if (!VertexCount || !vertices) {
        MERROR("VulkanAPI::LoadGeometry требует данных вершин, но они не были предоставлены. Количество вершин=%d, вершины=%p", VertexCount, vertices);
        return false;
    }
    // Проверьте, не повторная ли это загрузка. Если это так, необходимо впоследствии освободить старые данные.
    bool IsReupload = gid->InternalID != INVALID::ID;
    Geometry OldRange;

    Geometry* geometry = nullptr;
    if (IsReupload) {
        geometry = &this->geometries[gid->InternalID];

        // Скопируйте старый диапазон.
        OldRange = geometry;
    } else {
        for (u32 i = 0; i < VULKAN_MAX_GEOMETRY_COUNT; ++i) {
            if (this->geometries[i].id == INVALID::ID) {
                // Найден свободный индекс.
                gid->InternalID = i;
                this->geometries[i].id = i;
                geometry = &this->geometries[i];
                break;
            }
        }
    }
    if (!geometry) {
        MFATAL("VulkanRenderer::Load не удалось найти свободный индекс для загрузки новой геометрии. Настройте конфигурацию, чтобы обеспечить больше.");
        return false;
    }

    VkCommandPool &pool = this->Device.GraphicsCommandPool;
    VkQueue &queue = this->Device.GraphicsQueue;

    // Данные вершин.
    *geometry = Geometry(VertexCount, VertexSize, IndexCount);
    u32 TotalSize = VertexCount * VertexSize;
    if (!UploadDataRange(
            pool, 
            0, 
            queue, 
            this->ObjectVertexBuffer, 
            geometry->VertexBufferOffset, 
            TotalSize, 
            vertices)) {
        MERROR("VulkanAPI::LoadGeometry не удалось загрузить в буфер вершин!");
        return false;
    }

    // Данные индексов, если применимо
    if (IndexCount && indices) {
        TotalSize = IndexCount * IndexSize;
        if (!UploadDataRange(
                pool, 
                0, 
                queue, 
                this->ObjectIndexBuffer, 
                geometry->IndexBufferOffset, 
                TotalSize, 
                indices)) {
            MERROR("VulkanAPI::LoadGeometry не удалось загрузить в индексный буфер!");
            return false;
        }
    }

    if (gid->generation == INVALID::U16ID) {
        gid->generation = 0;
    } else {
        gid->generation++;
    }

    if (IsReupload) {
        // Освобождение данных вершин
        FreeDataRange(&this->ObjectVertexBuffer, OldRange.VertexBufferOffset, OldRange.VertexElementSize * OldRange.VertexCount);

        // Освобождение данных индексов, если применимо
        if (OldRange.IndexElementSize > 0) {
            FreeDataRange(&this->ObjectIndexBuffer, OldRange.IndexBufferOffset, OldRange.IndexElementSize * OldRange.IndexCount);
        }
    }

    return true;
}

void VulkanAPI::Unload(GeometryID *gid)
{
    if (gid && gid->InternalID != INVALID::ID) {
        vkDeviceWaitIdle(this->Device.LogicalDevice);
        Geometry& vGeometry = this->geometries[gid->InternalID];

        // Освобождение данных вершин
        FreeDataRange(&this->ObjectVertexBuffer, vGeometry.VertexBufferOffset, vGeometry.VertexElementSize * vGeometry.VertexCount);

        // Освобождение данных индексов, если это применимо
        if (vGeometry.IndexElementSize > 0) {
            FreeDataRange(&this->ObjectIndexBuffer, vGeometry.IndexBufferOffset, vGeometry.IndexElementSize * vGeometry.IndexCount);
        }

        // Очистка данных.
        vGeometry.Destroy(); //MMemory::ZeroMem(&this->geometries[geometry->InternalID], sizeof(Geometry));
        //geometry->id = INVALID::U32ID;
        //geometry->generation = INVALID::U32ID;
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT MessageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
    void* UserData) 
{
    switch (MessageSeverity) {
        default:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            MERROR(CallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            MWARN(CallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            MINFO(CallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            MTRACE(CallbackData->pMessage);
            break;
    }
    return VK_FALSE;
}

i32 VulkanAPI::FindMemoryIndex(u32 TypeFilter, VkMemoryPropertyFlags PropertyFlags)
{
    VkPhysicalDeviceMemoryProperties MemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(Device.PhysicalDevice, &MemoryProperties);

    for (u32 i = 0; i < MemoryProperties.memoryTypeCount; ++i) {
        // Проверьте каждый тип памяти, чтобы увидеть, установлен ли его бит в 1.
        if (TypeFilter & (1 << i) && (MemoryProperties.memoryTypes[i].propertyFlags & PropertyFlags) == PropertyFlags) {
            return i;
        }
    }

    MWARN("Не удалось найти подходящий тип памяти!");
    return -1;
}

void VulkanAPI::CreateCommandBuffers()
{
    if (GraphicsCommandBuffers.Capacity() == 0) {
        GraphicsCommandBuffers.Resize(swapchain.ImageCount);
    }

    for (u32 i = 0; i < swapchain.ImageCount; ++i) {
        if (GraphicsCommandBuffers[i].handle) {
            VulkanCommandBufferFree(
                this,
                Device.GraphicsCommandPool,
                &GraphicsCommandBuffers[i]);
        } 
        //MMemory::ZeroMemory(&GraphicsCommandBuffers[i], sizeof(VulkanCommandBuffer));
        VulkanCommandBufferAllocate(
            this,
            Device.GraphicsCommandPool,
            true,
            &this->GraphicsCommandBuffers[i]);
    }

    MDEBUG("Созданы командные буферы Vulkan.");
}

bool VulkanAPI::CreateBuffers()
{
    VkMemoryPropertyFlagBits MemoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    // Буфер вершин геометрии
    const u64 VertexBufferSize = sizeof(Vertex3D) * 1024 * 1024;
    if (!ObjectVertexBuffer.Create(
            this,
            VertexBufferSize,
            static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
            MemoryPropertyFlags,
            true,
            true)) {
        MERROR("Ошибка создания вершинного буфера.");
        return false;
    }
    //GeometryVertexOffset = 0;

    // Буфер индексов геометрии
    const u64 IndexBufferSize = sizeof(u32) * 1024 * 1024;
    if (!ObjectIndexBuffer.Create(
            this,
            IndexBufferSize,
            static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
            MemoryPropertyFlags,
            true,
            true)) {
        MERROR("Ошибка создания вершинного буфера.");
        return false;
    }
    //GeometryVertexOffset = 0; 
    
    return true;
}

bool VulkanAPI::CreateModule(VulkanShader *shader, const VulkanShaderStageConfig& config, VulkanShaderStage *ShaderStage)
{
    auto ResourceSystemInst = ResourceSystem::Instance();
    // Прочтите ресурс.
    Resource BinaryResource;
    if (!ResourceSystemInst->Load(config.FileName, ResourceType::Binary, nullptr, BinaryResource)) {
        MERROR("Невозможно прочитать модуль шейдера: %s.", config.FileName);
        return false;
    }

    // MMemory::ZeroMem(&ShaderStage->CreateInfo, sizeof(VkShaderModuleCreateInfo));
    ShaderStage->CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    // Используйте размер и данные ресурса напрямую.
    ShaderStage->CreateInfo.codeSize = BinaryResource.DataSize;
    ShaderStage->CreateInfo.pCode = reinterpret_cast<u32*>(BinaryResource.data);

    VK_CHECK(vkCreateShaderModule(
        Device.LogicalDevice,
        &ShaderStage->CreateInfo,
        allocator,
        &ShaderStage->handle));

    // Освободите ресурс.
    ResourceSystemInst->Unload(BinaryResource);

    // Информация об этапе шейдера
    //MMemory::ZeroMem(&ShaderStage->ShaderStageCreateInfo, sizeof(VkPipelineShaderStageCreateInfo));
    ShaderStage->ShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStage->ShaderStageCreateInfo.stage = config.stage;
    ShaderStage->ShaderStageCreateInfo.module = ShaderStage->handle;
    ShaderStage->ShaderStageCreateInfo.pName = "main";

    return true;
}

bool VulkanAPI::UploadDataRange(VkCommandPool pool, VkFence fence, VkQueue queue, VulkanBuffer &buffer, u64 &OutOffset, u64 size, const void *data)
{
    // Выделить место в буфере.
    if (!buffer.Allocate(size, OutOffset)) {
        MERROR("VulkanAPI::UploadDataRange не удалось выделить данные из данного буфера!");
        return false;
    }

    // Создание промежуточного буфера, видимого хосту, для загрузки. Отметьте его как источник передачи.
    VkBufferUsageFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VulkanBuffer staging;
    staging.Create(this, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, flags, true, false);

    // Загрузка данных в промежуточный буфер.
    staging.LoadData(this, 0, size, 0, data);

    // Копирование из промежуточного хранилища в локальный буфер устройства.
    staging.CopyTo(this, pool, fence, queue, 0, buffer.handle, OutOffset, size);

    // Очистка промежуточного буфера.
    staging.Destroy(this);

    return true;
}

void VulkanAPI::FreeDataRange(VulkanBuffer *buffer, u64 offset, u64 size)
{
    if (buffer) {
        buffer->Free(size, offset);
    }
}

bool VulkanAPI::RecreateSwapchain()
{
    // Если уже воссоздается, не повторяйте попытку.
    if (RecreatingSwapchain) {
        MDEBUG("RecreateSwapchain вызывается при повторном создании. Загрузка.");
        return false;
    }

    // Определите, не слишком ли мало окно для рисования
    if (FramebufferWidth == 0 || FramebufferHeight == 0) {
        MDEBUG("RecreateSwapchain вызывается, когда окно <1 в измерении. Загрузка.");
        return false;
    }

    // Пометить как воссоздаваемый, если размеры действительны.
    RecreatingSwapchain = true;

    // Дождитесь завершения всех операций.
    vkDeviceWaitIdle(Device.LogicalDevice);

    // Уберите это на всякий случай.
    for (u32 i = 0; i < swapchain.ImageCount; ++i) {
        ImagesInFlight[i] = nullptr;
    }

    // Поддержка запроса ЗАДАЧА: переделать
    Device.QuerySwapchainSupport(Device.PhysicalDevice, surface, &Device.SwapchainSupport);
    Device.DetectDepthFormat();

    swapchain.Recreate(this, FramebufferWidth, FramebufferHeight);

    // Обновить генерацию размера кадрового буфера.
    FramebufferSizeLastGeneration = FramebufferSizeGeneration;

    // Очистка цепочки подкачки
    for (u32 i = 0; i < swapchain.ImageCount; ++i) {
        VulkanCommandBufferFree(this, Device.GraphicsCommandPool, &GraphicsCommandBuffers[i]);
    }

    // Сообщите рендереру, что требуется обновление.
    if (OnRendertargetRefreshRequired) {
        OnRendertargetRefreshRequired.Run();
    }

    CreateCommandBuffers();

    // Снимите флаг воссоздания.
    RecreatingSwapchain = false;

    return true;
}

bool VulkanAPI::SetUniform(Shader *shader, ShaderUniform *uniform, const void *value)
{
    VulkanShader* VkShader = shader->ShaderData;
    if (uniform->type == ShaderUniformType::Sampler) {
        if (uniform->scope == ShaderScope::Global) {
            shader->GlobalTextureMaps[uniform->location] = (TextureMap*)value;
        } else {
            VkShader->InstanceStates[shader->BoundInstanceID].InstanceTexturesMaps[uniform->location] = (TextureMap*)value;
        }
    } else {
        if (uniform->scope == ShaderScope::Local) {
            // Является локальным, использует push-константы. Сделайте это немедленно.
            VkCommandBuffer CommandBuffer = GraphicsCommandBuffers[ImageIndex].handle;
            vkCmdPushConstants(CommandBuffer, VkShader->pipeline.PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, uniform->offset, uniform->size, value);
        } else {
            // Сопоставьте подходящую ячейку памяти и скопируйте данные.
            u64 addr = (u64)VkShader->MappedUniformBufferBlock;
            addr += shader->BoundUboOffset + uniform->offset;
            MMemory::CopyMem((void*)addr, value, uniform->size);
            if (addr) {
            }
        }
    }
    return true;
}

bool VulkanAPI::TextureMapAcquireResources(TextureMap *map)
{
    // Создайте сэмплер для текстуры
    VkSamplerCreateInfo SamplerInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

    SamplerInfo.minFilter = ConvertFilterType("min", map->FilterMinify);
    SamplerInfo.magFilter = ConvertFilterType("mag", map->FilterMagnify);

    SamplerInfo.addressModeU = ConvertRepeatType("U", map->RepeatU);
    SamplerInfo.addressModeV = ConvertRepeatType("V", map->RepeatV);
    SamplerInfo.addressModeW = ConvertRepeatType("W", map->RepeatW);

    // ЗАДАЧА: Настраивается
    SamplerInfo.anisotropyEnable = VK_TRUE;
    SamplerInfo.maxAnisotropy = 16;
    SamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    SamplerInfo.unnormalizedCoordinates = VK_FALSE;
    SamplerInfo.compareEnable = VK_FALSE;
    SamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    SamplerInfo.mipLodBias = 0.F;
    SamplerInfo.minLod = 0.F;
    SamplerInfo.maxLod = 0.F;

    VkResult result = vkCreateSampler(Device.LogicalDevice, &SamplerInfo, allocator, reinterpret_cast<VkSampler*>(&map->sampler));
    if (!VulkanResultIsSuccess(VK_SUCCESS)) {
        MERROR("Ошибка создания сэмплера текстуры: %s.", VulkanResultString(result, true));
        return false;
    }

    return true;
}

void VulkanAPI::TextureMapReleaseResources(TextureMap *map)
{
    if (map) {
        vkDestroySampler(Device.LogicalDevice, reinterpret_cast<VkSampler>(map->sampler), allocator);
        MMemory::ZeroMem(&map->sampler, sizeof(VkSampler)); 
        map->sampler = nullptr;
    }
}

void VulkanAPI::RenderTargetCreate(u8 AttachmentCount, Texture **attachments, Renderpass *pass, u32 width, u32 height, RenderTarget &OutTarget)
{
    // Максимальное количество вложений
    VkImageView AttachmentViews[32];
    for (u32 i = 0; i < AttachmentCount; ++i) {
        AttachmentViews[i] = (attachments[i]->Data)->view;
    }

    // Сделайте копию вложений и посчитайте.
    OutTarget.AttachmentCount = AttachmentCount;
    if (!OutTarget.attachments) {
        OutTarget.attachments = MMemory::TAllocate<Texture*>(Memory::Array, AttachmentCount);
    }
    MMemory::CopyMem(OutTarget.attachments, attachments, sizeof(Texture*) * AttachmentCount);

    VkFramebufferCreateInfo FramebufferCreateInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    FramebufferCreateInfo.renderPass = reinterpret_cast<VulkanRenderpass*>(pass->InternalData)->handle;
    FramebufferCreateInfo.attachmentCount = AttachmentCount;
    FramebufferCreateInfo.pAttachments = AttachmentViews;
    FramebufferCreateInfo.width = width;
    FramebufferCreateInfo.height = height;
    FramebufferCreateInfo.layers = 1;

    VK_CHECK(vkCreateFramebuffer(Device.LogicalDevice, &FramebufferCreateInfo, allocator, (VkFramebuffer*)&OutTarget.InternalFramebuffer));
}

void VulkanAPI::RenderTargetDestroy(RenderTarget &target, bool FreeInternalMemory)
{
    if (target.InternalFramebuffer) {
        vkDestroyFramebuffer(Device.LogicalDevice, (VkFramebuffer)target.InternalFramebuffer, allocator);
        target.InternalFramebuffer = 0;
        if (FreeInternalMemory) {
            target.~RenderTarget();
        }
    }
}

void VulkanAPI::RenderpassCreate(Renderpass *OutRenderpass, f32 depth, u32 stencil, bool HasPrevPass, bool HasNextPass)
{
    OutRenderpass->InternalData = new VulkanRenderpass(OutRenderpass->ClearFlags, depth, stencil, HasPrevPass, HasNextPass, this);
}

void VulkanAPI::RenderpassDestroy(Renderpass *OutRenderpass)
{
    reinterpret_cast<VulkanRenderpass*>(OutRenderpass->InternalData)->Destroy(this);
}

Texture *VulkanAPI::WindowAttachmentGet(u8 index)
{
    if (index >= swapchain.ImageCount) {
        MFATAL("Попытка получить индекс вложения вне диапазона: %d. Количество вложений: %d", index, swapchain.ImageCount);
        return nullptr;
    }

    return swapchain.RenderTextures[index];
}

Texture *VulkanAPI::DepthAttachmentGet()
{
    return swapchain.DepthTexture;
}

u8 VulkanAPI::WindowAttachmentIndexGet()
{
    return ImageIndex;
}

void *VulkanAPI::operator new(u64 size)
{
    return LinearAllocator::Instance().Allocate(size);
}