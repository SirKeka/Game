#include "vulkan_api.h"
#include "vulkan_image.h"
#include "vulkan_buffer.hpp"
#include "platform\vulkan_platform.h"
#include "vulkan_renderpass.h"

#include "renderer/renderbuffer.h"
#include "renderer/renderer_utils.h"
#include "renderer/rendering_system.h"
#include "renderer/viewport.h"

#include <systems/material_system.h>
#include <systems/texture_system.h>
#include <systems/resource_system.h>
#include <core/frame_data.h>
#include <core/event.h>
#include "core/metrics.h"
#include "core/systems_manager.h"

#include <math/vertex.h>

#include "vulkan_utils.h"


// Для компиляции шейдера во время выполнения.
#include <shaderc/shaderc.h>
#include <shaderc/status.h>

// ПРИМЕЧАНИЕ: Если вы хотите отслеживать выделения, раскомментируйте это.
// #ifndef MVULKAN_ALLOCATOR_TRACE
// #define MVULKAN_ALLOCATOR_TRACE 1
// #endif

// ПРИМЕЧАНИЕ: Чтобы отключить пользовательский распределитель, закомментируйте это или установите значение 0.
// #ifndef MVULKAN_USE_CUSTOM_ALLOCATOR
// #define MVULKAN_USE_CUSTOM_ALLOCATOR 1
// #endif

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
        MemorySystem::CopyMem(result, original, AllocSize);
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

VulkanAPI::VulkanAPI() : 
ApiMajor(),
ApiMinor(),
ApiPatch(),
FrameDeltaTime(),
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

device(), swapchain(),
staging("staging", RenderBufferType::Staging, RenderBuffer::AllocationType::Linear, 320 * 1000 * 1000), 
GraphicsCommandBuffers(3),
ImageAvailableSemaphores(),
QueueCompleteSemaphores(),
InFlightFenceCount(),
InFlightFences(),
ImageIndex(),
CurrentFrame(),
RecreatingSwapchain(false),
RenderFlagChanged(false),
WorldRenderTargets(),
MultithreadingEnabled(false) {}

VulkanAPI::~VulkanAPI()
{
    vkDeviceWaitIdle(device.LogicalDevice);

    // Уничтожать в порядке, обратном порядку создания.

    RenderBufferDestroy(staging);

    // Синхронизация объектов
    for (u8 i = 0; i < swapchain.MaxFramesInFlight; ++i) {
        if (ImageAvailableSemaphores[i]) {
            vkDestroySemaphore(device.LogicalDevice, ImageAvailableSemaphores[i], allocator);
            ImageAvailableSemaphores[i] = nullptr;
        }
        if (QueueCompleteSemaphores[i]) {
            vkDestroySemaphore(device.LogicalDevice, QueueCompleteSemaphores[i], allocator);
            QueueCompleteSemaphores[i] = nullptr;
        }
        vkDestroyFence(device.LogicalDevice, InFlightFences[i], allocator);
    }

    // Буферы команд
    for (u32 i = 0; i < swapchain.ImageCount; ++i) {
        if (GraphicsCommandBuffers[i].handle) {
            VulkanCommandBufferFree(this, device.GraphicsCommandPool, GraphicsCommandBuffers[i]);
                GraphicsCommandBuffers[i].handle = nullptr;
        }
    }

    // Цепочка подкачки (Swapchain)
    swapchain.Destroy(this);

    MDEBUG("Уничтожение устройства Вулкан...");
    device.Destroy(this);

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
    allocator = Allocate<VkAllocationCallbacks>(1, Memory::Renderer);
    if (!CreateVulkanAllocator(allocator)) {
        // Если это не удается, аккуратно вернитесь к распределителю по умолчанию.
        MFATAL("Не удалось создать пользовательский распределитель Vulkan. Продолжаем использовать распределитель драйвера по умолчанию.");
        MemorySystem::Free(allocator, sizeof(VkAllocationCallbacks), Memory::Renderer);
        allocator = nullptr;
        }
#else
    allocator = nullptr;
#endif

    ValidationEnabled = config.flags & RenderingConfigFlagBits::EnableValidation ? true : false;

    // Получите текущую установленную версию экземпляра. Не обязательно ту, что используется на устройстве. Используйте её для создания экземпляра.
    u32 ApiVersion = 0;
    vkEnumerateInstanceVersion(&ApiVersion);
    ApiMajor = VK_VERSION_MAJOR(ApiVersion);
    ApiMinor = VK_VERSION_MINOR(ApiVersion);
    ApiPatch = VK_VERSION_PATCH(ApiVersion);

    // Общая структура информации о приложении.
    VkApplicationInfo AppInfo {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = config.ApplicationName,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0), // VK_MAKE_API_VERSION(0, 1, 0, 0),
        .pEngineName = "Moon Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0), // VK_MAKE_API_VERSION(0, 1, 0, 0),
        .apiVersion = VK_MAKE_API_VERSION(0, ApiMajor, ApiMinor, ApiPatch)
    };

    // Создание экземпляра.
    VkInstanceCreateInfo CreateInfo = { 
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &AppInfo
    };

    // Получите список необходимых расширений
    
    DArray<const char*>RequiredExtensions{ AllocatorType::Temp };                                  // Общее расширение поверхности
    RequiredExtensions.PushBack(VK_KHR_SURFACE_EXTENSION_NAME);
    PlatformGetRequiredExtensionNames(RequiredExtensions);                  // Расширения для конкретной платформы
#if defined(_DEBUG)
    RequiredExtensions.PushBack(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);         // утилиты отладки

    MDEBUG("Необходимые расширения:");
    const u32 RequiredExtensionCount = RequiredExtensions.Length();
    for (u32 i = 0; i < RequiredExtensionCount; ++i) {
        MDEBUG(RequiredExtensions[i]);
    }
#endif

    CreateInfo.enabledExtensionCount = RequiredExtensions.Length();
    CreateInfo.ppEnabledExtensionNames = RequiredExtensions.Data(); //ЗАДАЧА: указателю ppEnabledExtensionNames присваевается адрес указателя массива после выхода из функции данные стираются

    u32 AvailableExtensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &AvailableExtensionCount, nullptr);
    DArray<VkExtensionProperties> AvailableExtensions{ AllocatorType::Temp };
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
    DArray<const char*> RequiredValidationLayerNames(AllocatorType::Temp); // указатель на массив символов ЗАДАЧА: придумать как использовать строки или другой способ отображать занятую память
    u32 RequiredValidationLayerCount = 0;

// Если необходимо выполнить проверку, получите список имен необходимых слоев проверки 
// и убедитесь, что они существуют. Слои проверки следует включать только в нерелизных сборках.
    if (ValidationEnabled) {
        MINFO("Уровни проверки включены. Перечисление...");

        // Список требуемых уровней проверки.
        RequiredValidationLayerNames.PushBack("VK_LAYER_KHRONOS_validation");
        RequiredValidationLayerCount = RequiredValidationLayerNames.Length();

        // Получите список доступных уровней проверки.
        u32 AvailableLayerCount = 0;
        VK_CHECK(vkEnumerateInstanceLayerProperties(&AvailableLayerCount, nullptr));
        DArray<VkLayerProperties> AvailableLayers(AvailableLayerCount, AllocatorType::Temp);
        AvailableLayers.Resize(AvailableLayerCount);
        VK_CHECK(vkEnumerateInstanceLayerProperties(&AvailableLayerCount, AvailableLayers.Data())); 

        // Убедитесь, что доступны все необходимые слои.
        for (u32 i = 0; i < RequiredValidationLayerCount; ++i) {
            bool found = false;
            for (u32 j = 0; j < AvailableLayerCount; ++j) {
                if (MString::Compare(RequiredValidationLayerNames[i], AvailableLayers[j].layerName)) {
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
    } else {
        MINFO("Слои валидации Vulkan не включены.");
    }
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
    u32 LogSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT DebugCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = LogSeverity,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT,
        .pfnUserCallback = VkDebugCallback
    };

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
    if (!device.Create(this)) {
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
        vkCreateSemaphore(device.LogicalDevice, &SemaphoreCreateInfo, allocator, &ImageAvailableSemaphores[i]);
        vkCreateSemaphore(device.LogicalDevice, &SemaphoreCreateInfo, allocator, &QueueCompleteSemaphores[i]);

        // Создайте ограждение в сигнальном состоянии, указывая, что первый кадр уже «отрисован».
        // Это не позволит приложению бесконечно ждать рендеринга первого кадра, 
        // поскольку он не может быть отрисован до тех пор, пока кадр не будет "отрисован" перед ним.
        VkFenceCreateInfo FenceCreateInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VK_CHECK(vkCreateFence(device.LogicalDevice, &FenceCreateInfo, allocator, &InFlightFences[i]));
    }

    // Промежуточный буфер
    if (!RenderBufferCreate(staging)) {
        MERROR("Ошибка создания промежуточного буфера.")
        return false;
    }
    RenderBufferBind(staging, 0);

    // Создайте компилятор шейдеров для использования.
    ShaderCompiler = shaderc_compiler_initialize();

    MINFO("Средство визуализации Vulkan успешно инициализировано.");
    return true;
}

void VulkanAPI::Shutdown()
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

void VulkanAPI::BeginDebugLabel(const char *LabelText, FVec3 colour)
{
#ifdef _DEBUG
    VulkanCommandBuffer& CommandBuffer = GraphicsCommandBuffers[ImageIndex];
#endif
    VK_BEGIN_DEBUG_LABEL(this, CommandBuffer.handle, LabelText, FVec4(colour, 1.F));
}

void VulkanAPI::EndDebugLabel()
{
#ifdef _DEBUG
    VulkanCommandBuffer& CommandBuffer = GraphicsCommandBuffers[ImageIndex];
#endif
    VK_END_DEBUG_LABEL(this, CommandBuffer.handle);
}

bool VulkanAPI::PrepareFrame(const FrameData& rFrameData)
{
    // FrameDeltaTime = rFrameData.DeltaTime;

    // Проверьте, не воссоздается ли цепочка подкачки заново, и загрузитесь.
    if (RecreatingSwapchain) {
        VkResult result = vkDeviceWaitIdle(device.LogicalDevice);
        if (!VulkanResultIsSuccess(result)) {
            MERROR("Ошибка VulkanBeginFrame vkDeviceWaitIdle (1): '%shader'", VulkanResultString(result, true));
            return false;
        }
        MINFO("Воссоздание цепочки подкачки, загрузка.");
        return false;
    }

    // Сбросить промежуточный буфер.
    staging.Clear(false);

    // Проверьте, был ли изменен размер фреймбуфера. Если это так, необходимо создать новую цепочку подкачки.
    // Также включите проверку изменения вертикальной синхронизации.
    if (FramebufferSizeGeneration != FramebufferSizeLastGeneration || RenderFlagChanged) {
        VkResult result = vkDeviceWaitIdle(device.LogicalDevice);
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
    VkResult result = vkWaitForFences(device.LogicalDevice, 1, &InFlightFences[CurrentFrame], true, UINT64_MAX);
    if (!VulkanResultIsSuccess(result)) {
        MERROR("Ошибка ожидания в полете! ошибка: %shader", VulkanResultString(result, true));
    }

    // Получаем следующее изображение из цепочки обмена. Передайте семафор, который должен сигнализировать, когда это завершится.
    // Этот же семафор позже будет ожидаться при отправке в очередь, чтобы убедиться, что это изображение доступно.
    result = vkAcquireNextImageKHR(
        device.LogicalDevice,
        swapchain.handle,
        UINT64_MAX,
        ImageAvailableSemaphores[CurrentFrame],
        VK_NULL_HANDLE,
        &ImageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // Запустите воссоздание цепочки обмена, затем выйдите из цикла рендеринга.
        swapchain.Recreate(this, FramebufferWidth, FramebufferHeight);
        return false;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        MFATAL("Не удалось получить изображение цепочки обмена!");
        return false;
    }

    // Сбросьте ограждение для использования в следующем кадре.
    VK_CHECK(vkResetFences(device.LogicalDevice, 1, &InFlightFences[CurrentFrame]));

    return true;
}

bool VulkanAPI::Begin(const FrameData &rFrameData)
{
    // Начните запись команд.
    VulkanCommandBuffer& CommandBuffer = GraphicsCommandBuffers[ImageIndex];

    VulkanCommandBufferReset(CommandBuffer);
    VulkanCommandBufferBegin(CommandBuffer, false, false, false);

    // Динамическое состояние
    SetWinding(RendererWinding::CounterClockwise);

    SetStencilReference(0);
    SetStencilCompareMask(0xFF);
    SetStencilOp(
        RendererStencilOp::Keep,
        RendererStencilOp::Replace,
        RendererStencilOp::Keep,
        RendererCompareOp::Always
    );
    SetStencilTestEnabled(false);
    SetDepthTestEnabled(true);
    // Отключить трафаретную печать.
    SetStencilWriteMask(0x00);

    return true;
}

bool VulkanAPI::End(const FrameData& rFrameData)
{
    VulkanCommandBuffer& CommandBuffer = GraphicsCommandBuffers[ImageIndex];

    VulkanCommandBufferEnd(CommandBuffer);

    // Отправьте очередь и дождитесь завершения операции.
    // Начать отправку в очередь
    VkSubmitInfo SubmitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        // Буфер(ы) команд, которые должны быть выполнены.
        .commandBufferCount = 1,
        .pCommandBuffers = &CommandBuffer.handle
    };
    
    if (DrawIndex == 0) {
        // Семафор(ы), который(ы) будет сигнализирован при заполнении очереди.
        SubmitInfo.signalSemaphoreCount = 1;
        SubmitInfo.pSignalSemaphores = &QueueCompleteSemaphores[CurrentFrame];

        // Семафор ожидания гарантирует, что операция не может начаться до тех пор, пока изображение не будет доступно.
        SubmitInfo.waitSemaphoreCount = 1;
        SubmitInfo.pWaitSemaphores = &ImageAvailableSemaphores[CurrentFrame];
    } else {
        SubmitInfo.signalSemaphoreCount = 0;
        SubmitInfo.waitSemaphoreCount = 0;
    }

    // Каждый семафор ожидает завершения соответствующего этапа конвейера. Соотношение 1:1.
    // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT предотвращает выполнение последующих 
    // записей вложений цвета до тех пор, пока не поступит сигнал семафора (т. е. за раз будет представлен один кадр)
    VkPipelineStageFlags flags[1] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    SubmitInfo.pWaitDstStageMask = flags;

    VkResult result = vkQueueSubmit(
        device.GraphicsQueue,
        1,
        &SubmitInfo,
        InFlightFences[CurrentFrame]);
    if (result != VK_SUCCESS) {
        MERROR("vkQueueSubmit не дал результата: %shader", VulkanResultString(result, true));
        return false;
    }

    VulkanCommandBufferUpdateSubmitted(CommandBuffer);
    // Отправка в конечную очередь

    // Для отсчёта времени дождитесь завершения очереди.
    // Это даёт точное представление о том, сколько времени займёт рендеринг, включая время, отведённое на фактическую очередь работ.
    // ЗАДАЧА: возможно, здесь вместо этого стоит использовать семафор для мониторинга.
    // vkWaitForFences(Device.LogicalDevice, 1, &InFlightFences[CurrentFrame], true, UINT64_MAX);

    return true;
}

bool VulkanAPI::Present(const FrameData &rFrameData)
{
    // Верните изображение в цепочку обмена для отображения.
    VkPresentInfoKHR PresentInfo {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &QueueCompleteSemaphores[CurrentFrame],
        .swapchainCount = 1,
        .pSwapchains = &swapchain.handle,
        .pImageIndices = &ImageIndex,
        .pResults = nullptr
    };

    // HACK: Ожидая в очереди передачи, мы по какой-то причине избегаем ошибки сегментации. 
    // Это не должно быть необходимо, поскольку _должно_ ожидать семафоры pWaitSemaphores, 
    // о которых _должно_ сообщать завершение очереди после отправки. 
    // И, как ни странно, это именно очередь _передачи_, хотя для представления здесь используется текущая очередь.
    // ЗАДАЧА:Чтобы разобраться, нужно изучить этот вопрос немного глубже.
    vkQueueWaitIdle(device.TransferQueue);
    VkResult result = vkQueuePresentKHR(device.PresentQueue, &PresentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Цепочка обмена устарела, неоптимальна или произошло изменение размера буфера кадра. Запустите пересоздание цепочки обмена.
        swapchain.Recreate(this, FramebufferWidth, FramebufferHeight);
        MDEBUG("Цепочка обмена пересоздана, поскольку цепочка обмена устарела или неоптимальна.");
    } else if (result != VK_SUCCESS) {
        MFATAL("Не удалось представить изображение цепочки обмена!");
    }

    // Увеличьте (и выполните цикл) индекс.
    CurrentFrame = (CurrentFrame + 1) % swapchain.MaxFramesInFlight;

    return true;
}

void VulkanAPI::SetViewport(const Rect2D &rect)
{
    VkViewport viewport;
    viewport.x = rect.x;
    viewport.y = rect.y;
    viewport.width = rect.z;
    viewport.height = rect.w;
    viewport.minDepth = 0.F;
    viewport.maxDepth = 1.F;

    VulkanCommandBuffer& CommandBuffer = GraphicsCommandBuffers[ImageIndex];

    vkCmdSetViewport(CommandBuffer.handle, 0, 1, &viewport);
}

void VulkanAPI::ViewportReset()
{
    // Просто установите текущий прямоугольник области просмотра.
    SetViewport(ViewportRect);
}

void VulkanAPI::SetScissor(const Rect2D &rect)
{
    VkRect2D scissor;
    scissor.offset.x = rect.x;
    scissor.offset.y = rect.y;
    scissor.extent.width = rect.z;
    scissor.extent.height = rect.w;

    VulkanCommandBuffer& CommandBuffer = GraphicsCommandBuffers[ImageIndex];

    vkCmdSetScissor(CommandBuffer.handle, 0, 1, &scissor);
}

void VulkanAPI::ScissorReset()
{
    // Просто установите текущий прямоугольник ножниц.
    SetScissor(ScissorRect);
}

void VulkanAPI::SetStencilTestEnabled(bool enabled)
{
    VulkanCommandBuffer& CommandBuffer = GraphicsCommandBuffers[ImageIndex];

    if (device.supportFlags & VulkanDevice::NativeDynamicStateBit) {
        vkCmdSetStencilTestEnable(CommandBuffer.handle, (VkBool32)enabled);
    } else if (device.supportFlags & VulkanDevice::DynamicStateBit) {
        vkCmdSetStencilTestEnableEXT(CommandBuffer.handle, (VkBool32)enabled);
    } else {
        MFATAL("VulkanAPI::SetStencilTestEnabled нельзя использовать на устройстве без поддержки динамического состояния.");
    }
}

void VulkanAPI::SetDepthTestEnabled(bool enabled)
{
    VulkanCommandBuffer& CommandBuffer = GraphicsCommandBuffers[ImageIndex];

    if (device.supportFlags & VulkanDevice::NativeDynamicStateBit) {
        vkCmdSetDepthTestEnable(CommandBuffer.handle, (VkBool32)enabled);
    } else if (device.supportFlags & VulkanDevice::DynamicStateBit) {
        vkCmdSetDepthTestEnableEXT(CommandBuffer.handle, (VkBool32)enabled);
    } else {
        MFATAL("VulkanAPI::SetDepthTestEnabled нельзя использовать на устройстве без поддержки динамического состояния.");
    }
}

void VulkanAPI::SetStencilReference(u32 reference)
{
    VulkanCommandBuffer& CommandBuffer = GraphicsCommandBuffers[ImageIndex];
    vkCmdSetStencilReference(CommandBuffer.handle, VK_STENCIL_FACE_FRONT_AND_BACK, reference);
}

static VkStencilOp GetStencilOp(RendererStencilOp op) {
    switch (op) {
        case RendererStencilOp::Keep:
            return VK_STENCIL_OP_KEEP;
        case RendererStencilOp::Zero:
            return VK_STENCIL_OP_ZERO;
        case RendererStencilOp::Replace:
            return VK_STENCIL_OP_REPLACE;
        case RendererStencilOp::IncrementAndClamp:
            return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
        case RendererStencilOp::DecrementAndClamp:
            return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
        case RendererStencilOp::IncrementAandWrap:
            return VK_STENCIL_OP_DECREMENT_AND_WRAP;
        case RendererStencilOp::DecrementAndWrap:
            return VK_STENCIL_OP_DECREMENT_AND_WRAP;
        case RendererStencilOp::Invert:
            return VK_STENCIL_OP_INVERT;
        default:
            MWARN("Неподдерживаемая операция трафарета, по умолчанию используется «сохранить».");
            return VK_STENCIL_OP_KEEP;
    }
}

static VkCompareOp GetCompareOp(RendererCompareOp op) {
    switch (op) {
        case RendererCompareOp::Never:
            return VK_COMPARE_OP_NEVER;
        case RendererCompareOp::Less:
            return VK_COMPARE_OP_LESS;
        case RendererCompareOp::Equal:
            return VK_COMPARE_OP_EQUAL;
        case RendererCompareOp::LessOrEqual:
            return VK_COMPARE_OP_LESS_OR_EQUAL;
        case RendererCompareOp::Greater:
            return VK_COMPARE_OP_GREATER;
        case RendererCompareOp::NotEqual:
            return VK_COMPARE_OP_NOT_EQUAL;
        case RendererCompareOp::GreaterOrEqual:
            return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case RendererCompareOp::Always:
            return VK_COMPARE_OP_ALWAYS;
        default:
            MWARN("Неподдерживаемая операция сравнения, используется «всегда».");
            return VK_COMPARE_OP_ALWAYS;
    }
}

void VulkanAPI::SetStencilOp(RendererStencilOp FailOp, RendererStencilOp PassOp, RendererStencilOp DepthFailOp, RendererCompareOp CompareOp)
{
    VulkanCommandBuffer& CommandBuffer = GraphicsCommandBuffers[ImageIndex];

    if (device.supportFlags & VulkanDevice::NativeDynamicStateBit) {
        vkCmdSetStencilOp(
            CommandBuffer.handle,
            VK_STENCIL_FACE_FRONT_AND_BACK,
            GetStencilOp(FailOp),
            GetStencilOp(PassOp),
            GetStencilOp(DepthFailOp),
            GetCompareOp(CompareOp));
    } else if (device.supportFlags & VulkanDevice::DynamicStateBit) {
        vkCmdSetStencilOpEXT(
            CommandBuffer.handle,
            VK_STENCIL_FACE_FRONT_AND_BACK,
            GetStencilOp(FailOp),
            GetStencilOp(PassOp),
            GetStencilOp(DepthFailOp),
            GetCompareOp(CompareOp));
    } else {
        MFATAL("VulkanAPI::SetStencilOp нельзя использовать на устройстве без поддержки динамического состояния.");
    }
}

void VulkanAPI::SetStencilCompareMask(u32 CompareMask)
{
    VulkanCommandBuffer& CommandBuffer = GraphicsCommandBuffers[ImageIndex];
    // Поддерживается начиная с версии Vulkan 1.0, поэтому нет необходимости проверять поддержку динамического состояния.
    vkCmdSetStencilCompareMask(CommandBuffer.handle, VK_STENCIL_FACE_FRONT_AND_BACK, CompareMask);
}

void VulkanAPI::SetStencilWriteMask(u32 WriteMask)
{
    VulkanCommandBuffer& CommandBuffer = GraphicsCommandBuffers[ImageIndex];
    // Поддерживается начиная с версии Vulkan 1.0, поэтому нет необходимости проверять поддержку динамического состояния.
    vkCmdSetStencilWriteMask(CommandBuffer.handle, VK_STENCIL_FACE_FRONT_AND_BACK, WriteMask);
}

void VulkanAPI::SetWinding(RendererWinding winding)
{
    VulkanCommandBuffer& CommandBuffer = GraphicsCommandBuffers[ImageIndex];

    VkFrontFace vkWinding = winding == RendererWinding::CounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
    if (device.supportFlags & VulkanDevice::NativeDynamicStateBit) {
        vkCmdSetFrontFace(CommandBuffer.handle, vkWinding);
    } else if (device.supportFlags & VulkanDevice::DynamicStateBit) {
        vkCmdSetFrontFaceEXT(CommandBuffer.handle, vkWinding);
    } else {
        MFATAL("VulkanAPI::SetWinding нельзя использовать на устройстве без поддержки динамического состояния.");
    }
}

bool VulkanAPI::RenderpassBegin(Renderpass* pass, RenderTarget& target)
{
    VulkanCommandBuffer& CommandBuffer = GraphicsCommandBuffers[ImageIndex];

    // Начало этапа рендеринга.
    auto VkRenderpass = (VulkanRenderpass*)pass->InternalData;

    Viewport* ActiveViewport = SystemsManager::GetRenderingSystem()->GetActiveViewport();

    VkRenderPassBeginInfo BeginInfo {
       .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = VkRenderpass->handle,
        .framebuffer = (VkFramebuffer)target.InternalFramebuffer,
        .clearValueCount = 0,
        .pClearValues = 0
    };
    BeginInfo.renderArea.offset.x = ActiveViewport->rect.x;
    BeginInfo.renderArea.offset.y = ActiveViewport->rect.y;
    BeginInfo.renderArea.extent.width = ActiveViewport->rect.width;
    BeginInfo.renderArea.extent.height = ActiveViewport->rect.height;

    // MTRACE("Рендерпасс '%s' использует фреймбуфер по адресу 0x%x", pass->name.c_str(), target.InternalFramebuffer);

    VkClearValue ClearValues[2]{};
    bool DoClearColour = (pass->ClearFlags & RenderpassClearFlag::ColourBuffer) != 0;
    if (DoClearColour) {
        MemorySystem::CopyMem(ClearValues[BeginInfo.clearValueCount].color.float32, pass->ClearColour.elements, sizeof(f32) * 4);
        BeginInfo.clearValueCount++;
    } else {
        // Если первое вложение цветное, всё равно добавьте значение прозрачности, но не тратьте время на копирование данных, так как они будут проигнорированы.
        // Это необходимо, потому что каждое вложение должно иметь значение прозрачности, даже если оно не используется.
        if (target.attachments[0].type == RenderTargetAttachmentType::Colour) {
            BeginInfo.clearValueCount++;
        }
    }

    bool DoClearDepth = (pass->ClearFlags & RenderpassClearFlag::DepthBuffer) != 0;
    bool DoClearStencil = (pass->ClearFlags & RenderpassClearFlag::StencilBuffer) != 0;
    if (DoClearDepth) {
        MemorySystem::CopyMem(ClearValues[BeginInfo.clearValueCount].color.float32, pass->ClearColour.elements, sizeof(f32) * 4);
        ClearValues[BeginInfo.clearValueCount].depthStencil.depth = VkRenderpass->depth;

        ClearValues[BeginInfo.clearValueCount].depthStencil.stencil = DoClearStencil ? VkRenderpass->stencil : 0;
        BeginInfo.clearValueCount++;
    } else {
        for (u32 i = 0; i < target.AttachmentCount; ++i) {
            if (target.attachments[i].type & RenderTargetAttachmentType::Depth || target.attachments[i].type & RenderTargetAttachmentType::Stencil) {
                // Если есть привязка глубины/трафарета, обязательно добавьте количество очисток, но не беспокойтесь о копировании данных.
                BeginInfo.clearValueCount++;
            }
        }
    }

    BeginInfo.pClearValues = BeginInfo.clearValueCount > 0 ? ClearValues : 0;

    vkCmdBeginRenderPass(CommandBuffer.handle, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    CommandBuffer.state = COMMAND_BUFFER_STATE_IN_RENDER_PASS;
#ifdef _DEBUG
    FVec3 rgb { Math::RandomInRange(0.F, 1.F), Math::RandomInRange(0.F, 1.F), Math::RandomInRange(0.F, 1.F) };
#endif
    BeginDebugLabel(pass->name.c_str(), rgb);

    return true;
}

bool VulkanAPI::RenderpassEnd(Renderpass* pass)
{
    VulkanCommandBuffer& CommandBuffer = GraphicsCommandBuffers[ImageIndex];

    // Завершение рендеринга.
    vkCmdEndRenderPass(CommandBuffer.handle);
    VK_END_DEBUG_LABEL(this, CommandBuffer.handle);
    CommandBuffer.state = COMMAND_BUFFER_STATE_RECORDING;
    return true;
}

void VulkanAPI::Load(const u8* pixels, Texture *texture)
{
    // Создание внутренних данных.
    u32 ImageSize = texture->width * texture->height * texture->ChannelCount * (texture->type == TextureType::Cube ? 6 : texture->ArraySize);

    // ПРИМЕЧАНИЕ: Здесь много предположений, для разных типов текстур потребуются разные параметры.
    VulkanImage::Config config = { 
        .VkAPI           = this,
        .name            = texture->name,
        .width           = texture->width,
        .height          = texture->height,
        .CreateView      = true,
        .format          = VK_FORMAT_R8G8B8A8_UNORM, // ПРИМЕЧАНИЕ: Предполагается, что на канал приходится 8 бит. ЗАДАЧА: подумать о выборе формата для изображений с линейным цветовым пространсвом и без.
        .tiling          = VK_IMAGE_TILING_OPTIMAL,
        .usage           = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .MemoryFlags     = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .ViewAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
        .LayerCount      = texture->ArraySize,
        .type            = texture->type,
        .MipLevels       = texture->MipLevels
    };

    texture->data = new VulkanImage(config);

    TextureWriteData(texture, 0, ImageSize, pixels, false);
    texture->generation++;
}

static VkFormat ChannelCountToFormat(u8 ChannelCount, VkFormat DefaultFormat) {
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
        usage = (VkImageUsageFlagBits)(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
        ImageFormat = device.DepthFormat;
    } else {
        usage = static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        ImageFormat = ChannelCountToFormat(texture->ChannelCount, VK_FORMAT_R8G8B8A8_UNORM);
    }

    VulkanImage::Config config = {
        .VkAPI               = this,
        .name                = texture->name,
        .width               = texture->width,
        .height              = texture->height,
        .CreateView          = true,
        .format              = ImageFormat,
        .tiling              = VK_IMAGE_TILING_OPTIMAL,
        .usage               = (VkImageUsageFlags)usage,
        .MemoryFlags         = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .ViewAspectFlags     = (VkImageAspectFlags)aspect,
        .LayerCount          = texture->ArraySize,
        .type                = texture->type,
        .MipLevels           = texture->MipLevels
    };

    texture->data = new VulkanImage(config);
    texture->generation++;
}

void VulkanAPI::TextureResize(Texture *texture, u32 NewWidth, u32 NewHeight)
{
    if (texture && texture->data) {
        auto image = (VulkanImage*)texture->data;
        // Изменение размера на самом деле просто разрушает старое изображение и создает новое. 
        // Данные не сохраняются, поскольку не существует надежного способа сопоставить старые 
        // данные с новыми, поскольку объем данных различается.
        image->Destroy(this);

        VkFormat ImageFormat = ChannelCountToFormat(texture->ChannelCount, VK_FORMAT_R8G8B8A8_UNORM);

        // Пересчитайте MIP-уровни, если они отличаются от 1.
        if (texture->MipLevels > 1) {
            // Пересчитайте количество уровней. 
            // Количество MIP-уровней рассчитывается следующим образом: 
            // сначала берётся наибольший размер (ширина или высота), 
            // затем вычисляется, сколько раз это число делится на 2, 
            // затем берётся минимальное значение (округляя в меньшую сторону) и добавляется 1, 
            // чтобы обозначить базовый уровень. В результате всегда получается значение не менее 1.
            texture->MipLevels = (u8)(Math::floor(Math::log2(MMAX(NewWidth, NewHeight))) + 1);
        }

        // ЗАДАЧА: здесь много предположений, разные типы текстур потребуют разных опций.
        VulkanImage::Config config = { this };
        config.name                = texture->name;
        config.width               = NewWidth;
        config.height              = NewHeight;
        config.CreateView          = true;
        config.format              = ImageFormat;
        config.tiling              = VK_IMAGE_TILING_OPTIMAL;
        config.usage               = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        config.MemoryFlags         = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        config.ViewAspectFlags     = VK_IMAGE_ASPECT_COLOR_BIT;
        config.LayerCount          = texture->ArraySize;
        config.type                = texture->type;
        config.MipLevels           = texture->MipLevels;

        image->Create(config);

        texture->generation++;
    }
}

void VulkanAPI::TextureWriteData(Texture *texture, u32 offset, u32 size, const u8 *pixels, bool IncludeInFrameWorkload)
{
    VkFormat ImageFormat = ChannelCountToFormat(texture->ChannelCount, VK_FORMAT_R8G8B8A8_UNORM);

    // Промежуточный буфер    209761968
    u64 StagingOffset = 0; // 256000000
    staging.Allocate(size, StagingOffset);
    RenderBufferLoadRange(staging, StagingOffset, size, pixels, true); 

    VulkanCommandBuffer TempBuffer;
    VulkanCommandBufferAllocateAndBeginSingleUse(this, device.GraphicsCommandPool, TempBuffer);

    auto image = (VulkanImage*)texture->data;
    // Переведите макет от текущего к оптимальному для получения данных.
    image->TransitionLayout(this, TempBuffer, ImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Скопируйте данные из буфера.
    image->CopyFromBuffer(this, ((VulkanBuffer*)staging.data)->handle, StagingOffset, TempBuffer);
    if (texture->MipLevels <= 1 || !image->GenerateMipMaps(this, TempBuffer)) {
        // Если генерация MIP не требуется или не удалась, вернитесь к обычному переходу.
        // Переход от оптимального для приема данных к оптимальному макету, доступному только для чтения шейдеров.
        image->TransitionLayout(this, TempBuffer, ImageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    

    VulkanCommandBufferEndSingleUse(this, device.GraphicsCommandPool, TempBuffer, device.GraphicsQueue);

    texture->generation++;
}

void VulkanAPI::TextureReadData(Texture *texture, u32 offset, u32 size, void **OutMemory)
{
    auto image = (VulkanImage*)texture->data;

    VkFormat ImageFormat = ChannelCountToFormat(texture->ChannelCount, VK_FORMAT_R8G8B8A8_UNORM);

    // Создайте промежуточный буфер и загрузите в него данные.
    RenderBuffer staging{"renderbuffer_texture_read_staging", RenderBufferType::Read, RenderBuffer::AllocationType::None, size};
    if (!RenderBufferCreate(staging)) {
        MERROR("Не удалось создать промежуточный буфер для чтения текстуры.");
        return;
    }

    RenderBufferBind(staging, 0);

    VulkanCommandBuffer TempBuffer;
    auto pool = device.GraphicsCommandPool;
    auto queue = device.GraphicsQueue;
    VulkanCommandBufferAllocateAndBeginSingleUse(this, pool, TempBuffer);

    // ПРИМЕЧАНИЕ: переход к VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    // Переведите макет из текущего состояния в оптимальное для выдачи данных.
    image->TransitionLayout(this, TempBuffer, ImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    // Копируем данные в буфер.
    image->CopyToBuffer(this, ((VulkanBuffer*)staging.data)->handle, TempBuffer);

    if (texture->MipLevels <= 1 || !image->GenerateMipMaps(this, TempBuffer)) {
        // Если генерация мип-объектов не требуется или завершается неудачей, используется обычный переход.
        // Переход от оптимального варианта для получения данных к оптимальному варианту компоновки только для чтения шейдера.
        image->TransitionLayout(this, TempBuffer, ImageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    VulkanCommandBufferEndSingleUse(this, pool, TempBuffer, queue);

    if (!RenderBufferRead(staging, offset, size, OutMemory)) {
        MERROR("Ошибка RenderBufferRead.");
    }

    RenderBufferUnbind(staging);
    RenderBufferDestroy(staging);
}

void VulkanAPI::TextureReadPixel(Texture *texture, u32 x, u32 y, u8 **OutRgba)
{
    auto image = (VulkanImage*)texture->data;

    auto ImageFormat = ChannelCountToFormat(texture->ChannelCount, VK_FORMAT_R8G8B8A8_UNORM);

    // ЗАДАЧА: Создавать буфер каждый раз не очень хорошо. Можно оптимизировать это, создав буфер один раз и просто повторно используя его.

    // Создайте промежуточный буфер и загрузите в него данные.
    RenderBuffer staging{"renderbuffer_texture_read_staging", RenderBufferType::Read, RenderBuffer::AllocationType::None, sizeof(u8) * 4};
    if (!RenderBufferCreate(staging)) {
        MERROR("Не удалось создать промежуточный буфер для чтения пикселей текстуры.");
        return;
    }

    RenderBufferBind(staging, 0);

    VulkanCommandBuffer TempBuffer;
    auto pool = device.GraphicsCommandPool;
    auto queue = device.GraphicsQueue;
    VulkanCommandBufferAllocateAndBeginSingleUse(this, pool, TempBuffer);

    // ПРИМЕЧАНИЕ: переход к VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    // Переведите макет из текущего состояния в оптимальное для передачи данных.
    image->TransitionLayout(this, TempBuffer, ImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    // Скопируйте данные в буфер.
    image->CopyPixelToBuffer(this, ((VulkanBuffer*)staging.data)->handle, x, y, TempBuffer);

    // Переход от оптимальной компоновки для чтения данных к оптимальной компоновке, предназначенной только для чтения шейдеров.
    image->TransitionLayout(this, TempBuffer, ImageFormat, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VulkanCommandBufferEndSingleUse(this, pool, TempBuffer, queue);

    if (!RenderBufferRead(staging, 0, sizeof(u8) * 4, (void**)OutRgba)) {
        MERROR("Ошибка RenderBufferRead.");
    }

    RenderBufferUnbind(staging);
    RenderBufferDestroy(staging);
}

void *VulkanAPI::TextureCopyData(const Texture* texture)
{
    auto& data = *(VulkanImage*)texture->data;
    return (new VulkanImage(data));
}

void VulkanAPI::Unload(Texture *texture)
{
    vkDeviceWaitIdle(device.LogicalDevice);

    if (texture->data) {
        auto data = (VulkanImage*)texture->data;
        data->Destroy(this);

        delete data;
        data = nullptr;
    }
    //MemorySystem::ZeroMem(texture, sizeof(Texture));
}

bool VulkanAPI::CreateGeometry(Geometry *geometry)
{
    // ПРИМЕЧАНИЕ: этому конкретному растеризатору не нужно ничего делать.
    return true;
}

#pragma region Shader

bool VulkanAPI::ShaderCreate(Shader* shader, const ShaderConfig& config, Renderpass* renderpass)
{
    // Проверить поддержку этапа.
    const u32 StageConfigCount = config.StageConfigs.Length(); 
    for (u8 i = 0; i < StageConfigCount; ++i) {
        switch (config.StageConfigs[i].stage) {
            case ShaderStage::Fragment:
            case ShaderStage::Vertex:
                break;
            case ShaderStage::Geometry:
                MWARN("VulkanAPI::LoadShader: VK_SHADER_STAGE_GEOMETRY_BIT установлен, но еще не поддерживается.");
                break;
            case ShaderStage::Compute:
                MWARN("VulkanAPI::LoadShader: SHADER_STAGE_COMPUTE установлен, но еще не поддерживается.");
                break;
            default:
                MERROR("Неподдерживаемый тип этапа: %d", config.StageConfigs[i].stage);
                break;
        }
    }

    // ЗАДАЧА: Максимальное количество должно быть настраиваемым или, возможно, иметь долгосрочную поддержку изменения размера буфера.
    const u64 TotalBufferSize = shader->GlobalUboStride + (shader->UboStride * VULKAN_MAX_MATERIAL_COUNT);  // global + (locals)
    RenderBuffer rbuf { "renderbuffer_global_uniform", RenderBufferType::Uniform, RenderBuffer::AllocationType::FreeList, TotalBufferSize };

    // Настройка шейдера вулкан.
    shader->ShaderData = new VulkanShader(config.MaxInstances);
    auto vkShader = shader->ShaderData;

    vkShader->renderpass = (VulkanRenderpass*)renderpass->InternalData;
    vkShader->StageCount = config.StageConfigs.Length();
    
    // Необходимо максимум 2 набора дескрипторов: один для глобальных переменных, другой — для отдельных экземпляров.
    // Обратите внимание, что это также может означать наличие только одного (или, возможно, ни одного) набора.
    // vkShader->DescriptorSetCount = 0;
    bool HasGlobal = shader->GlobalUniformCount > 0 || shader->GlobalUniformSamplerCount > 0;
    bool HasInstance = shader->InstanceUniformCount > 0 || shader->InstanceUniformSamplerCount > 0;
    u8 SetCount = 0;
    if (HasGlobal) {
        vkShader->DescriptorSets[SetCount].SamplerBindingIndexStart = INVALID::U8ID;
        SetCount++;
    }

    if (HasInstance) {
        vkShader->DescriptorSets[SetCount].SamplerBindingIndexStart = INVALID::U8ID;
        SetCount++;
    }

    // Вычислите общее количество необходимых дескрипторов.
    u32 FrameCount = swapchain.ImageCount;
    // 1 набор глобальных переменных * количество кадров + x сэмплеров на экземпляр, на кадр.
    u32 MaxSamplerCount = (shader->GlobalUniformSamplerCount * FrameCount) + (config.MaxInstances * shader->InstanceUniformSamplerCount * FrameCount);
    // 1 глобальная переменная (1 * количество кадров) + 1 на экземпляр, на кадр.
    u32 MaxUboCount = FrameCount + (config.MaxInstances * FrameCount);
    // Общее количество необходимых дескрипторов.
    u32 MaxDescriptorAllocateCount = MaxUboCount + MaxSamplerCount;

    vkShader->MaxDescriptorSetCount = MaxDescriptorAllocateCount;

    // На данный момент шейдеры будут иметь только эти два типа пулов дескрипторов.
    if (MaxUboCount > 0) {
        vkShader->PoolSizes[vkShader->PoolSizeCount] = (VkDescriptorPoolSize){VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MaxUboCount};
        vkShader->PoolSizeCount++;
    }
    if (MaxSamplerCount > 0) {
        vkShader->PoolSizes[vkShader->PoolSizeCount] = (VkDescriptorPoolSize){VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MaxSamplerCount};
        vkShader->PoolSizeCount++;
    }

    // Конфигурация глобального набора дескрипторов.
    if (HasGlobal) {
        // Глобальная конфигурация набора дескрипторов.
        auto& SetConfig = vkShader->DescriptorSets[vkShader->DescriptorSetCount];

        // Общее количество привязок составляет 1 UBO для глобальных данных (если необходимо), плюс количество глобальных сэмплеров.
        // Сейчас это выделяется динамически.
        u32 UboCount = shader->GlobalUniformCount ? 1 : 0;
        SetConfig.BindingCount = UboCount + shader->GlobalUniformSamplerCount;
        CreateArray(SetConfig.bindings, sizeof(VkDescriptorSetLayoutBinding) * SetConfig.BindingCount, true);

        // При наличии глобального UBO-привязки он является первым.
        u8 GlobalBindingIndex = 0;
        if (shader->GlobalUniformCount > 0) {
            SetConfig.bindings[GlobalBindingIndex].binding = GlobalBindingIndex;;
            SetConfig.bindings[GlobalBindingIndex].descriptorCount = 1; // ПРИМЕЧАНИЕ: весь UBO является одним связыванием.
            SetConfig.bindings[GlobalBindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            SetConfig.bindings[GlobalBindingIndex].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            GlobalBindingIndex++;
        }

        // Установите индекс, с которого начинаются привязки семплеров. Он будет использоваться позже, 
        // чтобы определить, с какого индекса начинать привязку дескрипторов семплера.
        SetConfig.SamplerBindingIndexStart = shader->GlobalUniformCount ? 1 : 0;

        // Добавьте привязку для каждого настроенного сэмплера.  
        if (shader->GlobalUniformSamplerCount > 0) {
            for (u32 i = 0; i < shader->GlobalUniformSamplerCount; ++i) {
                // Ищите по индексам сэмплеров, собранным выше.  
                auto& u = config.uniforms[shader->GlobalSamplerIndices[i]];
                SetConfig.bindings[GlobalBindingIndex].binding = GlobalBindingIndex;
                SetConfig.bindings[GlobalBindingIndex].descriptorCount = MMAX(u.ArrayLength, 1);  // Обрабатывайте как массив или как одну текстуру в зависимости от переданных данных.
                SetConfig.bindings[GlobalBindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                SetConfig.bindings[GlobalBindingIndex].stageFlags = VK_SHADER_STAGE_ALL;
                GlobalBindingIndex++;
            }
        }

        // Увеличьте счётчик набора.
        vkShader->DescriptorSetCount++;
    }
    
    // Если используются униформы экземпляра, добавьте дескрипторный набор UBO.
    if (HasInstance) {
        auto& SetConfig = vkShader->DescriptorSets[vkShader->DescriptorSetCount];

        // Общее количество привязок составляет 1 UBO на экземпляр (при необходимости), плюс количество семплеров на экземпляр.
        // Теперь это выделяется динамически.
        u32 UboCount = shader->InstanceUniformCount ? 1 : 0;
        SetConfig.BindingCount = UboCount + shader->InstanceUniformSamplerCount;
        CreateArray(SetConfig.bindings, sizeof(VkDescriptorSetLayoutBinding) * SetConfig.BindingCount, true);

        // Привязка экземпляра UBO идет первой, если она присутствует.
        u8 InstanceBindingIndex = 0;
        if (shader->InstanceUniformCount > 0) {
            SetConfig.bindings[InstanceBindingIndex].binding = InstanceBindingIndex;
            SetConfig.bindings[InstanceBindingIndex].descriptorCount = 1;
            SetConfig.bindings[InstanceBindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            SetConfig.bindings[InstanceBindingIndex].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            InstanceBindingIndex++;
        }

        // Установите индекс, с которого начинаются привязки сэмплера. Это будет использовано позже, чтобы определить, с какого индекса начинать привязку дескрипторов сэмплера.
        SetConfig.SamplerBindingIndexStart = shader->InstanceUniformCount ? 1 : 0;

        // Добавьте привязку для каждого настроенного сэмплера.
        if (shader->InstanceUniformSamplerCount > 0) {
            for (u32 i = 0; i < shader->InstanceUniformSamplerCount; ++i) {
                // Посмотрите по индексам выборки, собранным выше.
                auto& u = config.uniforms[shader->InstanceSamplerIndices[i]];
                SetConfig.bindings[InstanceBindingIndex].binding = InstanceBindingIndex;
                SetConfig.bindings[InstanceBindingIndex].descriptorCount = MMAX(u.ArrayLength, 1);  // Обрабатывайте либо как массив, либо как отдельную текстуру, в зависимости от того, что передано.
                SetConfig.bindings[InstanceBindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                SetConfig.bindings[InstanceBindingIndex].stageFlags = VK_SHADER_STAGE_ALL;
                InstanceBindingIndex++;
            }
        }

        // Увеличить установленный счетчик.
        vkShader->DescriptorSetCount++;
    }

    // Сохраните копию режима отбраковки.
    vkShader->CullMode = config.CullMode;

    // Сохраните копию типов топологии.
    shader->TopologyTypes = config.TopologyTypes;

    return true;
}

void VulkanAPI::ShaderDestroy(Shader *shader)
{
    if (!shader || !shader->ShaderData) { MERROR("VulkanAPI::ShaderDestroy требуется действительный указатель на шейдер."); return; }

    auto vkShader = (VulkanShader*)shader->ShaderData;
    VkDevice& LogicalDevice = device.LogicalDevice;

    // Макеты набора дескрипторов.
    for (u32 i = 0; i < vkShader->DescriptorSetCount; ++i) {
        if (vkShader->DescriptorSetLayouts[i]) {
            DestroyArray(vkShader->DescriptorSets->bindings, vkShader->DescriptorSets->BindingCount);
            vkDestroyDescriptorSetLayout(LogicalDevice, vkShader->DescriptorSetLayouts[i], allocator);
            vkShader->DescriptorSetLayouts[i] = 0;
        }
    }

    // Пул дескрипторов
    if (vkShader->DescriptorPool) {
        vkDestroyDescriptorPool(LogicalDevice, vkShader->DescriptorPool, allocator);
    }

    // Полностью сбросить состояния экземпляра.
    DestroyArray(vkShader->InstanceStates, vkShader->MaxInstances);

    // Однородный буфер.
    RenderBufferUnmapMemory(vkShader->UniformBuffer, 0, VK_WHOLE_SIZE);
    vkShader->MappedUniformBufferBlock = nullptr;
    RenderBufferDestroy(vkShader->UniformBuffer);

    // Конвееры
    for (u32 i = 0; i < VulkanTopology::Class::MAX; ++i) {
        if (vkShader->pipelines[i]) {
            vkShader->pipelines[i]->Destroy(this);
        }
        if (vkShader->WireframePipelines && vkShader->WireframePipelines[i]) {
            vkShader->WireframePipelines[i]->Destroy(this);
        }
    }

    // Шейдерные модули
    for (u32 i = 0; i < vkShader->StageCount; ++i) {
        vkDestroyShaderModule(LogicalDevice, vkShader->stages[i].handle, allocator);
    }

    // Освободите внутреннюю память данных.
    delete vkShader; // MemorySystem::Free(VkShader, sizeof(VulkanShader), Memory::Renderer);
    shader->ShaderData = nullptr;
}

bool VulkanAPI::ShaderCreateModulesAndPipelines(Shader *shader)
{
    VulkanShader* vkShader = shader->ShaderData;

    bool HasError = false;

    // Поддерживается только динамическая топология. Создавайте по одному конвейеру для каждого класса топологии.
    // Если это не поддерживается, возможно, стоит использовать другой бэкенд.
    u32 PipelineCount = 3;

    // Создайте временный массив для хранения конвейеров. Они будут находиться в нем до завершения процесса загрузки
    // — на случай, если этот метод будет вызван в ходе перезагрузки. Это гарантирует, что текущие конвейеры продолжат 
    // корректно функционировать вплоть до момента, когда новая загрузка будет успешно завершена и система будет готова к работе.
    VulkanPipeline* NewPipelines = AllocateTemp<VulkanPipeline>(sizeof(VulkanPipeline) * PipelineCount, true);
    // То же самое касается WireframePipelines, если это необходимо.
    VulkanPipeline* NewWireframePipelines = nullptr;
    if (vkShader->WireframePipelines) {
        NewWireframePipelines = AllocateTemp<VulkanPipeline>(sizeof(VulkanPipeline) * PipelineCount, true);
    }

    // Создайте модуль для каждого этапа.
    VulkanShaderStage* NewStages = nullptr; 
    CreateArray(NewStages, sizeof(VulkanShaderStage) * VulkanShaderConstants::MaxStages);
    for (u32 i = 0; i < vkShader->StageCount; ++i) {
        if (!CreateShaderModule(shader, shader->StageConfigs[i], NewStages[i])) {
            MERROR("Не удалось создать модуль шейдера %s для «%s». Шейдер будет уничтожен.", shader->StageConfigs[i].filename.c_str(), shader->name.c_str());
            HasError = true;
            return !HasError; 
        }
    }

    // Область просмотра / область отсечения по умолчанию; могут быть динамически переопределены.
    VkViewport viewport {
        .y = (f32)FramebufferHeight,
        .width = (f32)FramebufferWidth,
        .height = -(f32)FramebufferHeight,
        .maxDepth = 1.F
    };

    // Scissor
    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = FramebufferWidth;
    scissor.extent.height = FramebufferHeight;

    VkPipelineShaderStageCreateInfo StageCreateInfos[VulkanShaderConstants::MaxStages]{};
    for (u32 i = 0; i < vkShader->StageCount; ++i) {
        StageCreateInfos[i] = NewStages[i].ShaderStageCreateInfo;
    }

    // Выполните итерацию и настройте или создайте по одному конвейеру для каждого класса. Элементы со значением null пропускаются.
    for (u32 i = 0; i < PipelineCount; ++i) {
        if (!vkShader->pipelines[i]) {
            continue;
        }

        // Убедитесь, что поддерживаемые типы указаны в конвейерах для временных массивов.
        NewPipelines[i].SupportedTopologyTypes = vkShader->pipelines[i]->SupportedTopologyTypes;
        if (vkShader->WireframePipelines) {
            NewWireframePipelines[i].SupportedTopologyTypes = vkShader->WireframePipelines[i]->SupportedTopologyTypes;
        }
        
        Range PushConstantRange{0,  shader->LocalUboStride};
        // Удалите флаг каркасного режима, если он установлен.
        ShaderFlagBits flags = shader->flags;
        flags &= ~(Shader::Wireframe);
        VulkanPipeline::Config PipelineConfig{
            .name                   = shader->name,
            .renderpass             = vkShader->renderpass,
            .stride                 = shader->AttributeStride,
            .AttributeCount         = shader->attributes.Length(),
            .attributes             = vkShader->attributes,
            .DescriptorSetLayoutCount = vkShader->DescriptorSetCount,
            .DescriptorSetLayouts = vkShader->DescriptorSetLayouts,
            .StageCount             = vkShader->StageCount,
            .stages                 = StageCreateInfos,
            .viewport               = viewport,
            .scissor                = scissor,
            .CullMode               = vkShader->CullMode,
            .ShaderFlags = flags,
            // ПРИМЕЧАНИЕ: Для push-константы всегда используется один блок.
            .PushConstantRangeCount = 1,
            .PushConstantRanges = &PushConstantRange,
            .TopologyTypes = shader->TopologyTypes
        };

        bool PipelineResult = NewPipelines[i].Create(this, PipelineConfig);

        // Создайте каркасную версию.
        if (PipelineResult && NewWireframePipelines) {
            // Используйте ту же конфигурацию, но убедитесь, что установлен флаг каркасного режима (wireframe).
            PipelineConfig.ShaderFlags |= Shader::Wireframe;
            PipelineResult = NewWireframePipelines[i].Create(this, PipelineConfig);
        }

        if (!PipelineResult) {
            MERROR("Не удалось загрузить графический конвейер для шейдера: «%s».", shader->name.c_str());
            HasError = true;
            break;
        }
    }

    // В случае сбоя выполнить очистку.
    if (HasError) {
        for (u32 i = 0; i < PipelineCount; ++i) {
            NewPipelines[i].Destroy(this);
            if (NewWireframePipelines) {
                NewWireframePipelines[i].Destroy(this);
            }
        }
        for (u32 i = 0; i < vkShader->StageCount; ++i) {
            vkDestroyShaderModule(device.LogicalDevice, NewStages[i].handle, allocator);
        }
        return !HasError;
    }

    // В случае успеха демонтируйте старые трубопроводы и перенесите на их место новые.
    vkDeviceWaitIdle(device.LogicalDevice);
    for (u32 i = 0; i < PipelineCount; ++i) {
        if (vkShader->pipelines[i]) {
            vkShader->pipelines[i]->Destroy(this);
            MemorySystem::CopyMem(vkShader->pipelines[i], &NewPipelines[i], sizeof(VulkanPipeline));
        }
        if (NewWireframePipelines) {
            if (vkShader->WireframePipelines[i]) {
                vkShader->WireframePipelines[i]->Destroy(this);
                MemorySystem::CopyMem(vkShader->WireframePipelines[i], &NewWireframePipelines[i], sizeof(VulkanPipeline));
            }
        }
    }

    // Удалите старые модули шейдеров и скопируйте новые.
    for (u32 i = 0; i < vkShader->StageCount; ++i) {
        vkDestroyShaderModule(device.LogicalDevice, vkShader->stages[i].handle, allocator);
        MemorySystem::CopyMem(&vkShader->stages[i], &NewStages[i], sizeof(VulkanShaderStage));
    }

    return !HasError;
}

bool VulkanAPI::ShaderInitialize(Shader *shader)
{
    VkDevice LogicalDevice = device.LogicalDevice;
    VulkanShader* vkShader = shader->ShaderData;

    bool NeedsWireframe = (shader->flags & Shader::Wireframe) != 0;
    // Определите, поддерживает ли это реализация, и установите значение false, если нет.
    if (!device.features.fillModeNonSolid) {
        MINFO("Бэкенд рендерера не поддерживает fillModeNonSolid. Режим каркасного отображения (wireframe) невозможен, но был запрошен для шейдера «%s».", shader->name.c_str());
        NeedsWireframe = false;
    }

    // Статическая таблица поиска для наших типов -> Vulkan.
    static VkFormat* types = nullptr;
    static VkFormat t[11];
    if (!types) {
        t[ShaderAttribute::Float32]   =          VK_FORMAT_R32_SFLOAT;
        t[ShaderAttribute::Float32_2] =       VK_FORMAT_R32G32_SFLOAT;
        t[ShaderAttribute::Float32_3] =    VK_FORMAT_R32G32B32_SFLOAT;
        t[ShaderAttribute::Float32_4] = VK_FORMAT_R32G32B32A32_SFLOAT;
        t[ShaderAttribute::Int8]      =             VK_FORMAT_R8_SINT;
        t[ShaderAttribute::UInt8]     =             VK_FORMAT_R8_UINT;
        t[ShaderAttribute::Int16]     =            VK_FORMAT_R16_SINT;
        t[ShaderAttribute::UInt16]    =            VK_FORMAT_R16_UINT;
        t[ShaderAttribute::Int32]     =            VK_FORMAT_R32_SINT;
        t[ShaderAttribute::UInt32]    =            VK_FORMAT_R32_UINT;
        types = t;
    }

    // Атрибуты процесса
    const u32 AttributeCount = shader->attributes.Length();
    u32 offset = 0;
    for (u32 i = 0; i < AttributeCount; ++i) {
        // Настройте новый атрибут.
        VkVertexInputAttributeDescription attribute {
            .location = i,
            .binding = 0,
            .format = types[shader->attributes[i].type],
            .offset = offset
        };

        // Вставьте коллекцию атрибутов конфигурации и добавьте к шагу.
        vkShader->attributes[i] = attribute;

        offset += shader->attributes[i].size;
    }

    // Пул дескрипторов.
    VkDescriptorPoolCreateInfo PoolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,   // | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT
        .maxSets = vkShader->MaxDescriptorSetCount,
        .poolSizeCount = vkShader->PoolSizeCount,
        .pPoolSizes = vkShader->PoolSizes
    };

    // Создайте пул дескрипторов.
    VkResult result = vkCreateDescriptorPool(LogicalDevice, &PoolInfo, allocator, &vkShader->DescriptorPool);
    if (!VulkanResultIsSuccess(result)) {
        MERROR("VulkanAPI::ShaderInitialize — не удалось создать пул дескрипторов: '%s'", VulkanResultString(result, true));
        return false;
    }

    // Создайте макеты наборов дескрипторов.
    for (u32 i = 0; i < vkShader->DescriptorSetCount; ++i) {
        VkDescriptorSetLayoutCreateInfo LayoutInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = vkShader->DescriptorSets[i].BindingCount,
            .pBindings = vkShader->DescriptorSets[i].bindings
        };
        result = vkCreateDescriptorSetLayout(LogicalDevice, &LayoutInfo, allocator, &vkShader->DescriptorSetLayouts[i]);
        if (!VulkanResultIsSuccess(result)) {
            MERROR("VulkanAPI::ShaderInitialize — не удалось создать макет набора дескрипторов: '%s'", VulkanResultString(result, true));
            return false;
        }
    }

    // Поддерживается только динамическая топология. Создавайте один конвейер для каждого класса топологии.
    // Если это не поддерживается, возможно, следует использовать другой бэкенд.
    u32 PipelineCount = 3;

    // Создайте массив указателей на конвейеры, по одному на класс топологии. Нуль означает, что не поддерживается для этого шейдера.
    CreateArray(vkShader->pipelines, sizeof(void*) * PipelineCount, true);

    // Сделайте то же самое, что описано выше, но в виде каркасной модели.
    if (NeedsWireframe) {
        CreateArray(vkShader->WireframePipelines, sizeof(void*) * PipelineCount, true);
    } else {
        vkShader->WireframePipelines = nullptr;
    }

    // Создайте один конвейер на класс топологии.
    // Точечный класс.
    if (shader->TopologyTypes & PrimitiveTopology::PointList) {
        vkShader->pipelines[VulkanTopology::Point] = (VulkanPipeline*)MemorySystem::Allocate(sizeof(VulkanPipeline), Memory::Vulkan, true);
        // Установите поддерживаемые типы для этого класса.
        vkShader->pipelines[VulkanTopology::Point]->SupportedTopologyTypes |= PrimitiveTopology::Type::PointList;

        // Каркасные версии.
        if (NeedsWireframe) {
            vkShader->WireframePipelines[VulkanTopology::Line] = (VulkanPipeline*)MemorySystem::Allocate(sizeof(VulkanPipeline), Memory::Vulkan, true);
            // Установите поддерживаемые типы для этого класса.
            vkShader->WireframePipelines[VulkanTopology::Point]->SupportedTopologyTypes |= PrimitiveTopology::PointList;
        }
    }

    // Линейный класс.
    if (shader->TopologyTypes & PrimitiveTopology::LineList || shader->TopologyTypes & PrimitiveTopology::LineStrip) {
        vkShader->pipelines[VulkanTopology::Line] = (VulkanPipeline*)MemorySystem::Allocate(sizeof(VulkanPipeline), Memory::Vulkan, true);
        // Установите поддерживаемые типы для этого класса.
        vkShader->pipelines[VulkanTopology::Line]->SupportedTopologyTypes |= PrimitiveTopology::LineList;
        vkShader->pipelines[VulkanTopology::Line]->SupportedTopologyTypes |= PrimitiveTopology::LineStrip;

        // Каркасные версии.
        if (NeedsWireframe) {
            vkShader->WireframePipelines[VulkanTopology::Line] = (VulkanPipeline*)MemorySystem::Allocate(sizeof(VulkanPipeline), Memory::Vulkan, true);
            // Установите поддерживаемые типы для этого класса.
            vkShader->WireframePipelines[VulkanTopology::Line]->SupportedTopologyTypes |= PrimitiveTopology::LineList;
            vkShader->WireframePipelines[VulkanTopology::Line]->SupportedTopologyTypes |= PrimitiveTopology::LineStrip;
        }
    }

    // Треугольный класс.
    if (shader->TopologyTypes & PrimitiveTopology::TriangleList ||
        shader->TopologyTypes & PrimitiveTopology::TriangleStrip ||
        shader->TopologyTypes & PrimitiveTopology::TriangleFan) {
        vkShader->pipelines[VulkanTopology::Triangle] = (VulkanPipeline*)MemorySystem::Allocate(sizeof(VulkanPipeline), Memory::Vulkan, true);
        // Установите поддерживаемые типы для этого класса.
        vkShader->pipelines[VulkanTopology::Triangle]->SupportedTopologyTypes |= PrimitiveTopology::TriangleList;
        vkShader->pipelines[VulkanTopology::Triangle]->SupportedTopologyTypes |= PrimitiveTopology::TriangleStrip;
        vkShader->pipelines[VulkanTopology::Triangle]->SupportedTopologyTypes |= PrimitiveTopology::TriangleFan;

        // Каркасные версии.
        if (NeedsWireframe) {
            vkShader->WireframePipelines[VulkanTopology::Triangle] = (VulkanPipeline*)MemorySystem::Allocate(sizeof(VulkanPipeline), Memory::Vulkan, true);
            // Установите поддерживаемые типы для этого класса.
            vkShader->WireframePipelines[VulkanTopology::Triangle]->SupportedTopologyTypes |= PrimitiveTopology::TriangleList;
            vkShader->WireframePipelines[VulkanTopology::Triangle]->SupportedTopologyTypes |= PrimitiveTopology::TriangleStrip;
            vkShader->WireframePipelines[VulkanTopology::Triangle]->SupportedTopologyTypes |= PrimitiveTopology::TriangleFan;
        }
    }

    if(!ShaderCreateModulesAndPipelines(shader)) {
        MERROR("Не удалось выполнить начальную загрузку шейдера «%s». Подробности см. в журналах.", shader->name.c_str());
        return false;
    }

    // ЗАДАЧА: Выяснить, что должно быть по умолчанию здесь.
    vkShader->BoundPipelineIndex = 0;
    bool PipelineFound = false;
    for (u32 i = 0; i < PipelineCount; ++i) {
        if (vkShader->pipelines[i]) {
            vkShader->BoundPipelineIndex = i;

            // Извлеките первый тип из конвейера
            for (u32 j = 1; j < PrimitiveTopology::Type::MAX; j = j << 1) {
                if (vkShader->pipelines[i]->SupportedTopologyTypes & j) {
                    switch (j) {
                        case PrimitiveTopology::Type::TriangleList:
                            vkShader->CurrentTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                            break;
                        case PrimitiveTopology::Type::TriangleStrip:
                            vkShader->CurrentTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
                            break;
                        case PrimitiveTopology::Type::TriangleFan:
                            vkShader->CurrentTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
                            break;
                        case PrimitiveTopology::Type::LineList:
                            vkShader->CurrentTopology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
                            break;
                        case PrimitiveTopology::Type::LineStrip:
                            vkShader->CurrentTopology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
                            break;
                        case PrimitiveTopology::Type::PointList:
                            vkShader->CurrentTopology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
                            break;
                        default:
                            MWARN("Примитивная топология '%u' не поддерживается. Пропуск.", j);
                            break;
                    }

                    // Выделите здесь и просто предположите первый на данный момент. Это может быть переопределено тем, что использует шейдер, если это необходимо.
                    break;
                }
            }
            PipelineFound = true;
            break;
        }
    }

    if (!PipelineFound) {
        // Попадание сюда означает, что все конвейеры являются нулевыми, чего они определенно не должны быть.
        // Это дополнительная защита от сбоев, чтобы гарантировать, что конфигурация по крайней мере в какой-то степени разумна.
        MERROR("Доступные классы топологии недоступны, поэтому конвейер не может быть связан. Проверьте конфигурацию шейдера.");
        return false;
    }

    // Получите требования к выравниванию UBO с устройства.
    shader->RequiredUboAlignment = device.properties.limits.minUniformBufferOffsetAlignment;

    // Убедитесь, что UBO выровнен в соответствии с требованиями устройства.
    shader->GlobalUboStride = Range::GetAligned(shader->GlobalUboSize, shader->RequiredUboAlignment);
    shader->UboStride = Range::GetAligned(shader->UboSize, shader->RequiredUboAlignment);

    // Однородный буфер.
    RenderBuffer& ubuf = vkShader->UniformBuffer;
    ubuf.name = "renderbuffer_global_uniform";
    ubuf.AllocType = RenderBuffer::AllocationType::FreeList; 
    ubuf.TotalSize = shader->GlobalUboStride + (shader->UboStride * vkShader->MaxInstances);
    ubuf.type = RenderBufferType::Uniform;
    if (!RenderBufferCreate(ubuf)) {
        MERROR("VulkanAPI::ShaderInitialize — не удалось создать буфер Vulkan для шейдера объекта.");
        return false;
    }
    RenderBufferBind(ubuf, 0);

    // Отобразите всю память буфера.
    vkShader->MappedUniformBufferBlock = RenderBufferMapMemory(ubuf, 0, VK_WHOLE_SIZE);
    // Примечание: Все указанные ниже средства выделяются только в случае крайней необходимости.
    //
    // Выделите место для глобального UBO, которое должно занимать пространство _шага_, а не фактически используемый размер.
    if (shader->GlobalUboSize > 0 && shader->GlobalUboStride > 0) { 
        if (!ubuf.Allocate(shader->GlobalUboStride, shader->GlobalUboOffset)) {
            MERROR("Не удалось выделить место для универсального буфера!");
            return false;
        }
        
        // Выделите наборы глобальных дескрипторов, по одному на кадр. Global всегда является первым набором.
        // ЗАДАЧА: это должно быть динамическим и зависеть от количества образов Swapchain.
        VkDescriptorSetLayout GlobalLayouts[3] = {
            vkShader->DescriptorSetLayouts[0],
            vkShader->DescriptorSetLayouts[0],
            vkShader->DescriptorSetLayouts[0] };
            
        VkDescriptorSetAllocateInfo AllocInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = vkShader->DescriptorPool,
            // ЗАДАЧА: это должно быть динамическим и зависеть от количества образов Swapchain.
            .descriptorSetCount = 3,
            .pSetLayouts = GlobalLayouts
        };
        VK_CHECK(vkAllocateDescriptorSets(device.LogicalDevice, &AllocInfo, vkShader->GlobalDescriptorSets));
            
        for (u32 i = 0; i < 3; ++i) {
            char DescSetObjectName[512]{};
            MString::Format(DescSetObjectName, "desc_set_shader_%s_global_frame_%u", shader->name.c_str(), i);
            VulkanSetDebugObjectName(this, VK_OBJECT_TYPE_DESCRIPTOR_SET, vkShader->GlobalDescriptorSets[i], DescSetObjectName);
        }
    }

    return true;
}

bool VulkanAPI::ShaderReload(Shader *shader)
{
    return ShaderCreateModulesAndPipelines(shader);
}

bool VulkanAPI::ShaderSupportsWireframe(Shader *shader)
{
    // Если массив существует, это поддерживается.
    return shader->ShaderData->WireframePipelines;
}

bool VulkanAPI::ShaderUse(Shader *shader)
{
    if (!shader || !shader->ShaderData) { MERROR("VulkanAPI::ShaderUse требуется действительный указатель на шейдер."); return false; }

    auto VkShader = (VulkanShader*)shader->ShaderData;
    auto& CommandBuffer = GraphicsCommandBuffers[ImageIndex];
    VkShader->pipelines[VkShader->BoundPipelineIndex]->Bind(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
    BoundShader = shader;

    // Обязательно используйте текущий тип привязки.
    if (device.supportFlags & VulkanDevice::NativeDynamicStateBit) {
        vkCmdSetPrimitiveTopology(CommandBuffer.handle, VkShader->CurrentTopology);
    } else if (device.supportFlags & VulkanDevice::DynamicStateBit) {
        vkCmdSetPrimitiveTopologyEXT(CommandBuffer.handle, VkShader->CurrentTopology);
    }
    return true;
}

bool VulkanAPI::ShaderApplyGlobals(Shader *shader, bool NeedsUpdate, const FrameData& rFrameData)
{
    if (!shader || !shader->ShaderData) { MERROR("VulkanAPI::ShaderApplyGlobals требуется действительный указатель на шейдер."); return false; }
    // Не делайте ничего, если нет глобальных переменных для обновления.
    bool HasGlobal = shader->GlobalUniformCount > 0 || shader->GlobalUniformSamplerCount > 0;
    if (!HasGlobal) { return true; }
    
    VulkanShader* vkShader = shader->ShaderData;
    VkCommandBuffer CommandBuffer = GraphicsCommandBuffers[ImageIndex].handle;
    VkDescriptorSet GlobalDescriptor = vkShader->GlobalDescriptorSets[ImageIndex];

    if (NeedsUpdate) {
        VkWriteDescriptorSet DescriptorWrites[1 + VulkanShaderConstants::MaxGlobalTextures];

        u32 DescriptorWriteCount = 0;
        u32 BindingIndex = 0;

        // Обновляй только если там реально есть UBO
        if (shader->GlobalUniformCount > 0) {
            // Сначала примените UBO
            VkDescriptorBufferInfo BufferInfo {
                .buffer = ((VulkanBuffer*)vkShader->UniformBuffer.data)->handle,
                .offset = shader->GlobalUboOffset,
                .range = shader->GlobalUboStride
            };

            // Обновить наборы дескрипторов.
            VkWriteDescriptorSet UboWrite {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = vkShader->GlobalDescriptorSets[ImageIndex],
                .dstBinding = BindingIndex,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &BufferInfo
            };
            DescriptorWrites[BindingIndex] = UboWrite;
            DescriptorWriteCount++;
            BindingIndex++;
        }

        // Повторяйте выборку с помощью сэмплеров.
        if (shader->GlobalUniformSamplerCount > 0) {
            auto SetConfig = vkShader->DescriptorSets[0];

            // Повторите каждое связывание сэмплера.
            for (u32 sb = 0; sb < shader->GlobalUniformSamplerCount; ++sb) {
                VulkanUniformSamplerState& BindingSamplerState = vkShader->GlobalSamplerUniforms[sb];

                u32 BindingDescriptorCount = SetConfig.bindings[BindingIndex].descriptorCount;

                u32 UpdateSamplerCount = 0;
                VkDescriptorImageInfo ImageInfos[VulkanShaderConstants::MaxGlobalTextures];
                // Каждый дескриптор образца в составе переплета.
                for (u32 d = 0; d < BindingDescriptorCount; ++d) {
                    // ЗАДАЧА: Обновлять список следует только в случае крайней необходимости.
                    //
                    // Вместо простого списка текстурных карт, состояние экземпляра должно содержать список униформных сэмплеров, 
                    // каждый из которых имеет свой собственный список связанных с ним текстурных карт.
                    // Это позволит быстро находить информацию и назначать задания.
                    TextureMap* map = BindingSamplerState.UniformTextureMaps[d];
                    // if (!map) { continue; }
                    Texture* texture = map->texture;

                    // Убедитесь, что текстура корректна.
                    if (texture->generation == INVALID::ID) {
                        // Генерация текстур всегда некорректна для текстур по умолчанию, поэтому проверьте это, если вы уже используете какую-либо из них.
                        if (!TextureSystem::IsDefaultTexture(texture)) {
                            // Если вы не используете текстуру по умолчанию, возьмите текстуру по умолчанию. 
                            // Это сделано только в качестве подстраховки и предназначено для использования во время загрузки ресурсов.
                            texture = TextureSystem::GetDefaultTexture();
                        }
                        // Если используется текстура по умолчанию, отмените генерацию карты, чтобы она обновилась при следующем запуске.
                        map->generation = INVALID::ID;
                    } else {
                        // Если все в порядке, убедитесь, что генерация текстурной карты соответствует генерации самой текстуры.
                        // В противном случае, ресурсы текстурной карты следует перегенерировать.
                        if (texture->generation != map->generation) {
                            bool RefreshRequired = texture->MipLevels != map->MipLevels;
                            MTRACE("Требуется обновление сэмплера. Tex/map mips: %u/%u", RefreshRequired ? "" : " не", texture->MipLevels, map->MipLevels);
                            if (RefreshRequired && !TextureMapRefresh(map)) {
                                MWARN("Не удалось обновить ресурсы текстурных карт. Это означает, что настройки сэмплера могут быть устаревшими.");
                            } else {
                                // Синхронизируйте поколения.
                                map->generation = texture->generation;
                            }
                        }
                    }

                    auto image = (VulkanImage*)texture->data;
                    ImageInfos[d].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    ImageInfos[d].imageView = image->view;
                    ImageInfos[d].sampler = samplers[map->InternalID];

                    // ЗАДАЧА: Измените состояние дескриптора, чтобы обрабатывать это корректно.
                    // Синхронизируйте генерацию кадров, если не используется текстура по умолчанию.
                    // if (texture->generation != INVALID::ID) {
                    //     *DescriptorGeneration = texture->generation;
                    //     *DescriptorID = texture->id;
                    // }

                    UpdateSamplerCount++;
                }

                VkWriteDescriptorSet SamplerDescriptor {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = GlobalDescriptor,
                    .dstBinding = BindingIndex,
                    .descriptorCount = UpdateSamplerCount,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = ImageInfos
                };

                DescriptorWrites[DescriptorWriteCount] = SamplerDescriptor;
                DescriptorWriteCount++;

                BindingIndex++;
            }
        }

        if (DescriptorWriteCount > 0) {
            vkUpdateDescriptorSets(device.LogicalDevice, DescriptorWriteCount, DescriptorWrites, 0, nullptr);
        }
    }

    // Pick the correct pipeline.
    VulkanPipeline** PipelineArray = shader->IsWireframe ? vkShader->WireframePipelines : vkShader->pipelines;

    // Привяжите набор глобальных дескрипторов для обновления.
    vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineArray[vkShader->BoundPipelineIndex]->PipelineLayout, 0, 1, &GlobalDescriptor, 0, nullptr);
    return true;
}

bool VulkanAPI::ShaderApplyInstance(Shader *shader, bool NeedsUpdate, const FrameData &rFrameData)
{
    VulkanShader* vkShader = shader->ShaderData;
    if (shader->InstanceUniformCount < 1 && shader->InstanceUniformSamplerCount < 1) {
        MERROR("Этот шейдер не использует экземпляры.");
        return false;
    }
    VkCommandBuffer СommandBuffer = GraphicsCommandBuffers[ImageIndex].handle;

    // Получить данные экземпляра.
    VulkanShaderInstanceState& InstanceState = vkShader->InstanceStates[shader->BoundInstanceID];
    VkDescriptorSet InstanceDescriptorSet = InstanceState.DescriptorSets[ImageIndex];

    if (NeedsUpdate) {
        // Выделите достаточное количество операций записи в дескриптор для обработки максимально допустимого количества текстур на экземпляр.
        VkWriteDescriptorSet DescriptorWrites[1 + VulkanShaderConstants::MaxInstanceTextures]{};

        u32 DescriptorWriteCount = 0;
        u32 BindingIndex = 0;

        // Дескриптор 0 — буфер Uniform
        if (shader->InstanceUniformCount > 0) {
            // Выполняйте это только в том случае, если дескриптор еще не был обновлен.
            u8& InstanceUboGeneration = InstanceState.UboDescriptorState.generations[ImageIndex];
            if (InstanceUboGeneration == INVALID::U8ID) {
                VkDescriptorBufferInfo BufferInfo {
                    .buffer = ((VulkanBuffer*)vkShader->UniformBuffer.data)->handle,
                    .offset = InstanceState.offset,
                    .range = shader->UboSize
                };

                VkWriteDescriptorSet UboDescriptor {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = InstanceDescriptorSet,
                    .dstBinding = BindingIndex,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &BufferInfo
                };

                DescriptorWrites[DescriptorWriteCount] = UboDescriptor;
                DescriptorWriteCount++;

                // Обновите генерацию кадра. В данном случае это необходимо сделать только один раз, поскольку речь идёт о буфере.
                InstanceUboGeneration = 1;
            }
            BindingIndex++;
        }

        // Повторяйте выборку с помощью сэмплеров.
        if (shader->InstanceUniformSamplerCount > 0) {
            bool HasGlobal = shader->GlobalUniformCount > 0 || shader->GlobalUniformSamplerCount > 0;
            u8 InstanceDescSetIndex = HasGlobal ? 1 : 0;
            VulkanDescriptorSetConfig SetConfig = vkShader->DescriptorSets[InstanceDescSetIndex];

            // Повторите каждое связывание сэмплера.
            for (u32 sb = 0; sb < shader->InstanceUniformSamplerCount; ++sb) {
                VulkanUniformSamplerState& BindingSamplerState = InstanceState.SamplerUniforms[sb];

                u32 BindingDescriptorCount = SetConfig.bindings[BindingIndex].descriptorCount;

                u32 UpdateSamplerCount = 0;
                auto ImageInfos = (VkDescriptorImageInfo*)rFrameData.FrameAllocator->Allocate(sizeof(VkDescriptorImageInfo) * BindingDescriptorCount);
                // Каждый дескриптор сэмплера в пределах привязки.
                for (u32 d = 0; d < BindingDescriptorCount; ++d) {
                    // ЗАДАЧА: обновлять список только в случае крайней необходимости.
                    //
                    // Вместо простого списка текстурных карт, состояние экземпляра должно содержать список униформных сэмплеров, 
                    // каждый из которых имеет свой собственный список связанных с ним текстурных карт. Это обеспечит быстрый поиск/назначение.
                    TextureMap* map = BindingSamplerState.UniformTextureMaps[d];
                    // if (!map) { continue; }
                    Texture* texture = map->texture;

                    // Убедитесь, что текстура корректна.
                    if (texture->generation == INVALID::ID) {
                        // Генерация текстур всегда некорректна для текстур по умолчанию, поэтому проверьте, используете ли вы уже какую-либо из них.
                        if (!TextureSystem::IsDefaultTexture(texture)) {
                            // Если вы не используете какой-либо из них, возьмите значение по умолчанию. 
                            // Это сделано только в качестве резервного варианта и предназначено для использования во время загрузки ресурсов.
                            if (texture->type == TextureType::_2D) {
                                texture = TextureSystem::GetDefaultTexture();
                            } else if (texture->type == TextureType::_2DArray) {
                                // ЗАДАЧА: Исходя из предположения, что это ландшафт.
                                // Вероятно, следует получить новую текстуру по умолчанию с соответствующим количеством слоев.
                                texture = TextureSystem::GetDefaultTexture(Texture::Terrain);
                            } else if (texture->type == TextureType::Cube) {
                                texture = TextureSystem::GetDefaultTexture(Texture::Cube);
                            } else {
                                MERROR("При применении экземпляра рендерера Vulkan не удалось определить тип текстуры. Выполняется возврат к 2D-рендерингу.");
                                texture = TextureSystem::GetDefaultTexture();
                            }
                        }
                        // Если используется текстура по умолчанию, отмените генерацию карты, чтобы она обновилась при следующем запуске.
                        map->generation = INVALID::ID;
                    } else {
                        // Если все в порядке, убедитесь, что генерация текстурной карты соответствует генерации самой текстуры.
                        // В противном случае, ресурсы текстурной карты следует перегенерировать.
                        if (texture->generation != map->generation) {
                            bool RefreshRequired = texture->MipLevels != map->MipLevels;
                            MTRACE("%sTex/map mips: %u/%u", RefreshRequired ? "Требуется обновление сэмплера. " : "Обновление сэмплера не требуется. ", texture->MipLevels, map->MipLevels);
                            if (RefreshRequired && !TextureMapRefresh(map)) {
                                MWARN("Не удалось обновить ресурсы текстурных карт. Это означает, что настройки сэмплера могут быть устаревшими.");
                            } else {
                                // Синхронизируйте поколения.
                                map->generation = texture->generation;
                            }
                        }
                    }

                    auto image = (VulkanImage*)texture->data;
                    ImageInfos[d].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    ImageInfos[d].imageView = image->view;
                    ImageInfos[d].sampler = samplers[map->InternalID];

                    // ЗАДАЧА: изменить состояние дескриптора для корректной обработки этой проблемы.
                    // Синхронизировать генерацию кадров, если не используется текстура по умолчанию.
                    // if (texture->generation != INVALID::ID) {
                    //     *DescriptorGeneration = texture->generation;
                    //     *DescriptorID = texture->id;
                    // }

                    UpdateSamplerCount++;
                }

                VkWriteDescriptorSet SamplerDescriptor {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = InstanceDescriptorSet,
                    .dstBinding = BindingIndex,
                    .descriptorCount = UpdateSamplerCount,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = ImageInfos
                };

                DescriptorWrites[DescriptorWriteCount] = SamplerDescriptor;
                DescriptorWriteCount++;

                BindingIndex++;
            }
        }

        if (DescriptorWriteCount > 0) {
            vkUpdateDescriptorSets(device.LogicalDevice, DescriptorWriteCount, DescriptorWrites, 0, nullptr);
        }
    }

    // Определите индекс набора дескрипторов, который будет первым. 
    // Если глобальных переменных нет, например, он будет равен 0. 
    // Если глобальные переменные есть, он будет равен 1.
    u32 FirstSet = 1;
    bool HasGlobal = shader->GlobalUniformCount > 0 || shader->GlobalUniformSamplerCount > 0;
    if (!HasGlobal) {
        FirstSet = 0;
    }
    // Укажите набор дескрипторов, который необходимо обновить, или укажите его тип в случае изменения шейдера.
    vkCmdBindDescriptorSets(СommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkShader->pipelines[vkShader->BoundPipelineIndex]->PipelineLayout, FirstSet, 1, &InstanceDescriptorSet, 0, nullptr);
    return true;
}

bool VulkanAPI::ShaderApplyLocal(Shader *shader, const FrameData &rFrameData)
{
    VulkanShader* vkShader = shader->ShaderData;
    VkCommandBuffer CommandBuffer = GraphicsCommandBuffers[ImageIndex].handle;
    vkCmdPushConstants(
        CommandBuffer,
        vkShader->pipelines[vkShader->BoundPipelineIndex]->PipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0, 128, vkShader->LocalPushConstantBlock);
    return true;
}

bool VulkanAPI::ShaderBindGlobals(Shader *shader)
{
    if (!shader || !shader->ShaderData) { return false; }

    // ПРИМЕЧАНИЕ: Это поле намеренно оставлено пустым и ничего не делает. Для работы с другими API может потребоваться ввод данных здесь.
    return true;
}

bool VulkanAPI::ShaderBindInstance(Shader *shader, u32 InstanceID)
{
    if (!shader || !shader->ShaderData) {
        MERROR("Для работы функции VulkanAPI::ShaderBindInstance требуется действительный указатель на шейдер.");
        return false;
    }
    
    if (InstanceID == INVALID::ID) {
        MERROR("Невозможно привязать экземпляр INVALID::ID.");
        return false;
    }

    // shader->BoundInstanceID = InstanceID;
    shader->BoundUboOffset = shader->ShaderData->InstanceStates[shader->BoundInstanceID].offset;
    return true;
}

bool VulkanAPI::ShaderBindLocal(Shader *shader)
{
    if (!shader || !shader->ShaderData) {
        return false;
    }

    // ПРИМЕЧАНИЕ: Это поле намеренно оставлено пустым и ничего не делает. Для работы с другими API может потребоваться ввод данных здесь.
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

VkFilter ConvertFilterType(const char* op, TextureFilterMode filter) {
    switch (filter) {
        case TextureFilterMode::Nearest:
            return VK_FILTER_NEAREST;
        case TextureFilterMode::Linear:
            return VK_FILTER_LINEAR;
        default:
            MWARN("ConvertFilterType(op='%s'): Неподдерживаемый тип фильтра «%x», по умолчанию — линейный.", op, filter);
            return VK_FILTER_LINEAR;
    }
}

bool VulkanAPI::ShaderAcquireInstanceResources(Shader* shader, const ShaderInstanceResourceConfig& config, u32& OutInstanceID)
{
    auto VkShader = shader->ShaderData;
    OutInstanceID = INVALID::ID;
    for (u32 i = 0; i < VkShader->MaxInstances; ++i) {
        if (VkShader->InstanceStates[i].id == INVALID::ID) {
            VkShader->InstanceStates[i].id = i;
            OutInstanceID = i;
            break;
        }
    }
    if (OutInstanceID == INVALID::ID) {
        MERROR("VulkanShader::AcquireInstanceResources — не удалось получить новый идентификатор для шейдера '%s', максимальное количество экземпляров = %u", shader->name.c_str(), VkShader->MaxInstances);
        return false;
    }

    Texture* DefaultTexture = TextureSystem::GetDefaultTexture();
    // Сопоставьте текстурные карты из конфигурации с соответствующими униформами
    VulkanShaderInstanceState& InstanceState = VkShader->InstanceStates[OutInstanceID];
    
    // Настраивайте только в том случае, если шейдер действительно этого требует.
    if (shader->InstanceTextureCount > 0) {
        CreateArray(InstanceState.SamplerUniforms, sizeof(VulkanUniformSamplerState) * shader->InstanceUniformSamplerCount);

        // Назначьте униформы каждому из состояний сэмплера.
        for (u32 i = 0; i < shader->InstanceUniformSamplerCount; ++i) {
            VulkanUniformSamplerState& SamplerState = InstanceState.SamplerUniforms[i];
            SamplerState.uniform = &shader->uniforms[shader->InstanceSamplerIndices[i]];

            // Также получите конфигурацию текстур униформ.
            ShaderInstanceUniformTextureConfig& tc = config.UniformConfigs[i];

            u32 ArrayLength = MMAX(SamplerState.uniform->ArrayLength, 1);
            // Настройте массив для текстурных карт сэмплера.
            CreateArray(SamplerState.UniformTextureMaps, sizeof(void*) * ArrayLength);
            // Настройте состояния дескрипторов.
            CreateArray(SamplerState.DescriptorStates, sizeof(VulkanDescriptorState) * ArrayLength);
            // Для каждого дескриптора
            for (u32 d = 0; d < ArrayLength; ++d) {
                SamplerState.UniformTextureMaps[d] = tc.TextureMaps[d];
                // Убедитесь, что ему назначена текстурная карта. Используйте значение по умолчанию, если нет.
                if (!SamplerState.UniformTextureMaps[d]->texture) {
                    SamplerState.UniformTextureMaps[d]->texture = DefaultTexture;
                }
                // Для каждого кадра
                // ЗАДАЧА: обрабатывайте различное количество кадров.
                for (u32 j = 0; j < 3; ++j) {
                    SamplerState.DescriptorStates[d].generations[j] = INVALID::U8ID;
                    SamplerState.DescriptorStates[d].ids[j] = INVALID::ID;
                }
            }
        }
    }

    // Выделите немного места в УБО — по шагу, а не по размеру.
    const u64 size = shader->UboStride;
    if (size > 0) {
        if (!VkShader->UniformBuffer.Allocate(size, InstanceState.offset)) {
            MERROR("VulkanAPI::ShaderAcquireInstanceResources — не удалось получить пространство UBO");
            return false;
        }
    }

    // Привязка UBO. ПРИМЕЧАНИЕ: это имеет значение только там, где есть экземпляры униформ, но всё равно их установите.
    for (u32 i = 0; i < 3; ++i) {
        InstanceState.UboDescriptorState.generations[i] = INVALID::U8ID;
        InstanceState.UboDescriptorState.ids[i] = INVALID::ID;
    }

    bool HasGlobal = shader->GlobalUniformCount > 0 || shader->GlobalUniformSamplerCount > 0;
    u8 InstanceDescSetIndex = HasGlobal ? 1 : 0;
    // ЗАДАЧА: Обработка различного количества кадров.
    // Выделите 3 набора дескрипторов (по одному на кадр).
    VkDescriptorSetLayout layouts[3] = {
        VkShader->DescriptorSetLayouts[InstanceDescSetIndex],
        VkShader->DescriptorSetLayouts[InstanceDescSetIndex],
        VkShader->DescriptorSetLayouts[InstanceDescSetIndex]
    };

    VkDescriptorSetAllocateInfo AllocInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = VkShader->DescriptorPool,
        .descriptorSetCount = 3,
        .pSetLayouts = layouts
    };

    VkResult result = vkAllocateDescriptorSets(device.LogicalDevice, &AllocInfo, InstanceState.DescriptorSets);
    if (result != VK_SUCCESS) {
        MERROR("Ошибка при выделении наборов дескрипторов экземпляра в шейдере: '%s'.", VulkanResultString(result, true));
        return false;
    }

    for (u32 i = 0; i < 3; ++i) {
        char DescSetObjectName[512]{};
        MString::Format(DescSetObjectName, "desc_set_shader_%.*s_instance_frame_%u", S_Arg(shader->name), i);
        VulkanSetDebugObjectName(this, VK_OBJECT_TYPE_DESCRIPTOR_SET, InstanceState.DescriptorSets[i], DescSetObjectName);
    }

    return true;
}

bool VulkanAPI::ShaderReleaseInstanceResources(Shader *shader, u32 InstanceID)
{
    VulkanShader* VkShader = shader->ShaderData;
    VulkanShaderInstanceState& InstanceState = VkShader->InstanceStates[InstanceID];

    // Дождитесь завершения любых ожидающих операций, использующих набор дескрипторов.
    vkDeviceWaitIdle(device.LogicalDevice);

    // 3 свободных набора дескрипторов (по одному на кадр)
    VkResult result = vkFreeDescriptorSets(
        device.LogicalDevice,
        VkShader->DescriptorPool,
        3,
        InstanceState.DescriptorSets);
    if (result != VK_SUCCESS) {
        MERROR("Ошибка при освобождении наборов дескрипторов объекта шейдера!");
    }

    // Аннулировать состояние дескриптора UBO.
    for (u32 j = 0; j < 3; ++j) {
        InstanceState.UboDescriptorState.generations[j] = INVALID::U8ID;
        InstanceState.UboDescriptorState.ids[j] = INVALID::ID;
    }

    // Уничтожить привязки и их дескрипторные состояния/униформы.
    for (u32 a = 0; a < shader->InstanceUniformSamplerCount; ++a) {
        auto& SamplerState = InstanceState.SamplerUniforms[a];
        u32 ArrayLength = MMAX(SamplerState.uniform->ArrayLength, 1);
        MemorySystem::Free(SamplerState.DescriptorStates, sizeof(VulkanDescriptorState) * ArrayLength, Memory::Array);
        SamplerState.DescriptorStates = nullptr;
        MemorySystem::Free(SamplerState.UniformTextureMaps, sizeof(void*) * ArrayLength, Memory::Array);
        SamplerState.UniformTextureMaps = nullptr;
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

static bool SamplerStateTrySet(VulkanUniformSamplerState *SamplerUniforms, u32 SamplerCount, u16 UniformLocation, u32 ArrayIndex, TextureMap *map) {
    // Найдите состояние униформы сэмплера, которое необходимо обновить.
    for (u32 i = 0; i < SamplerCount; ++i) {
        auto& su = SamplerUniforms[i];
        if (su.uniform->location == UniformLocation) {
            if (su.uniform->ArrayLength > 1) {
                if (ArrayIndex >= su.uniform->ArrayLength) {
                    MERROR("Ошибка SamplerStateTrySet: array_index (%u) выходит за пределы диапазона (0-%u)", ArrayIndex, su.uniform->ArrayLength);
                    return false;
                }
                su.UniformTextureMaps[ArrayIndex] = map;
            } else {
                su.UniformTextureMaps[0] = map;
            }
            return true;
        }
    }
    MERROR("SamplerStateTrySet: Не удалось найти местоположение униформы %u. Униформа сэмплера не задана.", UniformLocation);
    return false;
}

bool VulkanAPI::ShaderSetUniform(Shader *shader, ShaderUniform *uniform, u32 ArrayIndex, const void *value)
{
    auto VkShader = shader->ShaderData;
    if (UniformTypeIsSampler(uniform->type)) {
        // Сэмплеры можно назначать только на уровне экземпляра или на глобальном уровне.
        auto map = (TextureMap*)value;
        if (uniform->scope == ShaderScope::Global) {
            return SamplerStateTrySet(VkShader->GlobalSamplerUniforms, shader->GlobalUniformSamplerCount, uniform->location, ArrayIndex, map);
        } else {
            auto& InstanceState = VkShader->InstanceStates[shader->BoundInstanceID];
            return SamplerStateTrySet(InstanceState.SamplerUniforms, shader->InstanceUniformSamplerCount, uniform->location, ArrayIndex, map);
        }
    } else {
        if (uniform->scope == ShaderScope::Local) {
            u64 addr = reinterpret_cast<u64>(VkShader->LocalPushConstantBlock);
            addr += uniform->offset + (uniform->size * ArrayIndex);
            MemorySystem::CopyMem(reinterpret_cast<void*>(addr), value, uniform->size);
        } else {
            // Сопоставьте подходящую ячейку памяти и скопируйте данные.
            u64 addr = (u64)VkShader->MappedUniformBufferBlock;
            addr += shader->BoundUboOffset + uniform->offset + (uniform->size * ArrayIndex);
            MemorySystem::CopyMem(reinterpret_cast<void*>(addr), value, uniform->size);
        }
    }
    return true;
}

bool VulkanAPI::CreateShaderModule(Shader *shader, const ShaderStageConfig& config, VulkanShaderStage &ShaderStage)
{
    shaderc_shader_kind ShaderKind;
    const char* ShaderTypeStr = nullptr;
    VkShaderStageFlagBits stage;

    switch (config.stage) {
        case ShaderStage::Vertex:
            ShaderKind = shaderc_glsl_default_vertex_shader;
            ShaderTypeStr = "vertex";
            stage = VK_SHADER_STAGE_VERTEX_BIT;
        break;
        case ShaderStage::Fragment:
            ShaderKind = shaderc_glsl_default_fragment_shader;
            ShaderTypeStr = "fragment";
            stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        break;
        case ShaderStage::Compute:
            ShaderKind = shaderc_glsl_default_compute_shader;
            ShaderTypeStr = "compute";
            stage = VK_SHADER_STAGE_COMPUTE_BIT;
        break;
        case ShaderStage::Geometry:
            ShaderKind = shaderc_glsl_default_geometry_shader;
            ShaderTypeStr = "geometry";
            stage = VK_SHADER_STAGE_GEOMETRY_BIT;
        break;
        default:
            MERROR("Неподдерживаемый тип шейдера. Не удалось создать модуль.");
        return false;
    }

    MDEBUG("Компиляция этапа '%s' для шейдера '%s'...", ShaderTypeStr, shader->name.c_str());

    // Попытаемся скомпилировать шейдер.
    auto CompilationResult = shaderc_compile_into_spv(
        ShaderCompiler,
        config.source.c_str(),
        config.source.Length(),
        ShaderKind,
        config.filename.c_str(),
        "main",
        nullptr
    );
        
    if (!CompilationResult) {
        MERROR("При попытке компиляции шейдера произошла неизвестная ошибка. Дальнейшая обработка невозможна.");
        return false;
    }
    auto status = shaderc_result_get_compilation_status(CompilationResult);

    // Обрабатывайте ошибки, если таковые имеются.
    if (status != shaderc_compilation_status_success) {
        const char *ErrorMessage = shaderc_result_get_error_message(CompilationResult);
        u64 ErrorCount = shaderc_result_get_num_errors(CompilationResult);
        MERROR("Ошибка компиляции шейдера с %llu ошибками.", ErrorCount);
        MERROR("Ошибки:\n%s", ErrorMessage);
        shaderc_result_release(CompilationResult);
        return false;
    }

    MDEBUG("Шейдер успешно скомпилирован.");

    // Вывести предупреждения, если таковые имеются.
    u64 WarningCount = shaderc_result_get_num_warnings(CompilationResult);
    if (WarningCount) {
        // ПРИМЕЧАНИЕ: Не уверен, что это правильный способ получения предупреждений.
        MWARN("Во время компиляции шейдера было сгенерировано %llu предупреждений:\n%s", WarningCount, shaderc_result_get_error_message(CompilationResult));
    }

    // Извлеките данные из результата.
    const char *bytes = shaderc_result_get_bytes(CompilationResult);
    size_t ResultLength = shaderc_result_get_length(CompilationResult);
    // Скопируйте данные результата и приведите их к типу u32*, как того требует Vulkan.
    u32 *code = (u32*)MemorySystem::Allocate(ResultLength, Memory::Renderer);
    MemorySystem::CopyMem(code, bytes, ResultLength);

    // Опубликуйте результат компиляции.
    shaderc_result_release(CompilationResult);

    // MMemory::ZeroMem(&ShaderStage->CreateInfo, sizeof(VkShaderModuleCreateInfo));
    ShaderStage.CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ShaderStage.CreateInfo.codeSize = ResultLength;
    ShaderStage.CreateInfo.pCode = code;

    VK_CHECK(vkCreateShaderModule(
        device.LogicalDevice,
        &ShaderStage.CreateInfo,
        allocator,
        &ShaderStage.handle)
    );

    // Информация об этапе шейдера
    //MMemory::ZeroMem(&ShaderStage->ShaderStageCreateInfo, sizeof(VkPipelineShaderStageCreateInfo));
    ShaderStage.ShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStage.ShaderStageCreateInfo.stage = stage;
    ShaderStage.ShaderStageCreateInfo.module = ShaderStage.handle;
    ShaderStage.ShaderStageCreateInfo.pName = "main";

    return true;
}

#pragma endregion shader

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
    vkGetPhysicalDeviceMemoryProperties(device.PhysicalDevice, &MemoryProperties);

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

void VulkanAPI::WaitForIdle()
{
    VK_CHECK(vkDeviceWaitIdle(device.LogicalDevice));
}

void VulkanAPI::CreateCommandBuffers()
{
    if (GraphicsCommandBuffers.Length() == 0) {
        GraphicsCommandBuffers.Resize(swapchain.ImageCount);
    }

    for (u32 i = 0; i < swapchain.ImageCount; ++i) {
        if (GraphicsCommandBuffers[i].handle) {
            VulkanCommandBufferFree(this, device.GraphicsCommandPool, GraphicsCommandBuffers[i]);
        } 
        //MMemory::ZeroMemory(&GraphicsCommandBuffers[i], sizeof(VulkanCommandBuffer));
        VulkanCommandBufferAllocate(this, device.GraphicsCommandPool, true, GraphicsCommandBuffers[i]);
    }

    MDEBUG("Созданы командные буферы Vulkan.");
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
    vkDeviceWaitIdle(device.LogicalDevice);

    // Поддержка запроса ЗАДАЧА: переделать
    device.QuerySwapchainSupport(device.PhysicalDevice, surface, &device.SwapchainSupport);
    device.DetectDepthFormat();

    swapchain.Recreate(this, FramebufferWidth, FramebufferHeight);

    // Обновить генерацию размера кадрового буфера.
    FramebufferSizeLastGeneration = FramebufferSizeGeneration;

    // Очистка цепочки подкачки
    for (u32 i = 0; i < swapchain.ImageCount; ++i) {
        VulkanCommandBufferFree(this, device.GraphicsCommandPool, GraphicsCommandBuffers[i]);
    }

    // Укажите слушателям, что требуется обновление цели рендеринга.
    EventContext EventContext{};
    EventSystem::Fire(EventSystem::DefaultRendertargetRefreshRequired, nullptr, EventContext);

    CreateCommandBuffers();

    // Снимите флаг воссоздания.
    RecreatingSwapchain = false;

    return true;
}

bool VulkanAPI::CreateSampler(TextureMap *map, VkSampler *sampler)
{
    // Синхронизируйте уровни MIP с уровнями назначенной текстуры.
    map->MipLevels = map->texture->MipLevels;

    // Создайте сэмплер для текстуры
    VkSamplerCreateInfo SamplerInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = ConvertFilterType("mag", map->FilterMagnify),
        .minFilter = ConvertFilterType("min", map->FilterMinify),
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = ConvertRepeatType("U", map->RepeatU),
        .addressModeV = ConvertRepeatType("V", map->RepeatV),
        .addressModeW = ConvertRepeatType("W", map->RepeatW),
        // ЗАДАЧА: Настраивается
        // .mipLodBias = 0.F,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = 16,
        // .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        // Использовать весь доступный диапазон mips.
        // .minLod = 0.F,
        // ПРИМЕЧАНИЕ: Раскомментируйте следующую строку, чтобы проверить самый низкий уровень MIP.
        // .minLod = map->MipLevels > 1 ? map->MipLevels : 0.F;
        .maxLod = (f32)map->MipLevels,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE,
        // .unnormalizedCoordinates = VK_FALSE
    };

    VkResult result = vkCreateSampler(device.LogicalDevice, &SamplerInfo, allocator, sampler);
    if (!VulkanResultIsSuccess(VK_SUCCESS)) {
        MERROR("Ошибка создания сэмплера текстуры: %s.", VulkanResultString(result, true));
        return false;
    }

    return true;
}

bool VulkanAPI::TextureMapResourceAcquire(TextureMap *map)
{
    // Найдите свободный сэмплер.
    u32 SamplerCount = samplers.Length();
    u32 SelectedID = INVALID::ID;
    for (u32 i = 0; i < SamplerCount; ++i) {
        if (samplers[i] == VK_NULL_HANDLE) {
            SelectedID = i;
            break;
        }
    }

    if (SelectedID == INVALID::ID) {
        // Добавьте пустой элемент в массив.
        samplers.PushBack(VK_NULL_HANDLE);
        SelectedID = SamplerCount;
    }

    if (!CreateSampler(map, &samplers[SelectedID])) {
        return false;
    }

    char FormattedName[TEXTURE_NAME_MAX_LENGTH] = {0};
    MString::Format(FormattedName, "%s_texmap_sampler", map->texture->name);
    VK_SET_DEBUG_OBJECT_NAME(this, VK_OBJECT_TYPE_SAMPLER, samplers[SelectedID], FormattedName);
    map->InternalID = SelectedID;

    return true;
}

void VulkanAPI::TextureMapDestroy(TextureMap *map)
{
    if (map && map->InternalID != INVALID::ID) {
        // Убедитесь, что это не используется.
        vkDeviceWaitIdle(device.LogicalDevice);
        VkSampler sampler = samplers[map->InternalID];
        vkDestroySampler(device.LogicalDevice, sampler, allocator);
        sampler = nullptr;
    }
}

bool VulkanAPI::TextureMapRefresh(TextureMap *map)
{
    if (map && map->InternalID != INVALID::ID) {
        // Сначала создайте новый сэмплер.
        VkSampler NewSampler = VK_NULL_HANDLE;
        if (!CreateSampler(map, &NewSampler)) {
            return false;
        }

        // Скопируйте указатель на текущий сэмплер.
        VkSampler& CurrentSampler = samplers[map->InternalID];
        VkSampler OldSampler = CurrentSampler;

        // Убедитесь, что он нигде не используется.
        vkDeviceWaitIdle(device.LogicalDevice);

        // Присваиваем новый идентификатор.
        CurrentSampler = NewSampler;

        // Удалите старый.
        vkDestroySampler(device.LogicalDevice, OldSampler, allocator);
    }
    return true;
}

void VulkanAPI::RenderTargetCreate(u8 AttachmentCount, RenderTargetAttachment *attachments, Renderpass *pass, u32 width, u32 height, u32 LayerIndex, RenderTarget &OutTarget)
{
    // Максимальное количество вложений
    VkImageView AttachmentViews[32]{};
    for (u32 i = 0; i < AttachmentCount; ++i) {
        auto vkImage = attachments ? (VulkanImage*)attachments[i].texture->data : (VulkanImage*)OutTarget.attachments[i].texture->data;
        if (vkImage->LayerViews) {
            AttachmentViews[i] = vkImage->LayerViews[LayerIndex];
        } else {
            AttachmentViews[i] = vkImage->view;
        }
    }

    if (attachments) {
        MemorySystem::CopyMem(OutTarget.attachments, attachments, sizeof(RenderTargetAttachment) * AttachmentCount);
    }
    
    VkFramebufferCreateInfo FramebufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = ((VulkanRenderpass*)pass->InternalData)->handle,
        .attachmentCount = AttachmentCount,
        .pAttachments = AttachmentViews,
        .width = width,
        .height = height,
        .layers = 1
    };

    VK_CHECK(vkCreateFramebuffer(device.LogicalDevice, &FramebufferCreateInfo, allocator, (VkFramebuffer*)&OutTarget.InternalFramebuffer));

    char FormattedName[512]{};
    MString::Format(FormattedName, "pass_%s_framebuffer_%u_x_%u", pass->name.c_str(), width, height);
    VK_SET_DEBUG_OBJECT_NAME(this, VK_OBJECT_TYPE_FRAMEBUFFER, OutTarget.InternalFramebuffer, FormattedName);
    MTRACE("Создан кадровый буфер '%s' по адресу 0x%x.", FormattedName, OutTarget.InternalFramebuffer);
}

void VulkanAPI::RenderTargetDestroy(RenderTarget &target, bool FreeInternalMemory)
{
    if (target.InternalFramebuffer) {
        vkDestroyFramebuffer(device.LogicalDevice, (VkFramebuffer)target.InternalFramebuffer, allocator);
        target.InternalFramebuffer = nullptr;
        if (FreeInternalMemory) {
            MemorySystem::Free(target.attachments, sizeof(RenderTargetAttachment) * target.AttachmentCount, Memory::Array);
            target.attachments = nullptr;
            target.AttachmentCount = 0;
        }
    }
}

bool VulkanAPI::RenderpassCreate(RenderpassConfig& config, Renderpass& OutRenderpass, bool copy)
{
    if (copy) {
        OutRenderpass.RenderArea = config.RenderArea;
        OutRenderpass.ClearColour = config.ClearColour;
        OutRenderpass.ClearFlags = config.ClearFlags;
        OutRenderpass.RenderTargetCount = config.RenderTargetCount;
        CreateArray(OutRenderpass.targets, sizeof(RenderTarget) * OutRenderpass.RenderTargetCount); // OutRenderpass.targets = nul_alloc(RenderTarget, OutRenderpass.RenderTargetCount, Memory::Array);
        OutRenderpass.name = config.name;
    }

    // Скопируйте конфигурацию для каждой цели.
    for (u32 t = 0; t < OutRenderpass.RenderTargetCount; ++t) {
        RenderTarget& target = OutRenderpass.targets[t];
        target.AttachmentCount = config.target.AttachmentCount;
        CreateArray(target.attachments, sizeof(RenderTargetAttachment) * target.AttachmentCount); // nul_alloc(RenderTargetAttachment, target.AttachmentCount, Memory::Array);

        // Каждое вложение для цели.
        for (u32 a = 0; a < target.AttachmentCount; ++a) {
            RenderTargetAttachment& attachment = target.attachments[a];
            RenderTargetAttachmentConfig& AttachmentConfig = config.target.attachments[a];

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

Texture *VulkanAPI::GetWindowAttachment(u8 index)
{
    if (index >= swapchain.ImageCount) {
        MFATAL("Попытка получить индекс цветового вложения вне диапазона: %d. Количество вложений: %d", index, swapchain.ImageCount);
        return nullptr;
    }

    return &swapchain.RenderTextures[index];
}

Texture *VulkanAPI::GetDepthAttachment(u8 index)
{
    if (index >= swapchain.ImageCount) {
        MFATAL("Попытка получить индекс вложения глубины вне диапазона: %d. Количество вложений: %d", index, swapchain.ImageCount);
        return nullptr;
    }

    return &swapchain.DepthTextures[index];
}

u8 VulkanAPI::GetWindowAttachmentIndex()
{
    return ImageIndex;
}

u8 VulkanAPI::GetWindowAttachmentCount()
{
    return swapchain.ImageCount;
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

bool VulkanAPI::RenderBufferCreate(RenderBuffer &buffer)
{
    VulkanBuffer InternalBuffer;

    switch (buffer.type) {
        case RenderBufferType::Vertex: {
            InternalBuffer.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            InternalBuffer.MemoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        } break;
        case RenderBufferType::Index: {
            InternalBuffer.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            InternalBuffer.MemoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        } break;
        case RenderBufferType::Uniform: {
            u32 DeviceLocalBits = device.SupportsDeviceLocalHostVisible ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0;
            InternalBuffer.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            InternalBuffer.MemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | DeviceLocalBits;
        } break;
        case RenderBufferType::Staging: {
            InternalBuffer.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            InternalBuffer.MemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        } break;
        case RenderBufferType::Read: {
            InternalBuffer.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            InternalBuffer.MemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        } break;
        case RenderBufferType::Storage: {
            MERROR("Буфер хранения пока не поддерживается.");
            return false;
        }
        default:
            MERROR("Неподдерживаемый тип буфера: %i", buffer.type);
            return false;
    }

    VkBufferCreateInfo BufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    BufferInfo.size = buffer.TotalSize;
    BufferInfo.usage = InternalBuffer.usage;
    BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // ПРИМЕЧАНИЕ: Используется только в одной очереди.

    VK_CHECK(vkCreateBuffer(device.LogicalDevice, &BufferInfo, allocator, &InternalBuffer.handle));

    // Соберите требования к памяти.
    vkGetBufferMemoryRequirements(device.LogicalDevice, InternalBuffer.handle, &InternalBuffer.MemoryRequirements);
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
    VkResult result = vkAllocateMemory(device.LogicalDevice, &AllocateInfo, allocator, &InternalBuffer.memory);
    VK_SET_DEBUG_OBJECT_NAME(this, VK_OBJECT_TYPE_DEVICE_MEMORY, InternalBuffer.memory, buffer.name.c_str());

    // Определите, находится ли память в куче устройства.
    bool IsDeviceMemory = (InternalBuffer.MemoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    // Сообщите о памяти как об используемой.
    MemorySystem::AllocateReport(InternalBuffer.MemoryRequirements.size, IsDeviceMemory ? Memory::GPULocal : Memory::Vulkan);

    if (result != VK_SUCCESS) {
        MERROR("Не удалось создать буфер vulkan, так как требуемое выделение памяти не удалось. Ошибка: %i", result);
        return false;
    }

    // При необходимости создайте бесплатный список.
    if (buffer.AllocType == RenderBuffer::AllocationType::FreeList && !buffer.FreelistBlock) {
        buffer.FreelistMemoryRequirement = FreeList::GetMemoryRequirement(buffer.TotalSize);
        buffer.FreelistBlock = MemorySystem::Allocate(buffer.FreelistMemoryRequirement, Memory::Renderer);
        buffer.BufferFreelist.Create(buffer.TotalSize, buffer.FreelistBlock);
    }

    // Выделите блок внутреннего состояния памяти в конце, как только мы убедимся, что все было создано успешно.
    buffer.data = new VulkanBuffer(InternalBuffer);

    return true;
}

void VulkanAPI::RenderBufferDestroy(RenderBuffer &buffer)
{
    auto VkBuf = (VulkanBuffer*)buffer.data;
    if (VkBuf) {
        if (VkBuf->memory) {
            vkFreeMemory(device.LogicalDevice, VkBuf->memory, allocator);
            VkBuf->memory = 0;
        }
        if (VkBuf->handle) {
            vkDestroyBuffer(device.LogicalDevice, VkBuf->handle, allocator);
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
    VK_CHECK(vkBindBufferMemory(device.LogicalDevice, VkBuf->handle, VkBuf->memory, offset))
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
    VK_CHECK(vkMapMemory(device.LogicalDevice, VkBuf->memory, offset, size, 0, &data));
    return data;
}

void VulkanAPI::RenderBufferUnmapMemory(RenderBuffer &buffer, u64 offset, u64 size)
{
    if (!buffer.data) {
        MERROR("VulkanAPI::RenderBufferUnmapMemory требует действительный указатель на буфер.");
        return;
    }
    auto VkBuffer = reinterpret_cast<VulkanBuffer*>(buffer.data);
    vkUnmapMemory(device.LogicalDevice, VkBuffer->memory);
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
        VK_CHECK(vkFlushMappedMemoryRanges(device.LogicalDevice, 1, &range));
    }

    return true;
}

bool VulkanAPI::RenderBufferRead(RenderBuffer &buffer, u64 offset, u64 size, void **OutMemory)
{
    if (!buffer.data || !OutMemory) {
        MERROR("Для работы функции VulkanAPI::RenderBufferRead требуется действительный указатель на буфер и значение OutMemory, размер которых должен быть ненулевым.");
        return false;
    }

    auto VkBuf = (VulkanBuffer*)buffer.data;
    if (VulkanBufferIsDeviceLocal(VkBuf) && !VulkanBufferIsHostVisible(VkBuf)) {
        // ПРИМЕЧАНИЕ: Если необходим буфер чтения (т.е. память целевого буфера не видна хосту, но является локальной для устройства, 
        // создайте буфер чтения, скопируйте в него данные, затем выполните чтение из этого буфера.

        // Создайте видимый хосту промежуточный буфер для копирования. Отметьте его как место назначения передачи.
        RenderBuffer read { "renderbuffer_read",  RenderBufferType::Read, RenderBuffer::AllocationType::None, size };
        if (!RenderBufferCreate(read)) {
            MERROR("VulkanAPI::RenderBufferRead() - Не удалось создать буфер чтения.");
            return false;
        }
        RenderBufferBind(read, 0);
        auto ReadInternal = (VulkanBuffer*)read.data;

        // Выполните копирование из локального устройства в буфер чтения.
        RenderBufferCopyRange(buffer, offset, read, 0, size, true);

        // Map/copy/unmap
        void* MappedData;
        VK_CHECK(vkMapMemory(device.LogicalDevice, ReadInternal->memory, 0, size, 0, &MappedData));
        MemorySystem::CopyMem(*OutMemory, MappedData, size);
        vkUnmapMemory(device.LogicalDevice, ReadInternal->memory);

        // Очистите буфер чтения.
        RenderBufferUnbind(read);
        RenderBufferDestroy(read);
    } else {
        // Если промежуточный буфер не нужен, отобразите/скопируйте/отмените отображение.
        void* PtrData;
        VK_CHECK(vkMapMemory(device.LogicalDevice, VkBuf->memory, offset, size, 0, &PtrData));
        MemorySystem::CopyMem(*OutMemory, PtrData, size);
        vkUnmapMemory(device.LogicalDevice, VkBuf->memory);
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
    VK_CHECK(vkCreateBuffer(device.LogicalDevice, &BufferInfo, allocator, &NewBuffer));

    // Собрать требования к памяти.
    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(device.LogicalDevice, NewBuffer, &requirements);

    // Выделить информацию о памяти
    VkMemoryAllocateInfo AllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    AllocateInfo.allocationSize = requirements.size;
    AllocateInfo.memoryTypeIndex = (u32)VkBuf->MemoryIndex;

    // Выделить память.
    VkDeviceMemory NewMemory;
    VkResult result = vkAllocateMemory(device.LogicalDevice, &AllocateInfo, allocator, &NewMemory);
    if (result != VK_SUCCESS) {
        MERROR("Невозможно изменить размер буфера vulkan, так как требуемое выделение памяти не удалось. Ошибка: %i", result);
        return false;
    }
    VK_SET_DEBUG_OBJECT_NAME(this, VK_OBJECT_TYPE_DEVICE_MEMORY, NewMemory, buffer.name.c_str());

    // Связать память нового буфера
    VK_CHECK(vkBindBufferMemory(device.LogicalDevice, NewBuffer, NewMemory, 0));

    // Скопировать данные.
    VulkanBufferCopyRangeInternal(VkBuf->handle, 0, NewBuffer, 0, buffer.TotalSize, false);

    // Убедитесь, что все, что потенциально использует их, завершено.
    // ПРИМЕЧАНИЕ: Мы могли бы использовать vkQueueWaitIdle здесь, если бы знали, с какой очередью будет использоваться этот буфер...
    vkDeviceWaitIdle(device.LogicalDevice);

    // Уничтожить старый
    if (VkBuf->memory) {
        vkFreeMemory(device.LogicalDevice, VkBuf->memory, allocator);
        VkBuf->memory = 0;
    }
    if (VkBuf->handle) {
        vkDestroyBuffer(device.LogicalDevice, VkBuf->handle, allocator);
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

bool VulkanAPI::RenderBufferLoadRange(RenderBuffer &buffer, u64 offset, u64 size, const void *data, bool IncludeInFrameWorkload)
{
    if (!buffer.data || !size || !data) {
        MERROR("VulkanAPI::RenderBufferLoadRange требует действительный указатель на буфер, ненулевой размер и действительный указатель на данные.");
        return false;
    }

    auto VkBuf = (VulkanBuffer*)buffer.data;
    if (VulkanBufferIsDeviceLocal(VkBuf) && !VulkanBufferIsHostVisible(VkBuf)) {
        // ПРИМЕЧАНИЕ: Если необходим промежуточный буфер (т.е. память целевого буфера не видна хосту, но является локальной для устройства), 
        // сначала создайте промежуточный буфер для загрузки данных. Затем скопируйте из него в целевой буфер.

        // Загрузите данные в промежуточный буфер.
        u64 StagingOffset = 0;
        staging.Allocate(size, StagingOffset);
        RenderBufferLoadRange(staging, StagingOffset, size, data, IncludeInFrameWorkload);

        // Выполните копирование из промежуточного буфера в локальный буфер устройства.
        RenderBufferCopyRange(staging, StagingOffset, buffer, offset, size, IncludeInFrameWorkload);

    } else {
        // Если промежуточный буфер не нужен, отобразите/скопируйте/отмените отображение.
        void* ptrData;
        VK_CHECK(vkMapMemory(device.LogicalDevice, VkBuf->memory, offset, size, 0, &ptrData));
        MemorySystem::CopyMem(ptrData, data, size);
        vkUnmapMemory(device.LogicalDevice, VkBuf->memory);
    }

    return true;
}

bool VulkanAPI::RenderBufferCopyRange(RenderBuffer &source, u64 SourceOffset, RenderBuffer &dest, u64 DestOffset, u64 size, bool IncludeInFrameWorkload)
{
    if (!source.data || !dest.data || !size) {
        MERROR("VulkanAPI::RenderBufferCopyRange требует действительных указателей на исходный и целевой буферы, а также ненулевой размер.");
        return false;
    }

    return VulkanBufferCopyRangeInternal(
        ((VulkanBuffer*)source.data)->handle,
        SourceOffset,
        ((VulkanBuffer*)dest.data)->handle,
        DestOffset,
        size, IncludeInFrameWorkload);
    return true;
}

bool VulkanAPI::RenderBufferDraw(RenderBuffer &buffer, u64 offset, u32 ElementCount, bool BindOnly)
{
    VkCommandBuffer CommandBuffer = GraphicsCommandBuffers[ImageIndex].handle;

    if (buffer.type == RenderBufferType::Vertex) {
        // Привязать буфер вершин по смещению.
        VkDeviceSize offsets[1] = {offset};
        vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &((VulkanBuffer*)buffer.data)->handle, offsets);
        if (!BindOnly) {
            vkCmdDraw(CommandBuffer, ElementCount, 1, 0, 0);
        }
        return true;
    } else if (buffer.type == RenderBufferType::Index) {
        // Привязать буфер индексов по смещению.
        vkCmdBindIndexBuffer(CommandBuffer, ((VulkanBuffer*)buffer.data)->handle, offset, VK_INDEX_TYPE_UINT32);
        if (!BindOnly) {
            vkCmdDrawIndexed(CommandBuffer, ElementCount, 1, 0, 0, 0);
        }
        return true;
    } else {
        MERROR("Невозможно нарисовать буфер типа: %i", buffer.type);
        return false;
    }
}

bool VulkanAPI::VulkanBufferCopyRangeInternal(VkBuffer source, u64 SourceOffset, VkBuffer dest, u64 DestOffset, u64 size, bool IncludeInFrameWorkload)
{
    VkQueue queue = device.GraphicsQueue;
    VulkanCommandBuffer TempCommandBuffer;
    VulkanCommandBuffer* CommandBuffer = nullptr;

    // Если задача не включается в рабочую нагрузку кадра, следует также использовать новый временный буфер команд. 
    // В противном случае это действие должно выполняться в рамках обработки текущего кадра.
    if (!IncludeInFrameWorkload) {
        vkQueueWaitIdle(queue);
        // Создайте одноразовый буфер команд.
        VulkanCommandBufferAllocateAndBeginSingleUse(this, device.GraphicsCommandPool, TempCommandBuffer);
        CommandBuffer = &TempCommandBuffer;
    } else {
        CommandBuffer = &GraphicsCommandBuffers[ImageIndex];
    }

    // Подготовьте команду копирования и добавьте ее в буфер команд.
    VkBufferCopy CopyRegion {
        .srcOffset = SourceOffset,
        .dstOffset = DestOffset,
        .size = size
    };
    vkCmdCopyBuffer(CommandBuffer->handle, source, dest, 1, &CopyRegion);

    if (!IncludeInFrameWorkload) {
        // Отправьте буфер на выполнение и дождитесь его завершения.
        VulkanCommandBufferEndSingleUse(this, device.GraphicsCommandPool, TempCommandBuffer, queue);
    }
    // ПРИМЕЧАНИЕ: Если ожидание не требуется, отправка будет обработана позже.

    return true;
}
