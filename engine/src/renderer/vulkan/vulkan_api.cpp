#include "vulkan_api.hpp"
#include "vulkan_swapchain.hpp"
#include "core/application.hpp"
#include "vulkan_platform.hpp"
#include "systems/material_system.hpp"
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

    Geometry BufferData = this->geometries[data.gid->InternalID];
    VulkanCommandBuffer& CommandBuffer = this->GraphicsCommandBuffers[this->ImageIndex];

    // Привязка буфера вершин к смещению.
    VkDeviceSize offsets[1] = {BufferData.VertexBufferOffset};
    vkCmdBindVertexBuffers(CommandBuffer.handle, 0, 1, &ObjectVertexBuffer.handle, static_cast<VkDeviceSize*>(offsets));

    // Рисовать индексированные или неиндексированные.
    if (BufferData.IndexCount > 0) {
        // Привязать индексный буфер по смещению.
        vkCmdBindIndexBuffer(CommandBuffer.handle, this->ObjectIndexBuffer.handle, BufferData.IndexBufferOffset, VK_INDEX_TYPE_UINT32);

        // Issue the draw.
        vkCmdDrawIndexed(CommandBuffer.handle, BufferData.IndexCount, 1, 0, 0, 0);
    } else {
        vkCmdDraw(CommandBuffer.handle, BufferData.VertexCount, 1, 0, 0);
    }
}

const u32 DESC_SET_INDEX_GLOBAL   = 0;  // Индекс набора глобальных дескрипторов.
const u32 DESC_SET_INDEX_INSTANCE = 1;  // Индекс набора дескрипторов экземпляра.
const u32 BINDING_INDEX_UBO       = 0;  // Индекс привязки УБО.
const u32 BINDING_INDEX_SAMPLER   = 1;  // Индекс привязки сэмплера изображения.

bool VulkanAPI::Load(Shader *shader, u8 RenderpassID, u8 StageCount, DArray<char*> StageFilenames, const ShaderStage *stages)
{
    shader->ShaderData = MMemory::Allocate(sizeof(VulkanShader), MemoryTag::Renderer);

    // СДЕЛАТЬ: динамические проходы рендеринга
    VulkanRenderpass* renderpass = RenderpassID == 1 ? &MainRenderpass : &UI_Renderpass;

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

    // СДЕЛАТЬ: настраиваемый максимальный счетчик выделения дескриптора.

    u32 MaxDescriptorAllocateCount = 1024;

    // Скопируйте указатель на контекст.
    VulkanShader* OutShader = reinterpret_cast<VulkanShader*>(shader->ShaderData);

    OutShader->renderpass = renderpass;

    // Создайте конфигурацию.
    OutShader->config.MaxDescriptorSetCount = MaxDescriptorAllocateCount;

    // Этапы шейдера. Разбираем флаги.
    MMemory::ZeroMem(OutShader->config.stages, sizeof(VulkanShaderStageConfig) * VulkanShaderConstants::MaxStages);
    OutShader->config.StageCount = 0;
    // Перебрать предоставленные этапы.
    for (u32 i = 0; i < StageCount; i++) {
        // Убедитесь, что достаточно места для добавления сцены.
        if (OutShader->config.StageCount + 1 >  VulkanShaderConstants::MaxStages) {
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

    // Обнуляем массивы и подсчитываем.
    MMemory::ZeroMem(OutShader->config.DescriptorSets, sizeof(VulkanDescriptorSetConfig) * 2);

    // Массив атрибутов.
    MMemory::ZeroMem(OutShader->config.attributes, sizeof(VkVertexInputAttributeDescription) * VulkanShaderConstants::MaxAttributes);

    // На данный момент шейдеры будут иметь только эти два типа пулов дескрипторов.
    OutShader->config.PoolSizes[0] = VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024};          // HACK: максимальное количество наборов дескрипторов ubo.
    OutShader->config.PoolSizes[1] = VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4096};  // HACK: максимальное количество наборов дескрипторов сэмплера изображений.

    // Конфигурация глобального набора дескрипторов.
    VulkanDescriptorSetConfig GlobalDescriptorSetConfig = {};

    // УБО всегда доступен и первый.
    GlobalDescriptorSetConfig.bindings[BINDING_INDEX_UBO].binding = BINDING_INDEX_UBO;
    GlobalDescriptorSetConfig.bindings[BINDING_INDEX_UBO].descriptorCount = 1;
    GlobalDescriptorSetConfig.bindings[BINDING_INDEX_UBO].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    GlobalDescriptorSetConfig.bindings[BINDING_INDEX_UBO].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    GlobalDescriptorSetConfig.BindingCount++;

    OutShader->config.DescriptorSets[DESC_SET_INDEX_GLOBAL] = GlobalDescriptorSetConfig;
    OutShader->config.DescriptorSetCount++;
    if (shader->UseInstances) {
        // Если вы используете экземпляры, добавьте второй набор дескрипторов.
        VulkanDescriptorSetConfig InstanceDescriptorSetConfig = {};

        // Добавьте к нему UBO, так как в экземплярах он всегда должен быть доступен.
        // ПРИМЕЧАНИЕ: Возможно, будет хорошей идеей добавить это только в том случае, если оно будет использоваться...
        InstanceDescriptorSetConfig.bindings[BINDING_INDEX_UBO].binding = BINDING_INDEX_UBO;
        InstanceDescriptorSetConfig.bindings[BINDING_INDEX_UBO].descriptorCount = 1;
        InstanceDescriptorSetConfig.bindings[BINDING_INDEX_UBO].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        InstanceDescriptorSetConfig.bindings[BINDING_INDEX_UBO].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        InstanceDescriptorSetConfig.BindingCount++;

        OutShader->config.DescriptorSets[DESC_SET_INDEX_INSTANCE] = InstanceDescriptorSetConfig;
        OutShader->config.DescriptorSetCount++;
    }

    // Сделайте недействительными все состояния экземпляра.
    // СДЕЛАТЬ: динамическим
    for (u32 i = 0; i < 1024; ++i) {
        OutShader->InstanceStates[i].id = INVALID::ID;
    }

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
        MMemory::Free(shader->ShaderData, sizeof(VulkanShader), MemoryTag::Renderer);
        shader->ShaderData = 0;
    }
}

bool VulkanAPI::ShaderInitialize(Shader *shader)
{
    VkDevice& LogicalDevice = Device.LogicalDevice;
    VulkanShader* VkShader = reinterpret_cast<VulkanShader*>(shader->ShaderData);

    // Создайте модуль для каждого этапа.
    MMemory::ZeroMem(VkShader->stages, sizeof(VulkanShaderStage) * VulkanShaderConstants::MaxStages);
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
        t[static_cast<int>(ShaderAttributeType::Float32)] = VK_FORMAT_R32_SFLOAT;
        t[static_cast<int>(ShaderAttributeType::Float32_2)] = VK_FORMAT_R32G32_SFLOAT;
        t[static_cast<int>(ShaderAttributeType::Float32_3)] = VK_FORMAT_R32G32B32_SFLOAT;
        t[static_cast<int>(ShaderAttributeType::Float32_4)] = VK_FORMAT_R32G32B32A32_SFLOAT;
        t[static_cast<int>(ShaderAttributeType::Int8)] = VK_FORMAT_R8_SINT;
        t[static_cast<int>(ShaderAttributeType::UInt8)] = VK_FORMAT_R8_UINT;
        t[static_cast<int>(ShaderAttributeType::Int16)] = VK_FORMAT_R16_SINT;
        t[static_cast<int>(ShaderAttributeType::UInt16)] = VK_FORMAT_R16_UINT;
        t[static_cast<int>(ShaderAttributeType::Int32)] = VK_FORMAT_R32_SINT;
        t[static_cast<int>(ShaderAttributeType::UInt32)] = VK_FORMAT_R32_UINT;
        types = t;
    }

    // Атрибуты процесса
    u32 AttributeCount = shader->attributes.Lenght();
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

    // Process uniforms.
    u32 UniformCount = shader->uniforms.Lenght();
    for (u32 i = 0; i < UniformCount; ++i) {
        // Для сэмплеров необходимо обновить привязки дескриптора. С другими видами униформы здесь ничего делать не нужно.
        if (shader->uniforms[i].type == ShaderUniformType::Sampler) {
            const u32 SetIndex = (shader->uniforms[i].scope == ShaderScope::Global ? DESC_SET_INDEX_GLOBAL : DESC_SET_INDEX_INSTANCE);
            VulkanDescriptorSetConfig* SetConfig = &VkShader->config.DescriptorSets[SetIndex];
            if (SetConfig->BindingCount < 2) {
                // Привязки пока нет, то есть это первый добавленный сэмплер.
                // Создайте привязку с одним дескриптором для этого семплера.
                SetConfig->bindings[BINDING_INDEX_SAMPLER].binding = BINDING_INDEX_SAMPLER;  // Всегда буду вторым.
                SetConfig->bindings[BINDING_INDEX_SAMPLER].descriptorCount = 1;              // По умолчанию 1, будет увеличиваться с каждым добавлением сэмплера до соответствующего уровня.
                SetConfig->bindings[BINDING_INDEX_SAMPLER].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                SetConfig->bindings[BINDING_INDEX_SAMPLER].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
                SetConfig->BindingCount++;
            } else {
                // Привязка для семплеров уже есть, поэтому просто добавьте к ней дескриптор.
                // Возьмите текущее количество дескрипторов в качестве местоположения и увеличьте количество дескрипторов.
                SetConfig->bindings[BINDING_INDEX_SAMPLER].descriptorCount++;
            }
        }
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
    MMemory::ZeroMem(VkShader->DescriptorSetLayouts, VkShader->config.DescriptorSetCount);
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

    // СДЕЛАТЬ: Кажется неправильным иметь их здесь, по крайней мере, в таком виде. 
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

    VkPipelineShaderStageCreateInfo StageCreateIfos[VulkanShaderConstants::MaxStages];
    MMemory::ZeroMem(StageCreateIfos, sizeof(VkPipelineShaderStageCreateInfo) * VulkanShaderConstants::MaxStages);
    for (u32 i = 0; i < VkShader->config.StageCount; ++i) {
        StageCreateIfos[i] = VkShader->stages[i].ShaderStageCreateInfo;
    }

    bool PipelineResult = VkShader->pipeline.Create(
        this,
        VkShader->renderpass,
        shader->AttributeStride,
        shader->attributes.Lenght(),
        VkShader->config.attributes,  // shader->attributes,
        VkShader->config.DescriptorSetCount,
        VkShader->DescriptorSetLayouts,
        VkShader->config.StageCount,
        StageCreateIfos,
        viewport,
        scissor,
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
    u32 DeviceLocalBits = Device.SupportsDeviceLocalHostVisible ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0;
    // СДЕЛАТЬ: Максимальное количество должно быть настраиваемым или, возможно, иметь долгосрочную поддержку изменения размера буфера.
    u64 TotalBufferSize = shader->GlobalUboStride + (shader->UboStride * VULKAN_MAX_MATERIAL_COUNT);  // global + (locals)
    if (!VkShader->UniformBuffer.Create(
            this,
            TotalBufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
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

VulkanAPI::~VulkanAPI()
{
    vkDeviceWaitIdle(Device.LogicalDevice);

    // Уничтожать в порядке, обратном порядку создания.

    ObjectVertexBuffer.Destroy(this);
    ObjectIndexBuffer.Destroy(this);

    MaterialShader.DestroyShaderModule(this);

    // Sync objects
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
    ImageAvailableSemaphores.~DArray();
    //ImageAvailableSemaphores = 0;

    QueueCompleteSemaphores.~DArray();
    //QueueCompleteSemaphores = 0;

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
    GraphicsCommandBuffers.~DArray();
    //GraphicsCommandBuffers = 0;

    // Уничтожить кадровые буферы.
    for (u32 i = 0; i < swapchain.ImageCount; ++i) {
        vkDestroyFramebuffer(Device.LogicalDevice, WorldFramebuffers[i], allocator);
        vkDestroyFramebuffer(Device.LogicalDevice, swapchain.framebuffers[i], allocator);
    }

    // Проход рендеринга (Renderpass)
    UI_Renderpass.Destroy(this);
    MainRenderpass.Destroy(this);

    // Цепочка подкачки (Swapchain)
    VulkanSwapchainDestroy(this, &swapchain);

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

bool VulkanAPI::Initialize(MWindow* window, const char* ApplicationName)
{
    // TODO: пользовательский allocator.
    allocator = NULL;

    Application::ApplicationGetFramebufferSize(CachedFramebufferWidth, CachedFramebufferHeight);
    FramebufferWidth = (CachedFramebufferWidth != 0) ? CachedFramebufferWidth : 800;
    FramebufferHeight = (CachedFramebufferHeight != 0) ? CachedFramebufferHeight : 600;
    CachedFramebufferWidth = 0;
    CachedFramebufferHeight = 0;

    // Настройка экземпляра Vulkan.
    VkApplicationInfo AppInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    AppInfo.apiVersion = VK_API_VERSION_1_2;
    AppInfo.pApplicationName = ApplicationName;
    AppInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    AppInfo.pEngineName = "Moon Engine";
    AppInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0); 

    VkInstanceCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    CreateInfo.pApplicationInfo = &AppInfo;

    // Получите список необходимых расширений
    
    DArray<const char*>RequiredExtensions;                              // Создаем массив указателей
    RequiredExtensions.PushBack(VK_KHR_SURFACE_EXTENSION_NAME);         // Общее расширение поверхности
    PlatformGetRequiredExtensionNames(RequiredExtensions);              // Расширения для конкретной платформы
#if defined(_DEBUG)
    RequiredExtensions.PushBack(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);     // утилиты отладки

    MDEBUG("Необходимые расширения:");
    u32 length = RequiredExtensions.Lenght();
    for (u32 i = 0; i < length; ++i) {
        MDEBUG(RequiredExtensions[i]);
    }
#endif

    CreateInfo.enabledExtensionCount = RequiredExtensions.Lenght();
    CreateInfo.ppEnabledExtensionNames = RequiredExtensions.Data(); //TODO: указателю ppEnabledExtensionNames присваевается адрес указателя массива после выхода из функции данные стираются

    // Уровни проверки.
    DArray<const char*> RequiredValidationLayerNames; // указатель на массив символов TODO: придумать как использовать строки или другой способ отображать занятую память
    u32 RequiredValidationLayerCount = 0;

// Если необходимо выполнить проверку, получите список имен необходимых слоев проверки 
// и убедитесь, что они существуют. Слои проверки следует включать только в нерелизных сборках.
#if defined(_DEBUG)
    MINFO("Уровни проверки включены. Перечисление...");

    // Список требуемых уровней проверки.
    RequiredValidationLayerNames.PushBack("VK_LAYER_KHRONOS_validation");
    RequiredValidationLayerCount = RequiredValidationLayerNames.Lenght();

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
            return false;
        }
    }
    MINFO("Присутствуют все необходимые уровни проверки.");
#endif

    CreateInfo.enabledLayerCount = RequiredValidationLayerCount;
    CreateInfo.ppEnabledLayerNames = RequiredValidationLayerNames.Data(); //TODO: указателю ppEnabledLayerNames присваевается адрес указателя массива после выхода из функции данные стираются

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
        return false;
    }
    MDEBUG("Поверхность Vulkan создана.");

    // Создание устройства
    if (!Device.Create(this)) {
        MERROR("Не удалось создать устройство!");
        return false;
    }

    // Swapchain
    VulkanSwapchainCreate(
        this,
        FramebufferWidth,
        FramebufferHeight,
        &swapchain
    );

    // World renderpass 
    MainRenderpass.Create(
        this,
        Vector4D<f32>(0, 0, this->FramebufferWidth, this->FramebufferHeight),
        Vector4D<f32>(0.0f, 0.0f, 0.2f, 1.0f),  // Темносиний цвет
        1.0f,
        0,
        static_cast<u8>(RenderpassClearFlag::ColourBufferFlag | RenderpassClearFlag::DepthBufferFlag | RenderpassClearFlag::StencilBufferFlag),
        false, true
    );

    //UI renderpass
    UI_Renderpass.Create(
        this,
        Vector4D<f32>(0, 0, this->FramebufferWidth, this->FramebufferHeight),
        Vector4D<f32>(0.0f, 0.0f, 0.0f, 0.0f),
        1.0f,
        0,
        static_cast<u8>(RenderpassClearFlag::NoneFlag),
        true, false
    );

    // Регенерировать цепочку подкачки и мировые фреймбуферы
    RegenerateFramebuffers();

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

    // Создание встроенных шейдеров
    if (!MaterialShader.Create(this)) {
        MERROR("Ошибка загрузки встроенного шейдера базового цвета (BasicLighting).");
        return false;
    }
    if (!UI_Shader.Create(this)) {
        MERROR("Ошибка загрузки встроенного шейдера пользовательского интерфейса.");
        return false;
    }

    CreateBuffers();

    // Отметить все геометрии как недействительные
    for (u32 i = 0; i < VULKAN_MAX_GEOMETRY_COUNT; ++i) {
        geometries[i].id = INVALID::ID;
    }

    MINFO("Средство визуализации Vulkan успешно инициализировано.");
    return true;
}

void VulkanAPI::ShutDown()
{
    this->~VulkanAPI();
}

void VulkanAPI::Resized(u16 width, u16 height)
{
    // Обновите «генерацию размера кадрового буфера», счетчик, 
    // который указывает, когда размер кадрового буфера был обновлен.
    CachedFramebufferWidth = width;
    CachedFramebufferHeight = height;
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
    if (!VulkanSwapchainAcquireNextImageIndex(
            this,
            &swapchain,
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

    // Обновляем размеры основного/мирового прохода рендеринга.
    MainRenderpass.RenderArea.z = FramebufferWidth;
    MainRenderpass.RenderArea.w = FramebufferHeight;
    // Также обновите размеры проход рендеринга пользовательского интерфейса.
    UI_Renderpass.RenderArea.z = FramebufferWidth;
    UI_Renderpass.RenderArea.w = FramebufferHeight;

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
    VulkanSwapchainPresent(
        this,
        &swapchain,
        Device.GraphicsQueue,
        Device.PresentQueue,
        QueueCompleteSemaphores[CurrentFrame],
        ImageIndex
    );

    return true;
}

bool VulkanAPI::BeginRenderpass(u8 RenderpassID)
{
    VulkanRenderpass* renderpass = nullptr;
    VkFramebuffer framebuffer = 0;
    VulkanCommandBuffer* CommandBuffer = &GraphicsCommandBuffers[ImageIndex];

    // Выберите рендерпасс на основе идентификатора.
    switch (RenderpassID) {
        case static_cast<u8>(BuiltinRenderpass::World):
            renderpass = &MainRenderpass;
            framebuffer = WorldFramebuffers[ImageIndex];
            break;
        case static_cast<u8>(BuiltinRenderpass::UI):
            renderpass = &UI_Renderpass;
            framebuffer = swapchain.framebuffers[ImageIndex];
            break;
        default:
            MERROR("VulkanRenderer::BeginRenderpass called on unrecognized renderpass id: %#02x", RenderpassID);
            return false;
    }

    // Начните этап рендеринга.
    renderpass->Begin(CommandBuffer, framebuffer);

    // Используйте соответствующий шейдер.
    switch (RenderpassID) {
        case static_cast<u8>(BuiltinRenderpass::World):
            MaterialShader.Use(this);
            break;
        case static_cast<u8>(BuiltinRenderpass::UI):
            UI_Shader.Use(this);
            break;
    }

    return true;
}

bool VulkanAPI::EndRenderpass(u8 RenderpassID)
{
    VulkanRenderpass* renderpass = nullptr;
    VulkanCommandBuffer* CommandBuffer = &GraphicsCommandBuffers[ImageIndex];

    // Choose a renderpass based on ID.
    switch (RenderpassID) {
        case static_cast<u8>(BuiltinRenderpass::World):
            renderpass = &MainRenderpass;
            break;
        case static_cast<u8>(BuiltinRenderpass::UI):
            renderpass = &UI_Renderpass;
            break;
        default:
            MERROR("VulkanRenderer::EndRenderpass вызывается по неизвестному идентификатору renderpass:  %#02x", RenderpassID);
            return false;
    }

    renderpass->End(CommandBuffer);
    return true;
}

bool VulkanAPI::Load(GeometryID *gid, u32 VertexSize, u32 VertexCount, const void* vertices, u32 IndexSize, u32 IndexCount, const void* indices)
{
    // Проверьте, не повторная ли это загрузка. Если это так, необходимо впоследствии освободить старые данные.
    bool IsReupload = gid->InternalID != INVALID::ID;
    Geometry OldRange;

    Geometry* geometry = nullptr;
    if (IsReupload) {
        geometry = &this->geometries[gid->InternalID];

        // Скопируйте старый диапазон.
        /* OldRange.IndexBufferOffset = geometry->IndexBufferOffset;
        OldRange.IndexCount = geometry->IndexCount;
        OldRange.IndexSize = geometry->IndexSize;
        OldRange.VertexBufferOffset = geometry->VertexBufferOffset;
        OldRange.VertexCount = geometry->VertexCount;
        OldRange.VertexSize = geometry->VertexSize;*/
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
    /*geometry->SetVertexData(VertexCount, sizeof(Vertex3D), this->GeometryVertexOffset);
    u32 TotalSize = VertexCount * VertexSize;
    UploadDataRange(
        pool, 
        0, 
        queue, 
        this->ObjectVertexBuffer, 
        geometry->VertexBufferOffset, 
        TotalSize, 
        vertices);
    // TODO: вместо этого следует поддерживать список свободной памяти
    this->GeometryVertexOffset += TotalSize;

    // Данные индексов, если применимо
    if (IndexCount && indices) {
        geometry->SetIndexData(IndexCount, this->GeometryIndexOffset);
        TotalSize = IndexCount * IndexSize;
        UploadDataRange(
            pool, 
            0, 
            queue, 
            this->ObjectIndexBuffer, 
            geometry->IndexBufferOffset, 
            TotalSize, 
            indices);
        // TODO: вместо этого следует поддерживать список свободной памяти
        this->GeometryIndexOffset += TotalSize;
    }*/

    if (gid->generation == INVALID::ID) {
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
    if (GraphicsCommandBuffers.Lenght() == 0) {
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

void VulkanAPI::RegenerateFramebuffers()
{
    u32& ImageCount = swapchain.ImageCount;
    for (u32 i = 0; i < ImageCount; ++i) {
        VkImageView WorldAttachments[2] = {swapchain.views[i], swapchain.DepthAttachment->view};
        VkFramebufferCreateInfo FramebufferCreateInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        FramebufferCreateInfo.renderPass = MainRenderpass.handle;
        FramebufferCreateInfo.attachmentCount = 2;
        FramebufferCreateInfo.pAttachments = WorldAttachments;
        FramebufferCreateInfo.width = FramebufferWidth;
        FramebufferCreateInfo.height = FramebufferHeight;
        FramebufferCreateInfo.layers = 1;

        VK_CHECK(vkCreateFramebuffer(Device.LogicalDevice, &FramebufferCreateInfo, allocator, &WorldFramebuffers[i]));

        // Кадровые буферы Swapchain (проход пользовательского интерфейса). Выводы в образы swapchain
        VkImageView UI_Attachments[1] = {swapchain.views[i]};
        VkFramebufferCreateInfo scFramebufferCreateInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        scFramebufferCreateInfo.renderPass = UI_Renderpass.handle;
        scFramebufferCreateInfo.attachmentCount = 1;
        scFramebufferCreateInfo.pAttachments = UI_Attachments;
        scFramebufferCreateInfo.width = FramebufferWidth;
        scFramebufferCreateInfo.height = FramebufferHeight;
        scFramebufferCreateInfo.layers = 1;

        VK_CHECK(vkCreateFramebuffer(Device.LogicalDevice, &scFramebufferCreateInfo, allocator, &swapchain.framebuffers[i]));
    }
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
            true)) {
        MERROR("Ошибка создания вершинного буфера.");
        return false;
    }
    //GeometryVertexOffset = 0; 
    
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
    staging.Create(this, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, flags, true);

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

    // Поддержка запроса
    Device.QuerySwapchainSupport(
        Device.PhysicalDevice,
        surface,
        &Device.SwapchainSupport);
    Device.DetectDepthFormat(&Device);

    VulkanSwapchainRecreate(
        this,
        CachedFramebufferWidth,
        CachedFramebufferHeight,
        &swapchain);

    // Синхронизируйте размер фреймбуфера с кэшированными размерами.
    FramebufferWidth = CachedFramebufferWidth;
    FramebufferHeight = CachedFramebufferHeight;
    MainRenderpass.RenderArea.z = FramebufferWidth;
    MainRenderpass.RenderArea.w = FramebufferHeight;
    CachedFramebufferWidth = 0;
    CachedFramebufferHeight = 0;

    // Обновить генерацию размера кадрового буфера.
    FramebufferSizeLastGeneration = FramebufferSizeGeneration;

    // Очистка цепочки подкачки
    for (u32 i = 0; i < swapchain.ImageCount; ++i) {
        VulkanCommandBufferFree(this, Device.GraphicsCommandPool, &GraphicsCommandBuffers[i]);
    }

    // Буферы кадров.
    for (u32 i = 0; i < swapchain.ImageCount; ++i) {
        vkDestroyFramebuffer(Device.LogicalDevice, WorldFramebuffers[i], allocator);
        vkDestroyFramebuffer(Device.LogicalDevice, swapchain.framebuffers[i], allocator);
    }

    MainRenderpass.RenderArea.x = 0;
    MainRenderpass.RenderArea.y = 0;
    MainRenderpass.RenderArea.z = FramebufferWidth;
    MainRenderpass.RenderArea.w = FramebufferHeight;

    // Восстановить цепочку обмена и мировые фреймбуферы
    RegenerateFramebuffers();

    CreateCommandBuffers();

    // Снимите флаг воссоздания.
    RecreatingSwapchain = false;

    return true;
}

void *VulkanAPI::operator new(u64 size)
{
    return LinearAllocator::Instance().Allocate(size);
}