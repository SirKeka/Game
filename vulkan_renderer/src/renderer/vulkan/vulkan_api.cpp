#include "vulkan_api.hpp"
#include "vulkan_command_buffer.hpp"
#include "vulkan_swapchain.hpp"
#include "vulkan_image.hpp"
#include "vulkan_buffer.hpp"
#include "platform\vulkan_platform.hpp"
#include "vulkan_renderpass.hpp"

#include "renderer/renderbuffer.hpp"

#include <systems/material_system.h>
#include <systems/texture_system.h>
#include <systems/resource_system.h>
#include <core/frame_data.h>

#include <math/vertex.hpp>

#include "vulkan_utils.hpp"

// ПРИМЕЧАНИЕ: Если вы хотите отслеживать выделения, раскомментируйте это.
// #ifndef MVULKAN_ALLOCATOR_TRACE
// #define MVULKAN_ALLOCATOR_TRACE 1
// #endif

// ПРИМЕЧАНИЕ: Чтобы отключить пользовательский распределитель, закомментируйте это или установите значение 0.
#ifndef MVULKAN_USE_CUSTOM_ALLOCATOR
#define MVULKAN_USE_CUSTOM_ALLOCATOR 1
#endif

VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT MessageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
    void* UserData
);

#if MVULKAN_USE_CUSTOM_ALLOCATOR == 1
/// @brief Реализация PFN_vkAllocationFunction.
/// @link https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/PFN_vkAllocationFunction.html
///
/// @param UserData пользовательские данные, указанные в распределителе приложением.
/// @param size размер запрошенного выделения в байтах.
/// @param alignment запрошенное выравнивание выделения в байтах. Должна быть степенью двойки.
/// @param allocationScope область выделения и время жизни.
/// @return Блок памяти в случае успеха; в противном случае nullptr.
void* VulkanAllocAllocation(void* UserData, size_t size, size_t alignment, VkSystemAllocationScope AllocationScope) {
    // Если это не удается, ОБЯЗАТЕЛЬНО должен быть возвращен null.
    if (size == 0) {
        return nullptr;
    }

    void* result = MemorySystem::AllocateAligned(size, (u16)alignment, Memory::Vulkan, true);
#ifdef MVULKAN_ALLOCATOR_TRACE
    MTRACE("Выделенный блок %p. Размер=%llu, Выравнивание=%llu", result, size, alignment);
#endif
    return result;
}

/// @brief Реализация PFN_vkFreeFunction.
/// @link https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/PFN_vkFreeFunction.html
///
/// @param UserData Данные пользователя, указанные в распределителе приложением.
/// @param memory Освобождаемое распределение.
void VulkanAllocFree(void* UserData, void* memory) {
    if (!memory) {
#ifdef MVULKAN_ALLOCATOR_TRACE
        MTRACE("Блок пустой, нечего освобождать: %p", memory);
#endif
        return;
    }

#ifdef MVULKAN_ALLOCATOR_TRACE
    MTRACE("Попытка освободить блок %p...", memory);
#endif
    u64 size;
    u16 alignment;
    bool result = MemorySystem::GetSizeAlignment(memory, size, alignment);
    if (result) {
#ifdef MVULKAN_ALLOCATOR_TRACE
        MTRACE("Найден блок %p с размером/выравниванием: %llu/%u. Освобождение выровненного блока...", memory, size, alignment);
#endif
        MemorySystem::FreeAligned(memory, size, alignment, Memory::Vulkan);
    } else {
        MERROR("VulkanAllocFree не удалось получить поиск выравнивания для блока %p.", memory);
    }
}

/// @brief Реализация PFN_vkReallocationFunction.
/// @link https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/PFN_vkReallocationFunction.html
///
/// @param UserData Пользовательские данные, указанные в распределителе приложением.
/// @param original Либо NULL, либо указатель, ранее возвращенный VulkanAllocAllocation.
/// @param size Размер запрошенного выделения в байтах.
/// @param alignment Запрошенное выравнивание выделения в байтах. Должна быть степенью двойки.
/// @param allocation_scope Область действия и время жизни выделения.
/// @return Блок памяти в случае успеха; в противном случае nullptr.
void* VulkanAllocReallocation(void* UserData, void* original, size_t size, size_t alignment, VkSystemAllocationScope AllocationScope) {
    if (!original) {
        return VulkanAllocAllocation(UserData, size, alignment, AllocationScope);
    }

    if (size == 0) {
        return nullptr;
    }

    // ПРИМЕЧАНИЕ: если pOriginal не равен нулю, для нового выделения должно использоваться то же выравнивание, что и для исходного.
    u64 AllocSize;
    u16 AllocAlignment;
    bool IsAligned = MemorySystem::GetSizeAlignment(original, AllocSize, AllocAlignment);
    if (!IsAligned) {
        MERROR("VulkanAllocReallocation невыровненного блока %p", original);
        return nullptr;
    }

    if (AllocAlignment != alignment) {
        MERROR("Попытка перераспределения с использованием выравнивания %llu, отличного от исходного %hu.", alignment, AllocAlignment);
        return nullptr;
    }

#ifdef MVULKAN_ALLOCATOR_TRACE
    MTRACE("Попытка перераспределить блок %p...", original);
#endif

    void* result = VulkanAllocAllocation(UserData, size, AllocAlignment, AllocationScope);
    if (result) {
#ifdef MVULKAN_ALLOCATOR_TRACE
        MTRACE("Блок %p перераспределен в %p, копирование данных...", original, result);
#endif

        // Копирование поверх исходной памяти.
        MemorySystem::CopyMem(result, original, size);
#ifdef MVULKAN_ALLOCATOR_TRACE
        MTRACE("Освобождение исходного выровненного блока %p...", original);
#endif
        // Освобождение исходной памяти только в случае успешного нового выделения.
        MemorySystem::FreeAligned(original, AllocSize, true, Memory::Vulkan);
    } else {
#ifdef MVULKAN_ALLOCATOR_TRACE
        MERROR("Не удалось перераспределить %p.", original);
#endif
    }

    return result;
}

/// @brief Реализация PFN_vkInternalAllocationNotification. Чисто информационная, с ней ничего нельзя сделать, кроме как отслеживать.
/// @link https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/PFN_vkInternalAllocationNotification.html
///
/// @param pUserData Пользовательские данные, указанные в распределителе приложением.
/// @param size Размер выделения в байтах.
/// @param allocationType Тип внутреннего выделения.
/// @param allocationScope Область действия и время жизни выделения.
void VulkanAllocInternalAlloc(void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope) {
#ifdef MVULKAN_ALLOCATOR_TRACE
    MTRACE("Внешнее распределение размера: %llu", size);
#endif
    MemorySystem::AllocateReport((u64)size, Memory::VulkanEXT);
}

/// @brief Реализация PFN_vkInternalFreeNotification. Чисто информационная, с ней ничего нельзя сделать, кроме как отслеживать.
/// @link https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/PFN_vkInternalFreeNotification.html
///
/// @param pUserData пользовательские данные, указанные в распределителе приложением.
/// @param size размер освобождаемого выделения в байтах.
/// @param allocationType тип внутреннего выделения.
/// @param allocationScope область действия и время жизни выделения.
void VulkanAllocInternalFree(void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope) {
#ifdef MVULKAN_ALLOCATOR_TRACE
    MTRACE("Внешний свободный размер: %llu", size);
#endif
    MemorySystem::FreeReport((u64)size, Memory::VulkanEXT);
}

/// @brief Создайте объект распределителя Vulkan, заполнив указатели функций в предоставленной структуре.
///
/// @param callbacks Указатель на структуру обратных вызовов распределения, которую необходимо заполнить.
/// @return True в случае успеха; в противном случае false.
bool VulkanAPI::CreateVulkanAllocator(VkAllocationCallbacks* callbacks) {
    if (callbacks) {
        callbacks->pfnAllocation = VulkanAllocAllocation;
        callbacks->pfnReallocation = VulkanAllocReallocation;
        callbacks->pfnFree = VulkanAllocFree;
        callbacks->pfnInternalAllocation = VulkanAllocInternalAlloc;
        callbacks->pfnInternalFree = VulkanAllocInternalFree;
        callbacks->pUserData = this;
        return true;
    }

    return false;
}
#endif // MVULKAN_USE_CUSTOM_ALLOCATOR == 1

VulkanAPI::VulkanAPI()
: FrameDeltaTime(),
// Просто установите некоторые значения по умолчанию для буфера кадра на данный момент.
// На самом деле неважно, что это, потому что они будут переопределены, но они необходимы для создания цепочки обмена.
FramebufferWidth(800), FramebufferHeight(600), 
FramebufferSizeGeneration(),
FramebufferSizeLastGeneration(),
ViewportRect(), // 0.F, (f32)FramebufferHeight, (f32)FramebufferWidth, -(f32)FramebufferHeight
ScissorRect(),  // 0, 0, FramebufferHeight, FramebufferHeight
instance(), allocator(nullptr), surface(),

#if defined(_DEBUG)
DebugMessenger(),
#endif

Device(), swapchain(),
ObjectVertexBuffer("renderbuffer_vertexbuffer_globalgeometry", RenderBufferType::Vertex, sizeof(Vertex3D) * 1024 * 1024, true),
ObjectIndexBuffer("renderbuffer_indexbuffer_globalgeometry", RenderBufferType::Index, sizeof(u32) * 1024 * 1024, true),
GraphicsCommandBuffers(),
ImageAvailableSemaphores(),
QueueCompleteSemaphores(),
InFlightFenceCount(),
InFlightFences(),
ImagesInFlight(),
ImageIndex(),
CurrentFrame(),
RecreatingSwapchain(false),
RenderFlagChanged(false),
geometries(),
WorldRenderTargets(),
MultithreadingEnabled(false)
{

}

VulkanAPI::~VulkanAPI()
{
    vkDeviceWaitIdle(Device.LogicalDevice);

    // Уничтожать в порядке, обратном порядку создания.

    RenderBufferDestroyInternal(ObjectVertexBuffer);
    RenderBufferDestroyInternal(ObjectIndexBuffer);

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

bool VulkanAPI::Initialize(const RenderingConfig &config, u8 &OutWindowRenderTargetCount)
{
    // ПРИМЕЧАНИЕ: Пользовательский распределитель.
#if MVULKAN_USE_CUSTOM_ALLOCATOR == 1
    allocator = MemorySystem::TAllocate<VkAllocationCallbacks>(Memory::Renderer);
    if (!CreateVulkanAllocator(allocator)) {
        // Если это не удается, аккуратно вернитесь к распределителю по умолчанию.
        MFATAL("Не удалось создать пользовательский распределитель Vulkan. Продолжаем использовать распределитель драйвера по умолчанию.");
        MemorySystem::Free(allocator, sizeof(VkAllocationCallbacks), Memory::Renderer);
        allocator = nullptr;
        }
#else
    allocator = nullptr;
#endif

    // Общая структура информации о приложении.
    VkApplicationInfo AppInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    AppInfo.apiVersion = VK_API_VERSION_1_2;
    AppInfo.pApplicationName = config.ApplicationName;
    AppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0); // VK_MAKE_API_VERSION(0, 1, 0, 0);
    AppInfo.pEngineName = "Moon Engine";
    AppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0); // VK_MAKE_API_VERSION(0, 1, 0, 0); 
    // Создание экземпляра.
    VkInstanceCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    CreateInfo.pApplicationInfo = &AppInfo;

    // Получите список необходимых расширений
    
    DArray<const char*>RequiredExtensions;                                  // Общее расширение поверхности
    RequiredExtensions.PushBack(VK_KHR_SURFACE_EXTENSION_NAME);
    PlatformGetRequiredExtensionNames(RequiredExtensions);                  // Расширения для конкретной платформы
#if defined(_DEBUG)
    RequiredExtensions.PushBack(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);         // утилиты отладки

    MDEBUG("Необходимые расширения:");
    const u32& RequiredExtensionCount = RequiredExtensions.Length();
    for (u32 i = 0; i < RequiredExtensionCount; ++i) {
        MDEBUG(RequiredExtensions[i]);
    }
#endif

    CreateInfo.enabledExtensionCount = RequiredExtensions.Length();
    CreateInfo.ppEnabledExtensionNames = RequiredExtensions.Data(); //ЗАДАЧА: указателю ppEnabledExtensionNames присваевается адрес указателя массива после выхода из функции данные стираются

    u32 AvailableExtensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &AvailableExtensionCount, nullptr);
    DArray<VkExtensionProperties> AvailableExtensions;
    AvailableExtensions.Resize(AvailableExtensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &AvailableExtensionCount, AvailableExtensions.Data());

    // Проверьте доступность необходимых расширений.
    for (u32 i = 0; i < RequiredExtensionCount; ++i) {
        bool found = false;
        for (u32 j = 0; j < AvailableExtensionCount; ++j) {
            if (MString::Equali(RequiredExtensions[i], AvailableExtensions[j].extensionName)) {
                found = true;
                MINFO("Требуемое расширение найдено: %s...", RequiredExtensions[i]);
                break;
            }
        }

        if (!found) {
            MFATAL("Отсутствует требуемое расширение: %s", RequiredExtensions[i]);
            return false;
        }
    }

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
    VK_CHECK(vkEnumerateInstanceLayerProperties(&AvailableLayerCount, nullptr));
    DArray<VkLayerProperties> AvailableLayers;
    AvailableLayers.Resize(AvailableLayerCount);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&AvailableLayerCount, AvailableLayers.Data())); 

    // Убедитесь, что доступны все необходимые слои.
    for (u32 i = 0; i < RequiredValidationLayerCount; ++i) {
        bool found = false;
        for (u32 j = 0; j < AvailableLayerCount; ++j) {
            if (MString::Equal(RequiredValidationLayerNames[i], AvailableLayers[j].layerName)) {
                found = true;
                MINFO("Найден слой проверки: %s...", RequiredValidationLayerNames[i]);
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
    CreateInfo.ppEnabledLayerNames = RequiredValidationLayerNames.Data();

    VkResult InstanceResult = vkCreateInstance(&CreateInfo, allocator, &instance);
    if (!VulkanResultIsSuccess(InstanceResult)) {
        const char* ResultString = VulkanResultString(InstanceResult, true);
        MFATAL("Создание экземпляра Vulkan не удалось, результат: '%s'", ResultString);
        return false;
    }
    MINFO("Создан экземпляр Vulkan.");

    // ЗАДАЧА: реализовать многопоточность.

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

    // Загрузите указатели отладочных функций.
    pfnSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
    if (!pfnSetDebugUtilsObjectNameEXT) {
        MWARN("Невозможно загрузить указатель функции для vkSetDebugUtilsObjectNameEXT. Отладочные функции, связанные с этим, не будут работать.");
    }
    pfnSetDebugUtilsObjectTagEXT = (PFN_vkSetDebugUtilsObjectTagEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectTagEXT");
    if (!pfnSetDebugUtilsObjectTagEXT) {
        MWARN("Невозможно загрузить указатель функции для vkSetDebugUtilsObjectTagEXT. Отладочные функции, связанные с этим, не будут работать.");
    }
    
    pfnCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT");
    if (!pfnCmdBeginDebugUtilsLabelEXT) {
        MWARN("Невозможно загрузить указатель функции для vkCmdBeginDebugUtilsLabelEXT. Отладочные функции, связанные с этим, не будут работать.");
    }
    
    pfnCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT");
    if (!pfnCmdEndDebugUtilsLabelEXT) {
        MWARN("Невозможно загрузить указатель функции для vkCmdEndDebugUtilsLabelEXT. Отладочные функции, связанные с этим, не будут работать.");
    }
#endif

    // Поверхность
    MDEBUG("Создание Vulkan поверхности...");
    if (!PlatformCreateVulkanSurface(this)) {
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
    swapchain.Create(this, FramebufferWidth, FramebufferHeight, config.flags);

    // Сохраните количество имеющихся у нас изображений в качестве необходимого количества целей рендеринга.
    OutWindowRenderTargetCount = swapchain.ImageCount;

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

    // Создать буферы

    // Буфер вершин геометрии
    if (!RenderBufferCreateInternal(ObjectVertexBuffer)) {
        MERROR("Ошибка создания буфера вершин.");
        return false;
    }
    RenderBufferBind(ObjectVertexBuffer, 0);
    // Буфер индексов геометрии
    if (!RenderBufferCreateInternal(ObjectIndexBuffer)) {
        MERROR("Ошибка создания буфера индексов.");
        return false;
    }
    RenderBufferBind(ObjectIndexBuffer, 0);
   
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
    FramebufferWidth = width;
    FramebufferHeight = height;
    FramebufferSizeGeneration++;

    MINFO("API рендеринга Vulkan-> изменен размер: w/h/gen: %i/%i/%llu", width, height, FramebufferSizeGeneration);
}

bool VulkanAPI::BeginFrame(const FrameData& rFrameData)
{
    FrameDeltaTime = rFrameData.DeltaTime;

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
    // Также включите проверку изменения вертикальной синхронизации.
    if (FramebufferSizeGeneration != FramebufferSizeLastGeneration || RenderFlagChanged) {
        VkResult result = vkDeviceWaitIdle(Device.LogicalDevice);
        if (!VulkanResultIsSuccess(result)) {
            MERROR("Ошибка VulkanBeginFrame vkDeviceWaitIdle (2): '%shader'", VulkanResultString(result, true));
            return false;
        }

        if (RenderFlagChanged) {
            RenderFlagChanged = false;
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
    ViewportRect = FVec4(0.F, (f32)FramebufferHeight, (f32)FramebufferWidth, -(f32)FramebufferHeight);
    ViewportSet(ViewportRect);

    ScissorRect = FVec4(0.F, 0.F, FramebufferWidth, FramebufferHeight);
    ScissorSet(ScissorRect);

    return true;
}

bool VulkanAPI::EndFrame(const FrameData& rFrameData)
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
        Device.PresentQueue,
        QueueCompleteSemaphores[CurrentFrame],
        ImageIndex
    );

    return true;
}

void VulkanAPI::ViewportSet(const FVec4 &rect)
{
    VkViewport viewport;
    viewport.x = rect.x;
    viewport.y = rect.y;
    viewport.width = rect.z;
    viewport.height = rect.w;
    viewport.minDepth = 0.F;
    viewport.maxDepth = 1.F;

    auto& CommandBuffer = GraphicsCommandBuffers[ImageIndex];

    vkCmdSetViewport(CommandBuffer.handle, 0, 1, &viewport);
}

void VulkanAPI::ViewportReset()
{
    // Просто установите текущий прямоугольник области просмотра.
    ViewportSet(ViewportRect);
}

void VulkanAPI::ScissorSet(const FVec4 &rect)
{
    VkRect2D scissor;
    scissor.offset.x = rect.x;
    scissor.offset.y = rect.y;
    scissor.extent.width = rect.z;
    scissor.extent.height = rect.w;

    auto CommandBuffer = GraphicsCommandBuffers[ImageIndex];

    vkCmdSetScissor(CommandBuffer.handle, 0, 1, &scissor);
}

void VulkanAPI::ScissorReset()
{
    // Просто установите текущий прямоугольник ножниц.
    ScissorSet(ScissorRect);
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
        MemorySystem::CopyMem(ClearValues[BeginInfo.clearValueCount].color.float32, pass->ClearColour.elements, sizeof(f32) * 4);
        BeginInfo.clearValueCount++;
    } else {
        // Все равно добавьте его, но не беспокойтесь о копировании данных, так как они будут проигнорированы.
        BeginInfo.clearValueCount++;
    }

    bool DoClearDepth = (pass->ClearFlags & RenderpassClearFlag::DepthBuffer) != 0;
    if (DoClearDepth) {
        MemorySystem::CopyMem(ClearValues[BeginInfo.clearValueCount].color.float32, pass->ClearColour.elements, sizeof(f32) * 4);
        ClearValues[BeginInfo.clearValueCount].depthStencil.depth = VkRenderpass->depth;

        bool DoClearStencil = (pass->ClearFlags & RenderpassClearFlag::StencilBuffer) != 0;
        ClearValues[BeginInfo.clearValueCount].depthStencil.stencil = DoClearStencil ? VkRenderpass->stencil : 0;
        BeginInfo.clearValueCount++;
    } else {
        for (u32 i = 0; i < target.AttachmentCount; ++i) {
            if (target.attachments[i].type == RenderTargetAttachmentType::Depth) {
                // Если есть привязка глубины, обязательно добавьте количество очисток, но не беспокойтесь о копировании данных.
                BeginInfo.clearValueCount++;
            }
        }
    }

    BeginInfo.pClearValues = BeginInfo.clearValueCount > 0 ? ClearValues : 0;

    vkCmdBeginRenderPass(CommandBuffer.handle, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    CommandBuffer.state = COMMAND_BUFFER_STATE_IN_RENDER_PASS;

    f32 r = Math::RandomInRange(0.F, 1.F);
    f32 g = Math::RandomInRange(0.F, 1.F);
    f32 b = Math::RandomInRange(0.F, 1.F);
    FVec4 colour{r, g, b, 1.F};
    VK_BEGIN_DEBUG_LABEL(this, CommandBuffer.handle, pass->name.c_str(), colour);

    return true;
}

bool VulkanAPI::RenderpassEnd(Renderpass* pass)
{
    auto& CommandBuffer = GraphicsCommandBuffers[ImageIndex];

    // Завершение рендеринга.
    vkCmdEndRenderPass(CommandBuffer.handle);
    VK_END_DEBUG_LABEL(this, CommandBuffer.handle);
    CommandBuffer.state = COMMAND_BUFFER_STATE_RECORDING;
    return true;
}

void VulkanAPI::Load(const u8* pixels, Texture *texture)
{
    // Создание внутренних данных.
    u32 ImageSize = texture->width * texture->height * texture->ChannelCount  * (texture->type == TextureType::Cube ? 6 : 1);

    // ПРИМЕЧАНИЕ: Предполагается, что на канал приходится 8 бит.
    VkFormat ImageFormat = VK_FORMAT_R8G8B8A8_UNORM;

    // ПРИМЕЧАНИЕ: Здесь много предположений, для разных типов текстур потребуются разные параметры.
    VulkanImage::Config config = { this };
    config.type            = texture->type;
    config.width           = texture->width;
    config.height          = texture->height;
    config.format          = ImageFormat;
    config.tiling          = VK_IMAGE_TILING_OPTIMAL;
    config.usage           = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    config.MemoryFlags     = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    config.CreateView      = true;
    config.ViewAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    config.name            = texture->name; 

    texture->data = new VulkanImage(config);

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
    VkImageUsageFlagBits usage;
    VkImageAspectFlagBits aspect;
    VkFormat ImageFormat;
    if (texture->flags & Texture::Flag::Depth) {
        usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
        ImageFormat = Device.DepthFormat;
    } else {
        usage = static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        ImageFormat = ChannelCountToFormat(texture->ChannelCount, VK_FORMAT_R8G8B8A8_UNORM);
    }

    VulkanImage::Config config = { this };
    config.type                = texture->type;
    config.width               = texture->width;
    config.height              = texture->height;
    config.format              = ImageFormat;
    config.tiling              = VK_IMAGE_TILING_OPTIMAL;
    config.usage               = usage;
    config.MemoryFlags         = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    config.CreateView          = true;
    config.ViewAspectFlags     = aspect;
    config.name                = texture->name;

    texture->data = new VulkanImage(config);
    texture->generation++;
}

void VulkanAPI::TextureResize(Texture *texture, u32 NewWidth, u32 NewHeight)
{
    if (texture && texture->data) {
        auto image = reinterpret_cast<VulkanImage*>(texture->data);
        // Изменение размера на самом деле просто разрушает старое изображение и создает новое. 
        // Данные не сохраняются, поскольку не существует надежного способа сопоставить старые 
        // данные с новыми, поскольку объем данных различается.
        image->Destroy(this);

        VkFormat ImageFormat = ChannelCountToFormat(texture->ChannelCount, VK_FORMAT_R8G8B8A8_UNORM);

        // ЗАДАЧА: здесь много предположений, разные типы текстур потребуют разных опций.
        VulkanImage::Config config = { this };
        config.type                = texture->type;
        config.width               = NewWidth;
        config.height              = NewHeight;
        config.format              = ImageFormat;
        config.tiling              = VK_IMAGE_TILING_OPTIMAL;
        config.usage               = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        config.MemoryFlags         = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        config.CreateView          = true;
        config.ViewAspectFlags     = VK_IMAGE_ASPECT_COLOR_BIT;
        config.name                = texture->name;

        image->Create(config);

        texture->generation++;
    }
}

void VulkanAPI::TextureWriteData(Texture *texture, u32 offset, u32 size, const u8 *pixels)
{
    VkFormat ImageFormat = ChannelCountToFormat(texture->ChannelCount, VK_FORMAT_R8G8B8A8_UNORM);

    // Создайте промежуточный буфер и загрузите в него данные.
    RenderBuffer staging { "renderbuffer_texture_write_staging", RenderBufferType::Staging, size, false };
    if (!RenderBufferCreateInternal(staging)) {
        MERROR("Не удалось создать промежуточный буфер для записи текстуры.");
        return;
    }
    RenderBufferBind(staging, 0);
    RenderBufferLoadRange(staging, 0 , size, pixels);

    VulkanCommandBuffer TempBuffer;
    const auto& pool = Device.GraphicsCommandPool;
    const auto& queue = Device.GraphicsQueue;
    VulkanCommandBufferAllocateAndBeginSingleUse(this, pool, TempBuffer);

    auto image = reinterpret_cast<VulkanImage*>(texture->data);
    // Переведите макет от текущего к оптимальному для получения данных.
    image->TransitionLayout(this, texture->type, TempBuffer, ImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Скопируйте данные из буфера.
    image->CopyFromBuffer(this, texture->type, reinterpret_cast<VulkanBuffer*>(staging.data)->handle, &TempBuffer);

    // Переход от оптимального для приема данных к оптимальному макету, доступному только для чтения шейдеров.
    image->TransitionLayout(
        this, 
        texture->type,
        TempBuffer, 
        ImageFormat, 
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    VulkanCommandBufferEndSingleUse(this, pool, TempBuffer, queue);
    
    RenderBufferUnbind(staging);
    RenderBufferDestroyInternal(staging);

    texture->generation++;
}

void VulkanAPI::TextureReadData(Texture *texture, u32 offset, u32 size, void **OutMemory)
{
    auto image = reinterpret_cast<VulkanImage*>(texture->data);

    VkFormat ImageFormat = ChannelCountToFormat(texture->ChannelCount, VK_FORMAT_R8G8B8A8_UNORM);

    // Создайте промежуточный буфер и загрузите в него данные.
    RenderBuffer staging;
    char bufname[] = "renderbuffer_texture_read_staging";
    if (!RenderBufferCreate(bufname, RenderBufferType::Read, size, false, staging)) {
        MERROR("Не удалось создать промежуточный буфер для чтения текстуры.");
        return;
    }
    RenderBufferBind(staging, 0);

    VulkanCommandBuffer TempBuffer;
    auto pool = Device.GraphicsCommandPool;
    auto queue = Device.GraphicsQueue;
    VulkanCommandBufferAllocateAndBeginSingleUse(this, pool, TempBuffer);

    // ПРИМЕЧАНИЕ: переход к VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    // Переведите макет из текущего состояния в оптимальное для выдачи данных.
    image->TransitionLayout(
        this,
        texture->type,
        TempBuffer,
        ImageFormat,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    // Копируем данные в буфер.
    image->CopyToBuffer(this, texture->type, reinterpret_cast<VulkanBuffer*>(staging.data)->handle, TempBuffer);

    // Переход от оптимальной для чтения данных к оптимальной для чтения только шейдера компоновке.
    image->TransitionLayout(
        this,
        texture->type,
        TempBuffer,
        ImageFormat,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VulkanCommandBufferEndSingleUse(this, pool, TempBuffer, queue);

    if (!RenderBufferRead(staging, offset, size, OutMemory)) {
        MERROR("RenderBufferRead failed.");
    }

    RenderBufferUnbind(staging);
    RenderBufferDestroyInternal(staging);
}

void VulkanAPI::TextureReadPixel(Texture *texture, u32 x, u32 y, u8 **OutRgba)
{
    auto image = reinterpret_cast<VulkanImage*>(texture->data);

    auto ImageFormat = ChannelCountToFormat(texture->ChannelCount, VK_FORMAT_R8G8B8A8_UNORM);

    // ЗАДАЧА: Создавать буфер каждый раз не очень хорошо. Можно оптимизировать это, создав буфер один раз и просто повторно используя его.

    // Создайте промежуточный буфер и загрузите в него данные.
    RenderBuffer staging;
    char bufname[] = "renderbuffer_texture_read_staging";
    if (!RenderBufferCreate(bufname, RenderBufferType::Read, sizeof(u8) * 4, false, staging)) {
        MERROR("Не удалось создать промежуточный буфер для чтения пикселей текстуры.");
        return;
    }
    RenderBufferBind(staging, 0);

    VulkanCommandBuffer TempBuffer;
    auto pool = Device.GraphicsCommandPool;
    auto queue = Device.GraphicsQueue;
    VulkanCommandBufferAllocateAndBeginSingleUse(this, pool, TempBuffer);

    // ПРИМЕЧАНИЕ: переход к VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    // Переведите макет из текущего состояния в оптимальное для передачи данных.
    image->TransitionLayout(
        this,
        texture->type,
        TempBuffer,
        ImageFormat,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    );

    // Скопируйте данные в буфер.
    // image.CopyToBuffer(this, texture->type, reinterpret_cast<VulkanBuffer*>(staging.data)->handle, TempBuffer);
    image->CopyPixelToBuffer(this, texture->type, reinterpret_cast<VulkanBuffer*>(staging.data)->handle, x, y, TempBuffer);

    // Переход от оптимальной компоновки для чтения данных к оптимальной компоновке, предназначенной только для чтения шейдеров.
    image->TransitionLayout(
        this,
        texture->type,
        TempBuffer,
        ImageFormat,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    VulkanCommandBufferEndSingleUse(this, pool, TempBuffer, queue);

    if (!RenderBufferRead(staging, 0, sizeof(u8) * 4, reinterpret_cast<void**>(OutRgba))) {
        MERROR("RenderBufferRead failed.");
    }

    RenderBufferUnbind(staging);
    RenderBufferDestroyInternal(staging);
}

void *VulkanAPI::TextureCopyData(const Texture* texture)
{
    auto data = reinterpret_cast<VulkanImage*>(texture->data);
    return (new VulkanImage(*data));
}

void VulkanAPI::Unload(Texture *texture)
{
    vkDeviceWaitIdle(Device.LogicalDevice);

    if (texture->data) {
        auto data = reinterpret_cast<VulkanImage*>(texture->data);
        data->Destroy(this);

        delete data;
        data = nullptr;
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

    // Данные вершин.
    geometry->SetVertexIndex(VertexCount, VertexSize, IndexCount);
    u32 TotalSize = VertexCount * VertexSize;
    
    if (!ObjectVertexBuffer.Allocate(TotalSize, geometry->VertexBufferOffset)) {
        MERROR("VulkanAPI::LoadDeometry не удалось выделить память из буфера вершин!");
        return false;
    }
    
    if (!RenderBufferLoadRange(ObjectVertexBuffer, geometry->VertexBufferOffset, TotalSize, vertices)) {
        MERROR("VulkanAPI::LoadGeometry не удалось загрузить в буфер вершин!");
        return false;
    }

    // Данные индексов, если применимо
    if (IndexCount && indices) {
        TotalSize = IndexCount * IndexSize;
        if (!ObjectIndexBuffer.Allocate(TotalSize, geometry->IndexBufferOffset)) {
            MERROR("VulkanAPI::LoadGeometry не удалось выделить память из буфера индексов!");
            return false;
        }
    
        if (!RenderBufferLoadRange(ObjectIndexBuffer, geometry->IndexBufferOffset, TotalSize, indices)) {
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
        ObjectVertexBuffer.Free(OldRange.VertexElementSize * OldRange.VertexCount, OldRange.VertexBufferOffset);

        // Освобождение данных индексов, если применимо
        if (OldRange.IndexElementSize > 0) {
            ObjectIndexBuffer.Free(OldRange.IndexElementSize * OldRange.IndexCount, OldRange.IndexBufferOffset);
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
        if (!ObjectVertexBuffer.Free(vGeometry.VertexElementSize * vGeometry.VertexCount, vGeometry.VertexBufferOffset)) {
            MERROR("VulkanAPI::UnloadGeometry не удалось освободить диапазон буфера вершин.");
        }

        // Освобождение данных индексов, если это применимо
        if (vGeometry.IndexElementSize > 0) {
            if (!ObjectIndexBuffer.Free(vGeometry.IndexElementSize * vGeometry.IndexCount, vGeometry.IndexBufferOffset)) {
                MERROR("VulkanAPI::UnloadGeometry не удалось освободить диапазон буфера индексов.");
            }
            
        }

        // Очистка данных.
        vGeometry.Destroy();
    }
}

void VulkanAPI::DrawGeometry(const GeometryRenderData& data)
{
    // Игнорировать незагруженные геометрии.
    if (data.gid && data.gid->InternalID == INVALID::ID) {
        return;
    }

    auto& BufferData = geometries[data.gid->InternalID];
    bool IncludesIndexData = BufferData.IndexCount > 0;
    if (!RenderBufferDraw(ObjectVertexBuffer, BufferData.VertexBufferOffset, BufferData.VertexCount, IncludesIndexData)) {
        MERROR("VulkanAPI::DrawGeometry не удалось отрисовать буфер вершин.");
        return;
    }

    if (IncludesIndexData) {
        if (!RenderBufferDraw(ObjectIndexBuffer, BufferData.IndexBufferOffset, BufferData.IndexCount, !IncludesIndexData)) {
            MERROR("VulkanAPI::DrawGeometry не удалось отрисовать буфер индексов.");
            return;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////
//                                  Shader                                        //
////////////////////////////////////////////////////////////////////////////////////

const u32 DESC_SET_INDEX_GLOBAL   = 0;  // Индекс набора глобальных дескрипторов.
const u32 DESC_SET_INDEX_INSTANCE = 1;  // Индекс набора дескрипторов экземпляра.

bool VulkanAPI::Load(Shader *shader, const Shader::Config& config, Renderpass* renderpass, const DArray<Shader::Stage>& stages, const DArray<MString>& StageFilenames)
{
    // Этапы перевода
    VkShaderStageFlags VkStages[VulkanShaderConstants::MaxStages];
    for (u8 i = 0; i < stages.Length(); ++i) {
        switch (stages[i]) {
            case Shader::Stage::Fragment:
                VkStages[i] = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            case Shader::Stage::Vertex:
                VkStages[i] = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case Shader::Stage::Geometry:
                MWARN("VulkanAPI::LoadShader: VK_SHADER_STAGE_GEOMETRY_BIT установлен, но еще не поддерживается.");
                VkStages[i] = VK_SHADER_STAGE_GEOMETRY_BIT;
                break;
            case Shader::Stage::Compute:
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
    auto VulkShader = shader->ShaderData;

    VulkShader->renderpass = reinterpret_cast<VulkanRenderpass*>(renderpass->InternalData);

    // Создайте конфигурацию.
    VulkShader->config.MaxDescriptorSetCount = MaxDescriptorAllocateCount;

    // Этапы шейдера. Разбираем флаги.
    // MMemory::ZeroMem(OutShader->config.stages, sizeof(VulkanShaderStageConfig) * VulkanShaderConstants::MaxStages);
    VulkShader->config.StageCount = 0;
    // Перебрать предоставленные этапы.
    for (u32 i = 0; i < stages.Length(); i++) {
        // Убедитесь, что достаточно места для добавления сцены.
        if (VulkShader->config.StageCount + 1 > VulkanShaderConstants::MaxStages) {
            MERROR("Шейдеры могут иметь максимум %d стадий.", VulkanShaderConstants::MaxStages);
            return false;
        }

        // Убедитесь, что сцена поддерживается.
        VkShaderStageFlagBits StageFlag;
        switch (stages[i]) {
            case Shader::Stage::Vertex:
                StageFlag = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case Shader::Stage::Fragment:
                StageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            default:
                // Перейти к следующему типу.
                MERROR("VulkanAPI::LoadShader: Отмечена неподдерживаемая стадия шейдера: %d. Стадия проигнорирована.", stages[i]);
                continue;
        }

        // Подготовьте сцену и ударьте по счетчику.
        VulkShader->config.stages[VulkShader->config.StageCount].stage = StageFlag;
        MString::Copy(VulkShader->config.stages[VulkShader->config.StageCount].FileName, StageFilenames[i].c_str(), 255);
        VulkShader->config.StageCount++;
    }

    VulkShader->config.DescriptorSets[0].SamplerBindingIndex = INVALID::U8ID;
    VulkShader->config.DescriptorSets[1].SamplerBindingIndex = INVALID::U8ID;

    // Получите данные по количеству униформы.
    VulkShader->GlobalUniformCount = 0;
    VulkShader->GlobalUniformSamplerCount = 0;
    VulkShader->InstanceUniformCount = 0;
    VulkShader->InstanceUniformSamplerCount = 0;
    VulkShader->LocalUniformCount = 0;
    const u32& TotalCount = config.uniforms.Length();
    for (u32 i = 0; i < TotalCount; ++i) {
        switch (config.uniforms[i].scope) {
            case Shader::Scope::Global:
                if (config.uniforms[i].type == Shader::UniformType::Sampler) {
                    VulkShader->GlobalUniformSamplerCount++;
                } else {
                    VulkShader->GlobalUniformCount++;
                }
                break;
            case Shader::Scope::Instance:
                if (config.uniforms[i].type == Shader::UniformType::Sampler){
                    VulkShader->InstanceUniformSamplerCount++;
                } else {
                    VulkShader->InstanceUniformCount++;
                }
                break;
            case Shader::Scope::Local:
                VulkShader->LocalUniformCount++;
                break;
        }
    }

    // На данный момент шейдеры будут иметь только эти два типа пулов дескрипторов.
    VulkShader->config.PoolSizes[0] = VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024};          // HACK: максимальное количество наборов дескрипторов ubo.
    VulkShader->config.PoolSizes[1] = VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4096};  // HACK: максимальное количество наборов дескрипторов сэмплера изображений.

    // Конфигурация глобального набора дескрипторов.
    if (VulkShader->GlobalUniformCount > 0 || VulkShader->GlobalUniformSamplerCount > 0) {
    
        auto& SetConfig = VulkShader->config.DescriptorSets[VulkShader->config.DescriptorSetCount];

        // При наличии глобального UBO-привязки он является первым.
        if (VulkShader->GlobalUniformCount > 0) {
            const u8& BindingIndex = SetConfig.BindingCount;
            SetConfig.bindings[BindingIndex].binding = BindingIndex;
            SetConfig.bindings[BindingIndex].descriptorCount = 1;
            SetConfig.bindings[BindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            SetConfig.bindings[BindingIndex].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            SetConfig.BindingCount++;
        }

        // Добавьте привязку для сэмплеров, если они используются.
        if (VulkShader->GlobalUniformSamplerCount > 0) {
            const u8& BindingIndex = SetConfig.BindingCount;
            SetConfig.bindings[BindingIndex].binding = BindingIndex;
            SetConfig.bindings[BindingIndex].descriptorCount = VulkShader->GlobalUniformSamplerCount;  // Один дескриптор на сэмплер.
            SetConfig.bindings[BindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            SetConfig.bindings[BindingIndex].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            SetConfig.SamplerBindingIndex = BindingIndex;
            SetConfig.BindingCount++;
        }

        // Увеличить установленный счетчик.
        VulkShader->config.DescriptorSetCount++;
    }

    // Если используются униформы экземпляров, добавьте набор дескрипторов UBO.
    if (VulkShader->InstanceUniformCount > 0 || VulkShader->InstanceUniformSamplerCount > 0) {
        // В этом наборе добавьте привязку для UBO, если она используется.
        auto& SetConfig = VulkShader->config.DescriptorSets[VulkShader->config.DescriptorSetCount];

        if (VulkShader->InstanceUniformCount > 0) {
            const u8& BindingIndex = SetConfig.BindingCount;
            SetConfig.bindings[BindingIndex].binding = BindingIndex;
            SetConfig.bindings[BindingIndex].descriptorCount = 1;
            SetConfig.bindings[BindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            SetConfig.bindings[BindingIndex].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            SetConfig.BindingCount++;
        }

        // Добавьте привязку для сэмплеров, если она используется.
        if (VulkShader->InstanceUniformSamplerCount > 0) {
            const u8& BindingIndex = SetConfig.BindingCount;
            SetConfig.bindings[BindingIndex].binding = BindingIndex;
            SetConfig.bindings[BindingIndex].descriptorCount = VulkShader->InstanceUniformSamplerCount;  // Один дескриптор на сэмплер.
            SetConfig.bindings[BindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            SetConfig.bindings[BindingIndex].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            SetConfig.SamplerBindingIndex = BindingIndex;
            SetConfig.BindingCount++;
        }

        // Увеличьте счетчик набора.
        VulkShader->config.DescriptorSetCount++;
    }

   // Сохраните копию режима отбраковки.
    VulkShader->config.CullMode = config.CullMode;

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
        RenderBufferUnmapMemory(VkShader->UniformBuffer, 0, VK_WHOLE_SIZE);
        VkShader->MappedUniformBufferBlock = 0;
        RenderBufferDestroyInternal(VkShader->UniformBuffer);

        // Конвеер
        VkShader->pipeline.Destroy(this);

        // Шейдерные модули
        for (u32 i = 0; i < VkShader->config.StageCount; ++i) {
            vkDestroyShaderModule(Device.LogicalDevice, VkShader->stages[i].handle, allocator);
        }

        // Уничтожьте конфигурацию.
        MemorySystem::ZeroMem(&VkShader->config, sizeof(VulkanShaderConfig));

        // Освободите внутреннюю память данных.
        MemorySystem::Free(shader->ShaderData, sizeof(VulkanShader), Memory::Renderer);
        shader->ShaderData = 0;
    }
}

bool VulkanAPI::ShaderInitialize(Shader *shader)
{
    auto& LogicalDevice = Device.LogicalDevice;
    auto VkShader = shader->ShaderData;

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
        t[Shader::AttributeType::Float32]   =          VK_FORMAT_R32_SFLOAT;
        t[Shader::AttributeType::Float32_2] =       VK_FORMAT_R32G32_SFLOAT;
        t[Shader::AttributeType::Float32_3] =    VK_FORMAT_R32G32B32_SFLOAT;
        t[Shader::AttributeType::Float32_4] = VK_FORMAT_R32G32B32A32_SFLOAT;
        t[Shader::AttributeType::Int8]      =             VK_FORMAT_R8_SINT;
        t[Shader::AttributeType::UInt8]     =             VK_FORMAT_R8_UINT;
        t[Shader::AttributeType::Int16]     =            VK_FORMAT_R16_SINT;
        t[Shader::AttributeType::UInt16]    =            VK_FORMAT_R16_UINT;
        t[Shader::AttributeType::Int32]     =            VK_FORMAT_R32_SINT;
        t[Shader::AttributeType::UInt32]    =            VK_FORMAT_R32_UINT;
        types = t;
    }

    // Атрибуты процесса
    const auto& AttributeCount = shader->attributes.Length();
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
    viewport.x = 0.F;
    viewport.y = (f32)FramebufferHeight;
    viewport.width = (f32)FramebufferWidth;
    viewport.height = -(f32)FramebufferHeight;
    viewport.minDepth = 0.F;
    viewport.maxDepth = 1.F;

    // Scissor
    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = FramebufferWidth;
    scissor.extent.height = FramebufferHeight;

    VkPipelineShaderStageCreateInfo StageCreateIfos[VulkanShaderConstants::MaxStages]{};
    for (u32 i = 0; i < VkShader->config.StageCount; ++i) {
        StageCreateIfos[i] = VkShader->stages[i].ShaderStageCreateInfo;
    }

    VulkanPipeline::Config PipelineConfig {
        shader->name,
        VkShader->renderpass,
        shader->AttributeStride,
        (u32)shader->attributes.Length(),
        VkShader->config.attributes,  // shader->attributes,
        VkShader->config.DescriptorSetCount,
        VkShader->DescriptorSetLayouts,
        VkShader->config.StageCount,
        StageCreateIfos,
        viewport,
        scissor,
        VkShader->config.CullMode,
        false,
        shader->flags,
        shader->PushConstantRangeCount,
        shader->PushConstantRanges
    };

    bool PipelineResult = VkShader->pipeline.Create(this, PipelineConfig);

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
    // ЗАДАЧА: Максимальное количество должно быть настраиваемым или, возможно, иметь долгосрочную поддержку изменения размера буфера.
    const u64 TotalBufferSize = shader->GlobalUboStride + (shader->UboStride * VULKAN_MAX_MATERIAL_COUNT);  // global + (locals)
    char bufname[] = "renderbuffer_global_uniform";
    if (!RenderBufferCreate(bufname, RenderBufferType::Uniform, TotalBufferSize, true, VkShader->UniformBuffer)) {
        MERROR("VulkanAPI::ShaderInitialize — не удалось создать буфер Vulkan для шейдера объекта.");
        return false;
    }
    RenderBufferBind(VkShader->UniformBuffer, 0);

    // Выделите место для глобального UBO, которое должно занимать пространство _шага_, а не фактически используемый размер.
    if (!VkShader->UniformBuffer.Allocate(shader->GlobalUboStride, shader->GlobalUboOffset)) {
        MERROR("Не удалось выделить место для универсального буфера!");
        return false;
    }

    // Отобразите всю память буфера.
    VkShader->MappedUniformBufferBlock = RenderBufferMapMemory(VkShader->UniformBuffer, 0, VK_WHOLE_SIZE);

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
    BufferInfo.buffer = reinterpret_cast<VulkanBuffer*>(VkShader->UniformBuffer.data)->handle;
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
    auto VkShader = shader->ShaderData;
    if (VkShader->InstanceUniformCount < 1 && VkShader->InstanceUniformSamplerCount < 1) {
        MERROR("Этот шейдер не использует экземпляры.");
        return false;
    }
    
    const auto& CommandBuffer = GraphicsCommandBuffers[ImageIndex].handle;

    // Получите данные экземпляра.
    auto& ObjectState = VkShader->InstanceStates[shader->BoundInstanceID];
    const auto& ObjectDescriptorSet = ObjectState.DescriptorSetState.DescriptorSets[ImageIndex];

    if(NeedsUpdate) {
        VkWriteDescriptorSet DescriptorWrites[2] {};  // Всегда максимум два набора дескрипторов.
        u32 DescriptorCount = 0;
        u32 DescriptorIndex = 0;
        VkDescriptorBufferInfo BufferInfo;
    
        // Дескриптор 0 — универсальный буфер
        if (VkShader->InstanceUniformCount > 0) {
            // Делайте это только в том случае, если дескриптор еще не был обновлен.
            u8& InstanceUboGeneration = ObjectState.DescriptorSetState.DescriptorStates[DescriptorIndex].generations[ImageIndex];
            // ЗАДАЧА: определить, требуется ли обновление.
            if (InstanceUboGeneration == INVALID::U8ID /*|| *global_ubo_generation != material->generation*/) {
                BufferInfo.buffer = reinterpret_cast<VulkanBuffer*>(VkShader->UniformBuffer.data)->handle;
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
                auto map = VkShader->InstanceStates[shader->BoundInstanceID].InstanceTextureMaps[i];
                auto texture = map->texture;

                // Убедитесь, что текстура верна.
                if (texture->generation == INVALID::ID) {
                    texture = TextureSystem::GetDefaultTexture(Texture::Default);
                }

                ImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                ImageInfos[i].imageView = reinterpret_cast<VulkanImage*>(texture->data)->view;
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

bool VulkanAPI::ShaderBindInstance(Shader *shader, u32 InstanceID)
{
    shader->BoundInstanceID = InstanceID;
    shader->BoundUboOffset = shader->ShaderData->InstanceStates[InstanceID].offset;
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

bool VulkanAPI::ShaderAcquireInstanceResources(Shader *shader, u32 TextureMapCount, TextureMap** maps, u32 &OutInstanceID)
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
    // const u8& SamplerBindingIndex = VkShader->config.DescriptorSets[DESC_SET_INDEX_INSTANCE].SamplerBindingIndex;
    // const u32& InstanceTextureCount = VkShader->config.DescriptorSets[DESC_SET_INDEX_INSTANCE].bindings[SamplerBindingIndex].descriptorCount;
    // Настраивайте только в том случае, если шейдер действительно этого требует.
    if (shader->InstanceTextureCount > 0) {
        // Очистите память всего массива, даже если она не вся использована.
        InstanceState.InstanceTextureMaps = reinterpret_cast<TextureMap**>(MemorySystem::Allocate(shader->InstanceTextureCount * sizeof(TextureMap*), Memory::Array, true));
        auto DefaultTexture = TextureSystem::GetDefaultTexture(Texture::Default);
        MemorySystem::CopyMem(InstanceState.InstanceTextureMaps, maps, sizeof(TextureMap*) * TextureMapCount);
        // Установите для всех указателей текстур значения по умолчанию, пока они не будут назначены.
        for (u32 i = 0; i < TextureMapCount; ++i) {
            if (!maps[i]->texture) {
                InstanceState.InstanceTextureMaps[i]->texture = DefaultTexture;
            }
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
    MemorySystem::ZeroMem(InstanceState.DescriptorSetState.DescriptorStates, sizeof(VulkanDescriptorState) * VulkanShaderConstants::MaxBindings);

    if (InstanceState.InstanceTextureMaps) {
        MemorySystem::Free(InstanceState.InstanceTextureMaps, sizeof(TextureMap*) * shader->InstanceTextureCount, Memory::Array);
        InstanceState.InstanceTextureMaps = nullptr;
    }

    if (shader->UboStride != 0) {
        if (!VkShader->UniformBuffer.Free(shader->UboStride, InstanceState.offset)) {
            MERROR("VulkanAPI::ShaderReleaseInstanceResources не удалось освободить диапазон из буфера рендеринга.");
        }
    }

    InstanceState.offset = INVALID::ID;
    InstanceState.id = INVALID::ID;

    return true;
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

const bool& VulkanAPI::IsMultithreaded()
{
    return MultithreadingEnabled;
}

bool VulkanAPI::FlagEnabled(RendererConfigFlags flag)
{
    return bool(swapchain.flags & flag);
}

void VulkanAPI::FlagSetEnabled(RendererConfigFlags flag, bool enabled)
{
    swapchain.flags = (enabled ? (swapchain.flags | flag) : swapchain.flags & ~flag);
    RenderFlagChanged = true;
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

bool VulkanAPI::CreateModule(VulkanShader *shader, const VulkanShaderStageConfig& config, VulkanShaderStage *ShaderStage)
{
    // Прочтите ресурс.
    BinaryResource BinRes;
    if (!ResourceSystem::Load(config.FileName, eResource::Type::Binary, nullptr, BinRes)) {
        MERROR("Невозможно прочитать модуль шейдера: %s.", config.FileName);
        return false;
    }

    // MMemory::ZeroMem(&ShaderStage->CreateInfo, sizeof(VkShaderModuleCreateInfo));
    ShaderStage->CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    // Используйте размер и данные ресурса напрямую.
    ShaderStage->CreateInfo.codeSize = BinRes.data.Length();
    ShaderStage->CreateInfo.pCode = reinterpret_cast<u32*>(BinRes.data.Data());

    VK_CHECK(vkCreateShaderModule(
        Device.LogicalDevice,
        &ShaderStage->CreateInfo,
        allocator,
        &ShaderStage->handle));

    // Освободите ресурс.
    ResourceSystem::Unload(BinRes);

    // Информация об этапе шейдера
    //MMemory::ZeroMem(&ShaderStage->ShaderStageCreateInfo, sizeof(VkPipelineShaderStageCreateInfo));
    ShaderStage->ShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStage->ShaderStageCreateInfo.stage = config.stage;
    ShaderStage->ShaderStageCreateInfo.module = ShaderStage->handle;
    ShaderStage->ShaderStageCreateInfo.pName = "main";

    return true;
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

    // Укажите слушателям, что требуется обновление цели рендеринга.
    EventContext EventContext = {0};
    EventSystem::Fire(EventSystem::DefaultRendertargetRefreshRequired, nullptr, EventContext);

    CreateCommandBuffers();

    // Снимите флаг воссоздания.
    RecreatingSwapchain = false;

    return true;
}

bool VulkanAPI::SetUniform(Shader *shader, Shader::Uniform *uniform, const void *value)
{
    auto VkShader = shader->ShaderData;
    if (uniform->type == Shader::UniformType::Sampler) {
        if (uniform->scope == Shader::Scope::Global) {
            shader->GlobalTextureMaps[uniform->location] = (TextureMap*)value;
        } else {
            VkShader->InstanceStates[shader->BoundInstanceID].InstanceTextureMaps[uniform->location] = (TextureMap*)value;
        }
    } else {
        if (uniform->scope == Shader::Scope::Local) {
            // Является локальным, использует push-константы. Сделайте это немедленно.
            VkCommandBuffer CommandBuffer = GraphicsCommandBuffers[ImageIndex].handle;
            vkCmdPushConstants(CommandBuffer, VkShader->pipeline.PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, uniform->offset, uniform->size, value);
        } else {
            // Сопоставьте подходящую ячейку памяти и скопируйте данные.
            u64 addr = (u64)VkShader->MappedUniformBufferBlock;
            addr += shader->BoundUboOffset + uniform->offset;
            MemorySystem::CopyMem((void*)addr, value, uniform->size);
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

    char FormattedName[TEXTURE_NAME_MAX_LENGTH] = {0};
    MString::Format(FormattedName, "%s_texmap_sampler", map->texture->name);
    VK_SET_DEBUG_OBJECT_NAME(this, VK_OBJECT_TYPE_SAMPLER, (VkSampler)map->sampler, FormattedName);

    return true;
}

void VulkanAPI::TextureMapReleaseResources(TextureMap *map)
{
    if (map) {
        // Убедитесь, что это не используется.
        vkDeviceWaitIdle(Device.LogicalDevice);
        vkDestroySampler(Device.LogicalDevice, reinterpret_cast<VkSampler>(map->sampler), allocator);
        map->sampler = nullptr;
    }
}

void VulkanAPI::RenderTargetCreate(u8 AttachmentCount, RenderTargetAttachment *attachments, Renderpass *pass, u32 width, u32 height, RenderTarget &OutTarget)
{
    // Максимальное количество вложений
    VkImageView AttachmentViews[32];
    for (u32 i = 0; i < AttachmentCount; ++i) {
        AttachmentViews[i] = reinterpret_cast<VulkanImage*>(attachments[i].texture->data)->view;
    }
    
    MemorySystem::CopyMem(OutTarget.attachments, attachments, sizeof(RenderTargetAttachment) * AttachmentCount);

    VkFramebufferCreateInfo FramebufferCreateInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    FramebufferCreateInfo.renderPass = reinterpret_cast<VulkanRenderpass*>(pass->InternalData)->handle;
    FramebufferCreateInfo.attachmentCount = AttachmentCount;
    FramebufferCreateInfo.pAttachments = AttachmentViews;
    FramebufferCreateInfo.width = width;
    FramebufferCreateInfo.height = height;
    FramebufferCreateInfo.layers = 1;

    VK_CHECK(vkCreateFramebuffer(Device.LogicalDevice, &FramebufferCreateInfo, allocator, reinterpret_cast<VkFramebuffer*>(&OutTarget.InternalFramebuffer)));
}

void VulkanAPI::RenderTargetDestroy(RenderTarget &target, bool FreeInternalMemory)
{
    if (target.InternalFramebuffer) {
        vkDestroyFramebuffer(Device.LogicalDevice, (VkFramebuffer)target.InternalFramebuffer, allocator);
        target.InternalFramebuffer = 0;
        if (FreeInternalMemory) {
            MemorySystem::Free(target.attachments, sizeof(RenderTargetAttachment) * target.AttachmentCount, Memory::Array);
            target.attachments = nullptr;
            target.AttachmentCount = 0;
        }
    }
}

bool VulkanAPI::RenderpassCreate(RenderpassConfig& config, Renderpass& OutRenderpass)
{
    OutRenderpass.RenderArea = config.RenderArea;
    OutRenderpass.ClearColour = config.ClearColour;
    OutRenderpass.ClearFlags = config.ClearFlags;
    OutRenderpass.RenderTargetCount = config.RenderTargetCount;
    OutRenderpass.targets = MemorySystem::TAllocate<RenderTarget>(Memory::Array, OutRenderpass.RenderTargetCount, true);
    OutRenderpass.name = config.name;

    // Скопируйте конфигурацию для каждой цели.
    for (u32 t = 0; t < OutRenderpass.RenderTargetCount; ++t) {
        auto& target = OutRenderpass.targets[t];
        target.AttachmentCount = config.target.AttachmentCount;
        target.attachments = MemorySystem::TAllocate<RenderTargetAttachment>(Memory::Array, target.AttachmentCount);

        // Каждое вложение для цели.
        for (u32 a = 0; a < target.AttachmentCount; ++a) {
            auto& attachment = target.attachments[a];
            auto& AttachmentConfig = config.target.attachments[a];

            attachment.source = AttachmentConfig.source;
            attachment.type = AttachmentConfig.type;
            attachment.LoadOperation = AttachmentConfig.LoadOperation;
            attachment.StoreOperation = AttachmentConfig.StoreOperation;
            attachment.PresentAfter = AttachmentConfig.PresentAfter;
            attachment.texture = nullptr;
        }
    }

    return (OutRenderpass.InternalData = new VulkanRenderpass(OutRenderpass.ClearFlags, config, this)) != nullptr;
}

void VulkanAPI::RenderpassDestroy(Renderpass *renderpass)
{
    for (u32 i = 0; i < renderpass->RenderTargetCount; i++) {
        RenderTargetDestroy(renderpass->targets[i], true);
    }
    
    reinterpret_cast<VulkanRenderpass*>(renderpass->InternalData)->Destroy(this);
}

Texture *VulkanAPI::WindowAttachmentGet(u8 index)
{
    if (index >= swapchain.ImageCount) {
        MFATAL("Попытка получить индекс цветового вложения вне диапазона: %d. Количество вложений: %d", index, swapchain.ImageCount);
        return nullptr;
    }

    return &swapchain.RenderTextures[index];
}

Texture *VulkanAPI::DepthAttachmentGet(u8 index)
{
    if (index >= swapchain.ImageCount) {
        MFATAL("Попытка получить индекс вложения глубины вне диапазона: %d. Количество вложений: %d", index, swapchain.ImageCount);
        return nullptr;
    }

    return &swapchain.DepthTextures[index];
}

u8 VulkanAPI::WindowAttachmentIndexGet()
{
    return ImageIndex;
}

u8 VulkanAPI::WindowAttachmentCountGet()
{
    return u8(swapchain.ImageCount);
}

//////////////////////////////////////////////////////////////////////////////////////////////
            //                           RenderBuffer                           //
//////////////////////////////////////////////////////////////////////////////////////////////

// Указывает, имеет ли предоставленный буфер локальную память устройства.
bool VulkanBufferIsDeviceLocal(VulkanBuffer* buffer) 
{
    return (buffer->MemoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
}

// Указывает, имеет ли предоставленный буфер память, видимую хостом.
bool VulkanBufferIsHostVisible(VulkanBuffer* buffer) 
{
    return (buffer->MemoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
}

// Указывает, имеет ли предоставленный буфер память, согласованную с хостом.
bool VulkanBufferIsHostCoherent(VulkanBuffer* buffer) 
{
    return (buffer->MemoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
}

bool VulkanAPI::RenderBufferCreate(const char* name, RenderBufferType type, u64 TotalSize, bool UseFreelist, RenderBuffer &buffer)
{
    buffer.type = type;
    buffer.TotalSize = TotalSize;
    if (name) {
        buffer.name = name;
    } else {
        char TempName[256]{};
        MString::Format(TempName, "renderbuffer_%s", "unnamed");
        buffer.name = TempName;
    }
    if (UseFreelist) {
        buffer.FreelistMemoryRequirement = FreeList::GetMemoryRequirement(TotalSize);
        buffer.FreelistBlock = MemorySystem::Allocate(buffer.FreelistMemoryRequirement, Memory::Renderer);
        buffer.BufferFreelist.Create(TotalSize, buffer.FreelistBlock);
    }
    if (!RenderBufferCreateInternal(buffer)) {
        MERROR("VulkanAPI::RenderBufferCreate не удалось создать RenderBuffer.");
        return false;
    }
    
    return true;
}

bool VulkanAPI::RenderBufferCreateInternal(RenderBuffer &buffer)
{
    VulkanBuffer InternalBuffer;

    switch (buffer.type) {
        case RenderBufferType::Vertex:
            InternalBuffer.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            InternalBuffer.MemoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            break;
        case RenderBufferType::Index:
            InternalBuffer.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            InternalBuffer.MemoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            break;
        case RenderBufferType::Uniform: {
            // u32 DeviceLocalBits = Device.SupportsDeviceLocalHostVisible ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0;
            InternalBuffer.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            InternalBuffer.MemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; // | DeviceLocalBits;
        } break;
        case RenderBufferType::Staging:
            InternalBuffer.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            InternalBuffer.MemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            break;
        case RenderBufferType::Read:
            InternalBuffer.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            InternalBuffer.MemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            break;
        case RenderBufferType::Storage:
            MERROR("Буфер хранения пока не поддерживается.");
            return false;
        default:
            MERROR("Неподдерживаемый тип буфера: %i", buffer.type);
            return false;
    }

    VkBufferCreateInfo BufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    BufferInfo.size = buffer.TotalSize;
    BufferInfo.usage = InternalBuffer.usage;
    BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // ПРИМЕЧАНИЕ: Используется только в одной очереди.

    VK_CHECK(vkCreateBuffer(Device.LogicalDevice, &BufferInfo, allocator, &InternalBuffer.handle));

    // Соберите требования к памяти.
    vkGetBufferMemoryRequirements(Device.LogicalDevice, InternalBuffer.handle, &InternalBuffer.MemoryRequirements);
    InternalBuffer.MemoryIndex = FindMemoryIndex(InternalBuffer.MemoryRequirements.memoryTypeBits, InternalBuffer.MemoryPropertyFlags);
    if (InternalBuffer.MemoryIndex == -1) {
        MERROR("Не удалось создать буфер Vulkan, так как требуемый индекс типа памяти не найден.");
        return false;
    }

    // Выделите информацию о памяти
    VkMemoryAllocateInfo AllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    AllocateInfo.allocationSize = InternalBuffer.MemoryRequirements.size;
    AllocateInfo.memoryTypeIndex = (u32)InternalBuffer.MemoryIndex;

    // Выделите память.
    VkResult result = vkAllocateMemory(Device.LogicalDevice, &AllocateInfo, allocator, &InternalBuffer.memory);

    // Определите, находится ли память в куче устройства.
    bool IsDeviceMemory = (InternalBuffer.MemoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    // Сообщите о памяти как об используемой.
    MemorySystem::AllocateReport(InternalBuffer.MemoryRequirements.size, IsDeviceMemory ? Memory::GPULocal : Memory::Vulkan);

    if (result != VK_SUCCESS) {
        MERROR("Не удалось создать буфер vulkan, так как требуемое выделение памяти не удалось. Ошибка: %i", result);
        return false;
    }

    // Выделите блок внутреннего состояния памяти в конце, как только мы убедимся, что все было создано успешно.
    buffer.data = new VulkanBuffer(InternalBuffer);

    return true;
}

void VulkanAPI::RenderBufferDestroyInternal(RenderBuffer &buffer)
{
    auto VkBuf = reinterpret_cast<VulkanBuffer*>(buffer.data);
    if (VkBuf) {
        if (VkBuf->memory) {
            vkFreeMemory(Device.LogicalDevice, VkBuf->memory, allocator);
            VkBuf->memory = 0;
        }
        if (VkBuf->handle) {
            vkDestroyBuffer(Device.LogicalDevice, VkBuf->handle, allocator);
            VkBuf->handle = 0;
        }

        // Сообщить о свободной памяти.
        bool IsDeviceMemory = (VkBuf->MemoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        MemorySystem::FreeReport(VkBuf->MemoryRequirements.size, IsDeviceMemory ? Memory::GPULocal : Memory::Vulkan);
        MemorySystem::ZeroMem(&VkBuf->MemoryRequirements, sizeof(VkMemoryRequirements));

        VkBuf->usage = static_cast<VkBufferUsageFlagBits>(0);
        VkBuf->IsLocked = false;

        // Освободить внутренний буфер.
        delete reinterpret_cast<VulkanBuffer*>(buffer.data);
        buffer.data = nullptr;
    }
}

bool VulkanAPI::RenderBufferBind(RenderBuffer &buffer, u64 offset)
{
    if (!buffer.data) {
        MERROR("VulkanAPI::RenderBufferBind требует действительный указатель на буфер.")
        return false;
    }
    
    auto VkBuf = reinterpret_cast<VulkanBuffer*>(buffer.data);
    VK_CHECK(vkBindBufferMemory(Device.LogicalDevice, VkBuf->handle, VkBuf->memory, offset))
    return true;
}

bool VulkanAPI::RenderBufferUnbind(RenderBuffer &buffer)
{
    if (!buffer.data) {
        MERROR("VulkanAPI::RenderBufferUnbind требует действительный указатель на буфер.");
        return false;
    }

    // ПРИМЕЧАНИЕ: На данный момент ничего не делает.
    return true;
}

void *VulkanAPI::RenderBufferMapMemory(RenderBuffer &buffer, u64 offset, u64 size)
{
    if (!buffer.data) {
        MERROR("VulkanAPI::RenderBufferMapMemory требует действительный указатель на буфер.");
        return nullptr;
    }
    auto VkBuf = reinterpret_cast<VulkanBuffer*>(buffer.data);
    void* data;
    VK_CHECK(vkMapMemory(Device.LogicalDevice, VkBuf->memory, offset, size, 0, &data));
    return data;
}

void VulkanAPI::RenderBufferUnmapMemory(RenderBuffer &buffer, u64 offset, u64 size)
{
    if (!buffer.data) {
        MERROR("VulkanAPI::RenderBufferUnmapMemory требует действительный указатель на буфер.");
        return;
    }
    auto VkBuffer = reinterpret_cast<VulkanBuffer*>(buffer.data);
    vkUnmapMemory(Device.LogicalDevice, VkBuffer->memory);
}

bool VulkanAPI::RenderBufferFlush(RenderBuffer &buffer, u64 offset, u64 size)
{
    if (!buffer.data) {
        MERROR("VulkanAPI::RenderBufferFlush требует действительный указатель на буфер.");
        return false;
    }
    // ПРИМЕЧАНИЕ: Если нет согласованности с хостом, очистите отображенный диапазон памяти.
    auto VkBuf = reinterpret_cast<VulkanBuffer*>(buffer.data);
    if (!VulkanBufferIsHostCoherent(VkBuf)) {
        VkMappedMemoryRange range = {VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
        range.memory = VkBuf->memory;
        range.offset = offset;
        range.size = size;
        VK_CHECK(vkFlushMappedMemoryRanges(Device.LogicalDevice, 1, &range));
    }

    return true;
}

bool VulkanAPI::RenderBufferRead(RenderBuffer &buffer, u64 offset, u64 size, void **OutMemory)
{
    if (!buffer.data || !OutMemory) {
        MERROR("VulkanAPI::RenderBufferRead requires a valid pointer to a buffer and out_memory, and the size must be nonzero.");
        return false;
    }

    auto VkBuf = reinterpret_cast<VulkanBuffer*>(buffer.data);
    if (VulkanBufferIsDeviceLocal(VkBuf) && !VulkanBufferIsHostVisible(VkBuf)) {
        // ПРИМЕЧАНИЕ: Если необходим буфер чтения (т.е. память целевого буфера не видна хосту, но является локальной для устройства, 
        // создайте буфер чтения, скопируйте в него данные, затем выполните чтение из этого буфера.

        // Создайте видимый хосту промежуточный буфер для копирования. Отметьте его как место назначения передачи.
        RenderBuffer read;
        char bufname[] = "renderbuffer_read";
        if (!RenderBufferCreate(bufname, RenderBufferType::Read, size, false, read)) {
            MERROR("VulkanAPI::RenderBufferRead() - Не удалось создать буфер чтения.");
            return false;
        }
        RenderBufferBind(read, 0);
        auto ReadInternal = reinterpret_cast<VulkanBuffer*>(read.data);

        // Выполните копирование из локального устройства в буфер чтения.
        RenderBufferCopyRange(buffer, offset, read, 0, size);

        // Map/copy/unmap
        void* MappedData;
        VK_CHECK(vkMapMemory(Device.LogicalDevice, ReadInternal->memory, 0, size, 0, &MappedData));
        MemorySystem::CopyMem(*OutMemory, MappedData, size);
        vkUnmapMemory(Device.LogicalDevice, ReadInternal->memory);

        // Очистите буфер чтения.
        RenderBufferUnbind(read);
        RenderBufferDestroyInternal(read);
    } else {
        // Если промежуточный буфер не нужен, отобразите/скопируйте/отмените отображение.
        void* PtrData;
        VK_CHECK(vkMapMemory(Device.LogicalDevice, VkBuf->memory, offset, size, 0, &PtrData));
        MemorySystem::CopyMem(*OutMemory, PtrData, size);
        vkUnmapMemory(Device.LogicalDevice, VkBuf->memory);
    }

    return true;
}

bool VulkanAPI::RenderBufferResize(RenderBuffer &buffer, u64 NewTotalSize)
{
    if (!buffer.data) {
        return false;
    }
    buffer.Resize(NewTotalSize);

    auto VkBuf = reinterpret_cast<VulkanBuffer*>(buffer.data);

    // Создать новый буфер.
    VkBufferCreateInfo BufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    BufferInfo.size = NewTotalSize;
    BufferInfo.usage = VkBuf->usage;
    BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // ПРИМЕЧАНИЕ: Используется только в одной очереди.

    VkBuffer NewBuffer;
    VK_CHECK(vkCreateBuffer(Device.LogicalDevice, &BufferInfo, allocator, &NewBuffer));

    // Собрать требования к памяти.
    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(Device.LogicalDevice, NewBuffer, &requirements);

    // Выделить информацию о памяти
    VkMemoryAllocateInfo AllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    AllocateInfo.allocationSize = requirements.size;
    AllocateInfo.memoryTypeIndex = (u32)VkBuf->MemoryIndex;

    // Выделить память.
    VkDeviceMemory NewMemory;
    VkResult result = vkAllocateMemory(Device.LogicalDevice, &AllocateInfo, allocator, &NewMemory);
    if (result != VK_SUCCESS) {
        MERROR("Невозможно изменить размер буфера vulkan, так как требуемое выделение памяти не удалось. Ошибка: %i", result);
        return false;
    }

    // Связать память нового буфера
    VK_CHECK(vkBindBufferMemory(Device.LogicalDevice, NewBuffer, NewMemory, 0));

    // Скопировать данные.
    VulkanBufferCopyRangeInternal(VkBuf->handle, 0, NewBuffer, 0, buffer.TotalSize);

    // Убедитесь, что все, что потенциально использует их, завершено.
    // ПРИМЕЧАНИЕ: Мы могли бы использовать vkQueueWaitIdle здесь, если бы знали, с какой очередью будет использоваться этот буфер...
    vkDeviceWaitIdle(Device.LogicalDevice);

    // Уничтожить старый
    if (VkBuf->memory) {
        vkFreeMemory(Device.LogicalDevice, VkBuf->memory, allocator);
        VkBuf->memory = 0;
    }
    if (VkBuf->handle) {
        vkDestroyBuffer(Device.LogicalDevice, VkBuf->handle, allocator);
        VkBuf->handle = 0;
    }

    // Отчет об освобождении старого, выделение нового.
    bool IsDeviceMemory = (VkBuf->MemoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    MemorySystem::FreeReport(VkBuf->MemoryRequirements.size, IsDeviceMemory ? Memory::GPULocal : Memory::Vulkan);
    VkBuf->MemoryRequirements = requirements;
    MemorySystem::AllocateReport(VkBuf->MemoryRequirements.size, IsDeviceMemory ? Memory::GPULocal : Memory::Vulkan);

    // Установить новые свойства
    VkBuf->memory = NewMemory;
    VkBuf->handle = NewBuffer;

    return true;
}

bool VulkanAPI::RenderBufferLoadRange(RenderBuffer &buffer, u64 offset, u64 size, const void *data)
{
    if (!buffer.data || !size || !data) {
        MERROR("VulkanAPI::RenderBufferLoadRange requires a valid pointer to a buffer, a nonzero size and a valid pointer to data.");
        return false;
    }

    auto VkBuf = reinterpret_cast<VulkanBuffer*>(buffer.data);
    if (VulkanBufferIsDeviceLocal(VkBuf) && !VulkanBufferIsHostVisible(VkBuf)) {
        // ПРИМЕЧАНИЕ: Если необходим промежуточный буфер (т.е. память целевого буфера не видна хосту, но является локальной для устройства), 
        // сначала создайте промежуточный буфер для загрузки данных. Затем скопируйте из него в целевой буфер.

        // Создайте видимый хосту промежуточный буфер для загрузки. Отметьте его как источник передачи.
        RenderBuffer staging;
        char bufname[] = "renderbuffer_loadrange_staging";
        if (!RenderBufferCreate(bufname, RenderBufferType::Staging, size, false, staging)) {
            MERROR("VulkanAPI::RenderBufferLoadRange() - Не удалось создать промежуточный буфер.");
            return false;
        }
        RenderBufferBind(staging, 0);

        // Загрузите данные в промежуточный буфер.
        RenderBufferLoadRange(staging, 0, size, data);

        // Выполните копирование из промежуточного буфера в локальный буфер устройства.
        RenderBufferCopyRange(staging, 0, buffer, offset, size);

        // Очистите промежуточный буфер.
        RenderBufferUnbind(staging);
        RenderBufferDestroyInternal(staging);
    } else {
        // Если промежуточный буфер не нужен, отобразите/скопируйте/отмените отображение.
        void* ptrData;
        VK_CHECK(vkMapMemory(Device.LogicalDevice, VkBuf->memory, offset, size, 0, &ptrData));
        MemorySystem::CopyMem(ptrData, data, size);
        vkUnmapMemory(Device.LogicalDevice, VkBuf->memory);
    }

    return true;
}

bool VulkanAPI::RenderBufferCopyRange(RenderBuffer &source, u64 SourceOffset, RenderBuffer &dest, u64 DestOffset, u64 size)
{
    if (!source.data || !dest.data || !size) {
        MERROR("VulkanAPI::RenderBufferCopyRange требует действительных указателей на исходный и целевой буферы, а также ненулевой размер.");
        return false;
    }

    return VulkanBufferCopyRangeInternal(
        reinterpret_cast<VulkanBuffer*>(source.data)->handle,
        SourceOffset,
        reinterpret_cast<VulkanBuffer*>(dest.data)->handle,
        DestOffset,
        size);
    return true;
}

bool VulkanAPI::RenderBufferDraw(RenderBuffer &buffer, u64 offset, u32 ElementCount, bool BindOnly)
{
    auto& CommandBuffer = GraphicsCommandBuffers[ImageIndex];

    if (buffer.type == RenderBufferType::Vertex) {
        // Привязать буфер вершин по смещению.
        VkDeviceSize offsets[1] = {offset};
        vkCmdBindVertexBuffers(CommandBuffer.handle, 0, 1, &reinterpret_cast<VulkanBuffer*>(buffer.data)->handle, offsets);
        if (!BindOnly) {
            vkCmdDraw(CommandBuffer.handle, ElementCount, 1, 0, 0);
        }
        return true;
    } else if (buffer.type == RenderBufferType::Index) {
        // Привязать буфер индексов по смещению.
        vkCmdBindIndexBuffer(CommandBuffer.handle, reinterpret_cast<VulkanBuffer*>(buffer.data)->handle, offset, VK_INDEX_TYPE_UINT32);
        if (!BindOnly) {
            vkCmdDrawIndexed(CommandBuffer.handle, ElementCount, 1, 0, 0, 0);
        }
        return true;
    } else {
        MERROR("Невозможно нарисовать буфер типа: %i", buffer.type);
        return false;
    }
}

bool VulkanAPI::VulkanBufferCopyRangeInternal(VkBuffer source, u64 SourceOffset, VkBuffer dest, u64 DestOffset, u64 size)
{
    // ЗАДАЧА: Предполагая использование очереди и пула здесь. Возможно, понадобится выделенная очередь.
    auto queue = Device.GraphicsQueue;
    vkQueueWaitIdle(queue);
    // Создайте одноразовый буфер команд.
    VulkanCommandBuffer TempCommandBuffer;
    VulkanCommandBufferAllocateAndBeginSingleUse(this, Device.GraphicsCommandPool, TempCommandBuffer);

    // Подготовьте команду копирования и добавьте ее в буфер команд.
    VkBufferCopy CopyRegion;
    CopyRegion.srcOffset = SourceOffset;
    CopyRegion.dstOffset = DestOffset;
    CopyRegion.size = size;
    vkCmdCopyBuffer(TempCommandBuffer.handle, source, dest, 1, &CopyRegion);

    // Отправьте буфер на выполнение и дождитесь его завершения.
    VulkanCommandBufferEndSingleUse(this, Device.GraphicsCommandPool, TempCommandBuffer, queue);

    return true;
}
