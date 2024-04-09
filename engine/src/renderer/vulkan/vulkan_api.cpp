#include "vulkan_api.hpp"
#include "vulkan_swapchain.hpp"
#include "core/application.hpp"
#include "vulkan_platform.hpp"
#include "systems/material_system.hpp"
#include "resources/geometry.hpp"
#include "renderer/vulkan/shaders/vulkan_ui_shader.hpp"

#include "math/vertex.hpp"
//#include "math/matrix4d.hpp"

#include "vulkan_utils.hpp"

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

void VulkanAPI::DrawGeometry(const GeometryRenderData& data)
{
    // Игнорировать незагруженные геометрии.
    if (data.gid && data.gid->InternalID == INVALID_ID) {
        return;
    }

    Geometry* BufferData = &this->geometries[data.gid->InternalID];
    VulkanCommandBuffer& CommandBuffer = this->GraphicsCommandBuffers[this->ImageIndex];

    // TODO: проверьте, действительно ли это необходимо.
    MaterialShader.Use(this);

    MaterialShader.SetModel(this, data.model);

    Material* m = nullptr;
    if (data.gid->material) {
        m = data.gid->material;
    } else {
        m = MaterialSystem::Instance()->GetDefaultMaterial();
    }
    MaterialShader.ApplyMaterial(this, m);
    
    // TODO: Временный тестовый код
    MaterialShader.Use(this);

    // Привязка буфера вершин к смещению.
    VkDeviceSize offsets[1] = {BufferData->VertexBufferOffset};
    vkCmdBindVertexBuffers(CommandBuffer.handle, 0, 1, &ObjectVertexBuffer.handle, static_cast<VkDeviceSize*>(offsets));

    // Рисовать индексированные или неиндексированные.
    if (BufferData->IndexCount > 0) {
        // Привязать индексный буфер по смещению.
        vkCmdBindIndexBuffer(CommandBuffer.handle, this->ObjectIndexBuffer.handle, BufferData->IndexBufferOffset, VK_INDEX_TYPE_UINT32);

        // Issue the draw.
        vkCmdDrawIndexed(CommandBuffer.handle, BufferData->IndexCount, 1, 0, 0, 0);
    } else {
        vkCmdDraw(CommandBuffer.handle, BufferData->VertexCount, 1, 0, 0);
    }
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
    MainRenderpass = VulkanRenderPass(
        this,
        Vector4D<f32>(0, 0, this->FramebufferWidth, this->FramebufferHeight),
        Vector4D<f32>(0.0f, 0.0f, 0.2f, 1.0f),  // Темносиний цвет
        1.0f,
        0,
        static_cast<u8>(RenderpassClearFlag::ColourBufferFlag | RenderpassClearFlag::DepthBufferFlag | RenderpassClearFlag::StencilBufferFlag),
        false, true
    );

    //UI renderpass
    UI_Renderpass = VulkanRenderPass(
        this,
        Vector4D<f32>(0, 0, this->FramebufferWidth, this->FramebufferHeight),
        Vector4D<f32>(0.0f, 0.0f, 0.0f, 0.0f),
        1.0f,
        0,
        static_cast<u8>(RenderpassClearFlag::NoneFlag),
        true, false
    );

    // Буферы кадров цепочки подкачки.
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
        geometries[i].id = INVALID_ID;
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
            MERROR("Ошибка VulkanBeginFrame vkDeviceWaitIdle (1): '%s'", VulkanResultString(result, true));
            return false;
        }
        MINFO("Воссоздание цепочки подкачки, загрузка.");
        return false;
    }

    // Проверьте, был ли изменен размер фреймбуфера. Если это так, необходимо создать новую цепочку подкачки.
    if (FramebufferSizeGeneration != FramebufferSizeLastGeneration) {
        VkResult result = vkDeviceWaitIdle(Device.LogicalDevice);
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
    VkResult result = vkWaitForFences(Device.LogicalDevice, 1, &InFlightFences[CurrentFrame], true, UINT64_MAX);
    if (!VulkanResultIsSuccess(result)) {
        MERROR("Ошибка ожидания в полете! ошибка: %s", VulkanResultString(result, true));
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

    MainRenderpass.RenderArea.z = FramebufferWidth;
    MainRenderpass.RenderArea.w = FramebufferHeight;

    // Начните этап рендеринга.
    MainRenderpass.Begin(
        &GraphicsCommandBuffers[ImageIndex],
        swapchain.framebuffers[ImageIndex]
    );

    return true;
}

    void VulkanAPI::UpdateGlobalWorldState(const Matrix4D& projection, const Matrix4D& view, const Vector3D<f32>& ViewPosition, const Vector4D<f32>& AmbientColour, i32 mode)
{
    //VulkanCommandBuffer* CommandBuffer = &GraphicsCommandBuffers[ImageIndex];

    MaterialShader.Use(this);

    MaterialShader.GlobalUObj.projection = projection;
    MaterialShader.GlobalUObj.view = view;

    // TODO: другие свойства ubo

    MaterialShader.UpdateGlobalState(this, FrameDeltaTime);
}

void VulkanAPI::UpdateGlobalUIState(const Matrix4D &projection, const Matrix4D &view, i32 mode)
{
    VulkanCommandBuffer& CommandBuffer = GraphicsCommandBuffers[ImageIndex];

    UI_Shader.Use(this);

    UI_Shader.GlobalUbo.projection = projection;
    UI_Shader.GlobalUbo.view = view;

    // TODO: other ubo properties

    UI_Shader.UpdateGlobalState(this, FrameDeltaTime);
}

bool VulkanAPI::EndFrame(f32 DeltaTime)
{
    // Конечный проход рендеринга
    MainRenderpass.End(&GraphicsCommandBuffers[ImageIndex]);

    VulkanCommandBufferEnd(&GraphicsCommandBuffers[ImageIndex]);

    // Убедитесь, что предыдущий кадр не использует это изображение (т.е. его ограждение находится в режиме ожидания).
    VkResult result = vkWaitForFences(Device.LogicalDevice, 1, ImagesInFlight[ImageIndex], true, UINT64_MAX);
        if (!VulkanResultIsSuccess(result)) {
            MFATAL("vkWaitForFences ошибка: %s", VulkanResultString(result, true));
        }

    // Отметьте ограждение изображения как используемое этим кадром.
    ImagesInFlight[ImageIndex] = &InFlightFences[CurrentFrame];

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

    /*VkResult */result = vkQueueSubmit(
        Device.GraphicsQueue,
        1,
        &SubmitInfo,
        InFlightFences[CurrentFrame]);
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
        Device.GraphicsQueue,
        Device.PresentQueue,
        QueueCompleteSemaphores[CurrentFrame],
        ImageIndex
    );

    return true;
}

bool VulkanAPI::BeginRenderpass(u8 RenderpassID)
{
    VulkanRenderPass* renderpass = nullptr;
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
    VulkanRenderPass* renderpass = nullptr;
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

bool VulkanAPI::CreateMaterial(Material *material)
{
    if (material) {
        if (!MaterialShader.AcquireResources(this, material)) {
            MERROR("VulkanRenderer::CreateMaterial — не удалось получить ресурсы шейдера.");
            return false;
        }

        MTRACE("Средство визуализации: материал создан.");
        return true;
    }

    MERROR("VulkanRenderer::CreateMaterial вызывается с nullptr. Создание не удалось.");
    return false;
}

void VulkanAPI::DestroyMaterial(Material *material)
{
    if (material) {
        if (material->InternalId != INVALID_ID) {
            MaterialShader.ReleaseResources(this, material);
        } else {
            MWARN("VulkanRenderer::DestroyMaterial вызывается с InternalId = INVALID_ID. Ничего не было сделано.");
        }
    } else {
        MWARN("VulkanRenderer::DestroyMaterial вызывается с nullptr. Ничего не было сделано.");
    }
}

bool VulkanAPI::Load(GeometryID *gid, u32 VertexCount, const Vertex3D *vertices, u32 IndexCount, const u32 *indices)
{
    // Проверьте, не повторная ли это загрузка. Если это так, необходимо впоследствии освободить старые данные.
    bool IsReupload = gid->InternalID != INVALID_ID;
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
            if (this->geometries[i].id == INVALID_ID) {
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
    geometry->SetVertexData(VertexCount, this->GeometryVertexOffset);

    UploadDataRange(pool, 0, queue, this->ObjectVertexBuffer, geometry->VertexBufferOffset, geometry->VertexSize, vertices);
    // TODO: вместо этого следует поддерживать список свободной памяти
    this->GeometryVertexOffset += geometry->VertexSize;

    // Данные индексов, если применимо
    if (IndexCount && indices) {
        geometry->SetIndexData(IndexCount, this->GeometryIndexOffset);
        UploadDataRange(pool, 0, queue, this->ObjectIndexBuffer, geometry->IndexBufferOffset, geometry->IndexSize, indices);
        // TODO: вместо этого следует поддерживать список свободной памяти
        this->GeometryIndexOffset += geometry->IndexSize;
    }

    if (gid->generation == INVALID_ID) {
        gid->generation = 0;
    } else {
        gid->generation++;
    }

    if (IsReupload) {
        // Освобождение данных вершин
        FreeDataRange(&this->ObjectVertexBuffer, OldRange.VertexBufferOffset, OldRange.VertexSize);

        // Освобождение данных индексов, если применимо
        if (OldRange.IndexSize > 0) {
            FreeDataRange(&this->ObjectIndexBuffer, OldRange.IndexBufferOffset, OldRange.IndexSize);
        }
    }

    return true;
}

void VulkanAPI::Unload(GeometryID *gid)
{
    if (gid && gid->InternalID != INVALID_ID) {
        vkDeviceWaitIdle(this->Device.LogicalDevice);
        Geometry& vGeometry = this->geometries[gid->InternalID];

        // Освобождение данных вершин
        FreeDataRange(&this->ObjectVertexBuffer, vGeometry.VertexBufferOffset, vGeometry.VertexSize);

        // Освобождение данных индексов, если это применимо
        if (vGeometry.IndexSize > 0) {
            FreeDataRange(&this->ObjectIndexBuffer, vGeometry.IndexBufferOffset, vGeometry.IndexSize);
        }

        // Очистка данных.
        vGeometry.Destroy(); //MMemory::ZeroMem(&this->geometries[geometry->InternalID], sizeof(Geometry));
        //geometry->id = INVALID_ID;
        //geometry->generation = INVALID_ID;
    }
}

void *VulkanAPI::operator new(u64 size)
{
    return LinearAllocator::Instance().Allocate(size);
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
    GeometryVertexOffset = 0;

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
    GeometryVertexOffset = 0; 
    
    return true;
}

void VulkanAPI::UploadDataRange(VkCommandPool pool, VkFence fence, VkQueue queue, VulkanBuffer &buffer, u64 offset, u64 size, const void *data)
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

void VulkanAPI::FreeDataRange(VulkanBuffer *buffer, u64 offset, u64 size)
{
    // TODO: Освободить это в буфере.
    // TODO: обновить список свободной памяти, чтобы этот диапазон был свободным.
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
