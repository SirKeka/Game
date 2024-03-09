#include "vulkan_api.hpp"
#include "vulkan_platform.hpp"
#include "vulkan_device.hpp"
#include "vulkan_swapchain.hpp"
#include "vulkan_renderpass.hpp"
#include "vulkan_command_buffer.hpp"
#include "vulkan_framebuffer.hpp"
#include "vulkan_fence.hpp"
#include "vulkan_utils.hpp"
#include "vulkan_buffer.hpp"

#include "core/logger.hpp"
#include "core/mmemory.hpp"
#include "core/application.hpp"

#include "math/mvertex3D.hpp"

// Shaders
#include "shaders/vulkan_object_shader.hpp"


VkInstance VulkanAPI::instance;
VkAllocationCallbacks* VulkanAPI::allocator;
u32 VulkanAPI::CachedFramebufferWidth = 0;
u32 VulkanAPI::CachedFramebufferHeight = 0;

VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT MessageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
    void* UserData
);

VulkanAPI::~VulkanAPI()
{
    vkDeviceWaitIdle(Device->LogicalDevice);

    // Уничтожать в порядке, обратном порядку создания.

    ObjectVertexBuffer->Destroy(this);
    ObjectIndexBuffer->Destroy(this);
    delete ObjectVertexBuffer;
    delete ObjectIndexBuffer;

    ObjectShader->DestroyShaderModule(this);
    delete ObjectShader;

    // Sync objects
    for (u8 i = 0; i < swapchain.MaxFramesInFlight; ++i) {
        if (ImageAvailableSemaphores[i]) {
            vkDestroySemaphore(
                Device->LogicalDevice,
                ImageAvailableSemaphores[i],
                allocator);
            ImageAvailableSemaphores[i] = 0;
        }
        if (QueueCompleteSemaphores[i]) {
            vkDestroySemaphore(
                Device->LogicalDevice,
                QueueCompleteSemaphores[i],
                allocator);
            QueueCompleteSemaphores[i] = 0;
        }
        VulkanFenceDestroy(this, &InFlightFences[i]);
    }
    ImageAvailableSemaphores.~DArray();
    //ImageAvailableSemaphores = 0;

    QueueCompleteSemaphores.~DArray();
    //QueueCompleteSemaphores = 0;

    InFlightFences.~DArray();
    //InFlightFences = 0;

    ImagesInFlight.~DArray();
    //ImagesInFlight = 0;

    // Буферы команд
    for (u32 i = 0; i < swapchain.ImageCount; ++i) {
        if (GraphicsCommandBuffers[i].handle) {
            VulkanCommandBufferFree(
                this,
                Device->GraphicsCommandPool,
                &GraphicsCommandBuffers[i]);
                GraphicsCommandBuffers[i].handle = 0;
        }
    }
    GraphicsCommandBuffers.~DArray();
    //GraphicsCommandBuffers = 0;

    // Уничтожить кадровые буферы.
    for (u32 i = 0; i < swapchain.ImageCount; ++i) {
        VulkanFramebufferDestroy(this, &swapchain.framebuffers[i]);
    }

    // Проход рендеринга (Renderpass)
    VulkanRenderpassDestroy(this, &this->MainRenderpass);

    // Цепочка подкачки (Swapchain)
    VulkanSwapchainDestroy(this, &swapchain);

    MINFO("Уничтожение командных пулов...");
    vkDestroyCommandPool(
        Device->LogicalDevice,
        Device->GraphicsCommandPool,
        allocator);

    // Уничтожить логическое устройство
    MINFO("Уничтожение логического устройства...");
    if (Device->LogicalDevice) {
        vkDestroyDevice(Device->LogicalDevice, allocator);
        Device->LogicalDevice = 0;
    }

    MDEBUG("Уничтожение устройства Вулкан...");
    delete Device;

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
    AppInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 2, 0);
    AppInfo.pEngineName = "Moon Engine";
    AppInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 2, 0); 

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
        MINFO("Поиск слоя: %s...", RequiredValidationLayerNames[i]);
        bool found = false;
        for (u32 j = 0; j < AvailableLayerCount; ++j) {
            if (StringsEqual(RequiredValidationLayerNames[i], AvailableLayers[j].layerName)) {
                found = true;
                MINFO("Найдено.");
                break;
            }
        }

        if (!found) {
            MFATAL("Необходимый уровень проверки отсутствует: %s", RequiredValidationLayerNames[i]);
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
    Device = new VulkanDevice();
    if (!Device->Create(this)) {
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

    VulkanRenderpassCreate(
        this,
        &this->MainRenderpass,
        0, 0, this->FramebufferWidth, this->FramebufferHeight,
        0.0f, 0.0f, 0.2f, 1.0f,
        1.0f,
        0
    );

    // Буферы кадров цепочки подкачки.
    swapchain.framebuffers.Resize(swapchain.ImageCount);
    RegenerateFramebuffers();

    // Создайте буферы команд.
    CreateCommandBuffers();

    // Создайте объекты синхронизации.
    ImageAvailableSemaphores.Resize(swapchain.MaxFramesInFlight);
    QueueCompleteSemaphores.Resize(swapchain.MaxFramesInFlight);
    InFlightFences.Resize(swapchain.MaxFramesInFlight);

    for (u8 i = 0; i < swapchain.MaxFramesInFlight; ++i) {
        VkSemaphoreCreateInfo SemaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(Device->LogicalDevice, &SemaphoreCreateInfo, allocator, &ImageAvailableSemaphores[i]);
        vkCreateSemaphore(Device->LogicalDevice, &SemaphoreCreateInfo, allocator, &QueueCompleteSemaphores[i]);

        // Создайте ограждение в сигнальном состоянии, указывая, что первый кадр уже «отрисован».
        // Это не позволит приложению бесконечно ждать рендеринга первого кадра, 
        // поскольку он не может быть отрисован до тех пор, пока кадр не будет "отрисован" перед ним.
        VulkanFenceCreate(this, true, &InFlightFences[i]);
    }

    // На этом этапе ограждений в полете еще не должно быть, поэтому очистите список. 
    // Они хранятся в указателях, поскольку начальное состояние должно быть 0 и будет 0, 
    // когда не используется. Актуальные заборы не входят в этот список.
    ImagesInFlight.Resize(swapchain.ImageCount);
    for (u32 i = 0; i < swapchain.ImageCount; ++i) {
        ImagesInFlight[i] = 0;
    }

    // Создание встроенных шейдеров
    ObjectShader = new VulkanObjectShader();
    if (!ObjectShader->Create(this)) {
        MERROR("Ошибка загрузки встроенного шейдера базового цвета (BasicLighting).");
        return false;
    }

    CreateBuffers();

    // TODO: временный тестовый код
    const u32 VertCount = 4;
    Vertex3D verts[VertCount] {};

    verts[0].x = 0.0;
    verts[0].y = -0.5;

    verts[1].x = 0.5;
    verts[1].y = 0.5;

    verts[2].x = 0;
    verts[2].y = 0.5;

    verts[3].x = 0.5;
    verts[3].y = -0.5;

    const u32 IndexCount = 6;
    u32 indices[IndexCount] = {0, 1, 2, 0, 3, 1};

    UploadDataRange(Device->GraphicsCommandPool, 0, Device->GraphicsQueue, *ObjectVertexBuffer, 0, sizeof(Vertex3D) * VertCount, verts);
    UploadDataRange(Device->GraphicsCommandPool, 0, Device->GraphicsQueue, *ObjectIndexBuffer, 0, sizeof(u32) * IndexCount, indices);
    // TODO: конец временного кода

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
    // Проверьте, не воссоздается ли цепочка подкачки заново, и загрузитесь.
    if (RecreatingSwapchain) {
        VkResult result = vkDeviceWaitIdle(Device->LogicalDevice);
        if (!VulkanResultIsSuccess(result)) {
            MERROR("Ошибка VulkanBeginFrame vkDeviceWaitIdle (1): '%s'", VulkanResultString(result, true));
            return false;
        }
        MINFO("Воссоздание цепочки подкачки, загрузка.");
        return false;
    }

    // Проверьте, был ли изменен размер фреймбуфера. Если это так, необходимо создать новую цепочку подкачки.
    if (FramebufferSizeGeneration != FramebufferSizeLastGeneration) {
        VkResult result = vkDeviceWaitIdle(Device->LogicalDevice);
        if (!VulkanResultIsSuccess(result)) {
            MERROR("Ошибка VulkanBeginFrame vkDeviceWaitIdle (2): '%s'", VulkanResultString(result, true));
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
    if (!VulkanFenceWait(
            this,
            &InFlightFences[CurrentFrame],
            UINT64_MAX)) {
        MWARN("Ошибка ожидания in-flight fence!");
        return false;
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

    MainRenderpass.w = FramebufferWidth;
    MainRenderpass.h = FramebufferHeight;

    // Начните этап рендеринга.
    VulkanRenderpassBegin(
        &GraphicsCommandBuffers[ImageIndex],
        &MainRenderpass,
        swapchain.framebuffers[ImageIndex].handle
    );

    // TODO: Временный тестовый код
    ObjectShader->Use(this);

    // Привязка буфера вершин к смещению.
    VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(GraphicsCommandBuffers[ImageIndex].handle, 0, 1, &ObjectVertexBuffer->handle, (VkDeviceSize*)offsets);

    // Привязка индексного буфер по смещению.
    vkCmdBindIndexBuffer(GraphicsCommandBuffers[ImageIndex].handle, ObjectIndexBuffer->handle, 0, VK_INDEX_TYPE_UINT32);

    // Отрисовка.
    vkCmdDrawIndexed(GraphicsCommandBuffers[ImageIndex].handle, 3, 1, 0, 0, 0);
    // TODO: конец ыременного тестового кода

    return true;
}

bool VulkanAPI::EndFrame(f32 DeltaTime)
{
    // Конечный проход рендеринга
    VulkanRenderpassEnd(&GraphicsCommandBuffers[ImageIndex], &MainRenderpass);

    VulkanCommandBufferEnd(&GraphicsCommandBuffers[ImageIndex]);

    // Убедитесь, что предыдущий кадр не использует это изображение (т.е. его ограждение находится в режиме ожидания).
    if (ImagesInFlight[ImageIndex] != VK_NULL_HANDLE) {  // был кадр
        VulkanFenceWait(
            this,
            ImagesInFlight[ImageIndex],
            UINT64_MAX);
    }

    // Отметьте ограждение изображения как используемое этим кадром.
    ImagesInFlight[ImageIndex] = &InFlightFences[CurrentFrame];

    // Сбросьте ограждение для использования на следующем кадре
    VulkanFenceReset(this, &InFlightFences[CurrentFrame]);

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
        Device->GraphicsQueue,
        1,
        &SubmitInfo,
        InFlightFences[CurrentFrame].handle);
    if (result != VK_SUCCESS) {
        MERROR("vkQueueSubmit не дал результата: %s", VulkanResultString(result, true));
        return false;
    }

    VulkanCommandBufferUpdateSubmitted(&GraphicsCommandBuffers[ImageIndex]);
    // Отправка в конечную очередь

    // Верните изображение обратно в swapchain.
    VulkanSwapchainPresent(
        this,
        &swapchain,
        Device->GraphicsQueue,
        Device->PresentQueue,
        QueueCompleteSemaphores[CurrentFrame],
        ImageIndex
    );

    return true;
}

void *VulkanAPI::operator new(u64 size)
{
    return LinearAllocator::Allocate(size);
}

/*void VulkanAPI::operator delete(void *ptr)
{
    MMemory::Free(ptr,sizeof(VulkanAPI), MEMORY_TAG_RENDERER);
}*/

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
    vkGetPhysicalDeviceMemoryProperties(Device->PhysicalDevice, &MemoryProperties);

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
                Device->GraphicsCommandPool,
                &GraphicsCommandBuffers[i]);
        } 
        //MMemory::ZeroMemory(&GraphicsCommandBuffers[i], sizeof(VulkanCommandBuffer));
        VulkanCommandBufferAllocate(
            this,
            Device->GraphicsCommandPool,
            true,
            &this->GraphicsCommandBuffers[i]);
    }

    MDEBUG("Созданы командные буферы Vulkan.");
}

void VulkanAPI::RegenerateFramebuffers()
{
    for (u32 i = 0; i < swapchain.ImageCount; ++i) {
        // TODO: сделать это динамическим на основе настроенных в данный момент вложений
        u32 AttachmentCount = 2;
        VkImageView attachments[] = {
            swapchain.views[i],
            swapchain.DepthAttachment.view};

        VulkanFramebufferCreate(
            this,
            &MainRenderpass,
            FramebufferWidth,
            FramebufferHeight,
            AttachmentCount,
            attachments,
            &swapchain.framebuffers[i]);
    }
}

bool VulkanAPI::CreateBuffers()
{
    VkMemoryPropertyFlagBits MemoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    const u64 VertexBufferSize = sizeof(Vertex3D) * 1024 * 1024;
    ObjectVertexBuffer = new VulkanBuffer();
    if (!ObjectVertexBuffer->Create(
            this,
            VertexBufferSize,
            static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
            MemoryPropertyFlags,
            true)) {
        MERROR("Ошибка создания вершинного буфера.");
        return false;
    }
    GeometryVertexOffset = 0;

    const u64 IndexBufferSize = sizeof(u32) * 1024 * 1024;
    ObjectIndexBuffer = new VulkanBuffer();
    if (!ObjectIndexBuffer->Create(
            this,
            IndexBufferSize,
            static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
            MemoryPropertyFlags,
            true)) {
        MERROR("Ошибка создания вершинного буфера.");
        return false;
    }
    GeometryVertexOffset = 0; 
    
    return true;
}

void VulkanAPI::UploadDataRange(VkCommandPool pool, VkFence fence, VkQueue queue, VulkanBuffer &buffer, u64 offset, u64 size, void *data)
{
    // Создание промежуточного буфера, видимого хосту, для загрузки. Отметьте его как источник передачи.
    VkBufferUsageFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VulkanBuffer staging;
    staging.Create(this, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, flags, true);

    // Загрузка данных в промежуточный буфер.
    staging.LoadData(this, 0, size, 0, data);

    // Копирование из промежуточного хранилища в локальный буфер устройства.
    staging.CopyTo(this, pool, fence, queue, 0, buffer.handle, offset, size);

    // Очистка промежуточного буфера.
    staging.Destroy(this);
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
    vkDeviceWaitIdle(Device->LogicalDevice);

    // Уберите это на всякий случай.
    for (u32 i = 0; i < swapchain.ImageCount; ++i) {
        ImagesInFlight[i] = nullptr;
    }

    // Поддержка запроса
    Device->QuerySwapchainSupport(
        Device->PhysicalDevice,
        surface,
        &Device->SwapchainSupport);
    Device->DetectDepthFormat(Device);

    VulkanSwapchainRecreate(
        this,
        CachedFramebufferWidth,
        CachedFramebufferHeight,
        &swapchain);

    // Синхронизируйте размер фреймбуфера с кэшированными размерами.
    FramebufferWidth = CachedFramebufferWidth;
    FramebufferHeight = CachedFramebufferHeight;
    MainRenderpass.w = FramebufferWidth;
    MainRenderpass.h = FramebufferHeight;
    CachedFramebufferWidth = 0;
    CachedFramebufferHeight = 0;

    // Обновить генерацию размера кадрового буфера.
    FramebufferSizeLastGeneration = FramebufferSizeGeneration;

    // Очистка цепочки подкачки
    for (u32 i = 0; i < swapchain.ImageCount; ++i) {
        VulkanCommandBufferFree(this, Device->GraphicsCommandPool, &GraphicsCommandBuffers[i]);
    }

    // Буферы кадров.
    for (u32 i = 0; i < swapchain.ImageCount; ++i) {
        VulkanFramebufferDestroy(this, &swapchain.framebuffers[i]);
    }

    MainRenderpass.x = 0;
    MainRenderpass.y = 0;
    MainRenderpass.w = FramebufferWidth;
    MainRenderpass.h = FramebufferHeight;

    RegenerateFramebuffers();

    CreateCommandBuffers();

    // Снимите флаг воссоздания.
    RecreatingSwapchain = false;

    return true;
}
