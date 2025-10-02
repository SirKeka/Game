#include "vulkan_device.h"
#include "vulkan_utils.h"
#include "vulkan_api.h"
#include "containers/mstring.hpp"

struct VulkanPhysicalDeviceRequirements {
        bool graphics;
        bool present;
        bool compute;
        bool transfer;
        DArray<const char*> DeviceExtensionNames;
        bool SamplerAnisotropy;
        bool DiscreteGPU;
        constexpr VulkanPhysicalDeviceRequirements()
        : graphics(), present(), compute(), transfer(), DeviceExtensionNames(), SamplerAnisotropy(), DiscreteGPU() {}
        constexpr VulkanPhysicalDeviceRequirements(bool graphics, bool present, bool transfer, const char* DeviceExtensionNames, bool SamplerAnisotropy, bool DiscreteGPU)
        : graphics(graphics), present(present), compute(), transfer(transfer), DeviceExtensionNames(), SamplerAnisotropy(SamplerAnisotropy), DiscreteGPU(DiscreteGPU) { this->DeviceExtensionNames.PushBack(DeviceExtensionNames); }
        constexpr VulkanPhysicalDeviceRequirements(bool graphics, bool present, bool compute, bool transfer, const char* DeviceExtensionNames, bool SamplerAnisotropy, bool DiscreteGPU)
        : graphics(graphics), present(present), compute(compute), transfer(transfer), DeviceExtensionNames(), SamplerAnisotropy(SamplerAnisotropy), DiscreteGPU(DiscreteGPU) { this->DeviceExtensionNames.PushBack(DeviceExtensionNames); }
    };

bool VulkanDevice::Create(VulkanAPI* VkAPI)
{
    if (!SelectPhysicalDevice(VkAPI)) {
        return false;
    }

    MINFO("Создание логического устройства...");
    // ПРИМЕЧАНИЕ. Не создавайте дополнительные очереди для общих индексов.
    bool PresentSharesGraphicsQueue = this->GraphicsQueueIndex == this->PresentQueueIndex;
    bool TransferSharesGraphicsQueue = this->GraphicsQueueIndex == this->TransferQueueIndex;
    u32 IndexCount = 1;
    if (!PresentSharesGraphicsQueue) {
        IndexCount++;
    }
    if (!TransferSharesGraphicsQueue) {
        IndexCount++;
    }
    u32 indices[32];
    u8 index = 0;
    indices[index++] = this->GraphicsQueueIndex;
    if (!PresentSharesGraphicsQueue) {
        indices[index++] = this->PresentQueueIndex;
    }
    if (!TransferSharesGraphicsQueue) {
        indices[index++] = this->TransferQueueIndex;
    }

    VkDeviceQueueCreateInfo QueueCreateInfos[32];
    f32 QueuePriority = 1.F;
    for (u32 i = 0; i < IndexCount; ++i) {
        QueueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        QueueCreateInfos[i].queueFamilyIndex = indices[i];
        QueueCreateInfos[i].queueCount = 1;
        // ЗАДАЧА: Включите это для дальнейшего улучшения.
        // if (indices[i] == this->GraphicsQueueIndex) {
        //    QueueCreateInfos[i].queueCount = 2;
        // }
        QueueCreateInfos[i].flags = 0;
        QueueCreateInfos[i].pNext = 0;
        QueueCreateInfos[i].pQueuePriorities = &QueuePriority;
    }

    // Запросите характеристики устройства.
    // ЗАДАЧА: должно управляться конфигурацией
    VkPhysicalDeviceFeatures DeviceFeatures = {};
    DeviceFeatures.samplerAnisotropy = VK_TRUE;  // Запросить анизотропию
    DeviceFeatures.fillModeNonSolid = VK_TRUE;   // ЗАДАЧА: Проверить, поддерживается ли?

    bool PortabilityRequired = false;
    u32 AvailableExtensionCount = 0;
    VkExtensionProperties* AvailableExtensions = 0;
    VK_CHECK(vkEnumerateDeviceExtensionProperties(PhysicalDevice, 0, &AvailableExtensionCount, 0));
    if (AvailableExtensionCount != 0) {
        AvailableExtensions = MemorySystem::TAllocate<VkExtensionProperties>(Memory::Renderer, AvailableExtensionCount);
        VK_CHECK(vkEnumerateDeviceExtensionProperties(PhysicalDevice, 0, &AvailableExtensionCount, AvailableExtensions));
        for (u32 i = 0; i < AvailableExtensionCount; ++i) {
            if (MString::Equal(AvailableExtensions[i].extensionName, "VK_KHR_portability_subset")) {
                MINFO("Добавляем необходимое расширение VK_KHR_portability_subset.");
                PortabilityRequired = true;
                break;
            }
        }
    }
    MemorySystem::Free(AvailableExtensions, sizeof(VkExtensionProperties) * AvailableExtensionCount, Memory::Renderer);

    // Создайте массив из 3 элементов, даже если мы не будем использовать их все.
    const char* ExtensionNames[4];
    u32 ExtIndex = 0;
    ExtensionNames[ExtIndex] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    ExtIndex++;
    // Если требуется переносимость (например, для Mac), добавьте её.
    if (PortabilityRequired) {
        ExtensionNames[ExtIndex] = "VK_KHR_portability_subset";
        ExtIndex++;
    }

    bool DynamicStateExtensionIncluded = false;
    // Если динамическая топология не поддерживается изначально, но *поддерживается* через расширение, включите расширение. В случае Mac OS X оба этих условия могут быть ложными.
    if (((supportFlags & NativeDynamicTopologyBit) == 0) &&
        ((supportFlags & DynamicTopologyBit) != 0)) {
        ExtensionNames[ExtIndex] = VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME;
        ExtIndex++;
        DynamicStateExtensionIncluded = true;
    }

    // Если поддерживаются плавные линии, загрузите расширение.
    if ((supportFlags & LineSmoothRasterisationBit)) {
        ExtensionNames[ExtIndex] = VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME;
        ExtIndex++;
    }

    if (!DynamicStateExtensionIncluded) {
        // Если динамическая передняя сторона не поддерживается изначально, но *поддерживается* через расширение, включите это расширение. В случае Mac OS X оба этих значения могут быть неверными.
        if (((supportFlags & NativeDynamicFrontFaceBit) == 0) &&
            ((supportFlags & DynamicFrontFaceBit) != 0)) {
            ExtensionNames[ExtIndex] = VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME;
            ExtIndex++;
            DynamicStateExtensionIncluded = true;
        }
    }
    VkDeviceCreateInfo DeviceCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    DeviceCreateInfo.queueCreateInfoCount = IndexCount;
    DeviceCreateInfo.pQueueCreateInfos = QueueCreateInfos;
    DeviceCreateInfo.pEnabledFeatures = &DeviceFeatures;
    DeviceCreateInfo.enabledExtensionCount = ExtIndex;
    DeviceCreateInfo.ppEnabledExtensionNames = ExtensionNames;

    // Устарел и игнорируется, так что ничего не передавайте.
    DeviceCreateInfo.enabledLayerCount = 0;
    DeviceCreateInfo.ppEnabledLayerNames = 0;

    // VK_EXT_extended_dynamic_state
    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT ExtendedDynamicState = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT};
    ExtendedDynamicState.extendedDynamicState = VK_TRUE;
    DeviceCreateInfo.pNext = &ExtendedDynamicState;

    // Плавная растеризация линий, если поддерживается.
    VkPhysicalDeviceLineRasterizationFeaturesEXT LineRasterizationExt{};
    if (supportFlags & LineSmoothRasterisationBit) {
        LineRasterizationExt.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT;
        LineRasterizationExt.smoothLines = VK_TRUE;
        ExtendedDynamicState.pNext = &LineRasterizationExt;
    }

    // Создайте устройство.
    VK_CHECK(vkCreateDevice(
        this->PhysicalDevice,
        &DeviceCreateInfo,
        VkAPI->allocator,
        &this->LogicalDevice)
    );

    VK_SET_DEBUG_OBJECT_NAME(VkAPI, VK_OBJECT_TYPE_DEVICE, LogicalDevice, "Vulkan Logical Device");

    MINFO("Логическое устройство создано.");

    // Проверить поддержку динамической топологии и загрузить указатель функции, если необходимо.
    if (!(supportFlags & NativeDynamicTopologyBit) &&
        (supportFlags & DynamicTopologyBit)) {
        MINFO("Устройство Vulkan не поддерживает собственную динамическую топологию, но поддерживает её через расширение. Использование расширения.");
        VkAPI->vkCmdSetPrimitiveTopologyEXT = (PFN_vkCmdSetPrimitiveTopologyEXT)vkGetInstanceProcAddr(VkAPI->instance, "vkCmdSetPrimitiveTopologyEXT");
    } else {
        if (supportFlags & NativeDynamicTopologyBit) {
            MINFO("Устройство Vulkan поддерживает собственную динамическую топологию.");
        } else {
            MINFO("Устройство Vulkan не поддерживает собственную или расширенную динамическую топологию.");
        }
    }

    // Проверьте поддержку динамического интерфейса и загрузите указатель на функцию при необходимости.
    if (!(supportFlags & NativeDynamicFrontFaceBit) &&
        (supportFlags & DynamicFrontFaceBit)) {
        MINFO("Устройство Vulkan не поддерживает встроенный динамический интерфейс, но поддерживает его через расширение. Использование расширения.");
        VkAPI->vkCmdSetFrontFaceEXT = (PFN_vkCmdSetFrontFaceEXT)vkGetInstanceProcAddr(VkAPI->instance, "vkCmdSetFrontFaceEXT");
    } else {
        if (supportFlags & NativeDynamicFrontFaceBit) {
            MINFO("Устройство Vulkan поддерживает встроенный динамический интерфейс.");
        } else {
            MINFO("Устройство Vulkan не поддерживает встроенный или расширенный динамический интерфейс.");
        }
    }

    // Получите очереди.
    vkGetDeviceQueue(
        this->LogicalDevice,
        this->GraphicsQueueIndex,
        0,
        &this->GraphicsQueue);

    vkGetDeviceQueue(
        this->LogicalDevice,
        this->PresentQueueIndex,
        0,
        &this->PresentQueue);

    vkGetDeviceQueue(
        this->LogicalDevice,
        this->TransferQueueIndex,
        0,
        &this->TransferQueue);
    MINFO("Получены очереди.");

    // Создайте пул команд для графической очереди.
    VkCommandPoolCreateInfo PoolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    PoolCreateInfo.queueFamilyIndex = this->GraphicsQueueIndex;
    PoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK(vkCreateCommandPool(
        this->LogicalDevice,
        &PoolCreateInfo,
        VkAPI->allocator,
        &this->GraphicsCommandPool));
    MINFO("Пул графических команд создан.");

    return true;
}

void VulkanDevice::Destroy(VulkanAPI *VkAPI)
{
    MINFO("Уничтожение командных пулов...");
    vkDestroyCommandPool(
        VkAPI->Device.LogicalDevice,
        VkAPI->Device.GraphicsCommandPool,
        VkAPI->allocator);

    // Уничтожить логическое устройство
    MINFO("Уничтожение логического устройства...");
    if (VkAPI->Device.LogicalDevice) {
        vkDestroyDevice(VkAPI->Device.LogicalDevice, VkAPI->allocator);
        VkAPI->Device.LogicalDevice = 0;
    }

    // Неустановленные очереди
    GraphicsQueue = 0;
    PresentQueue = 0;
    TransferQueue = 0;

    // Физические устройства не уничтожаются.
    MINFO("Высвобождение ресурсов физического устройства...");
    PhysicalDevice = 0;

    if (SwapchainSupport.formats) {
        MemorySystem::Free(SwapchainSupport.formats, sizeof(VkSurfaceFormatKHR) * SwapchainSupport.FormatCount, Memory::Renderer);
        SwapchainSupport.formats = nullptr;
        SwapchainSupport.FormatCount = 0;
    }

    if (SwapchainSupport.PresentModes) {
        MemorySystem::Free(SwapchainSupport.PresentModes, sizeof(VkPresentModeKHR) * SwapchainSupport.PresentModeCount, Memory::Renderer);
        SwapchainSupport.PresentModes = nullptr;
        SwapchainSupport.PresentModeCount = 0;
    }

    MemorySystem::ZeroMem(
        &SwapchainSupport.capabilities,
        sizeof(SwapchainSupport.capabilities));

    GraphicsQueueIndex = -1;
    PresentQueueIndex = -1;
    TransferQueueIndex = -1;
}

void VulkanDevice::QuerySwapchainSupport(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface, VulkanSwapchainSupportInfo* OutSupportInfo)
{
    // Возможности поверхности
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        PhysicalDevice,
        Surface,
        &OutSupportInfo->capabilities));

    // Форматы поверхностей
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
        PhysicalDevice,
        Surface,
        &OutSupportInfo->FormatCount,
        nullptr));

    if (OutSupportInfo->FormatCount != 0) {
        if (!OutSupportInfo->formats) { // раскоментировать или закоментировать знак ! если будет ошибка
            OutSupportInfo->formats = MemorySystem::TAllocate<VkSurfaceFormatKHR>(Memory::Renderer, OutSupportInfo->FormatCount);
        }
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
            PhysicalDevice,
            Surface,
            &OutSupportInfo->FormatCount,
            OutSupportInfo->formats));
    }

    // Существующие режимы
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
        PhysicalDevice,
        Surface,
        &OutSupportInfo->PresentModeCount,
        0));
    if (OutSupportInfo->PresentModeCount != 0) {
        if (!OutSupportInfo->PresentModes) { // раскоментировать или закоментировать знак ! если будет ошибка
            OutSupportInfo->PresentModes = MemorySystem::TAllocate<VkPresentModeKHR>(Memory::Renderer, OutSupportInfo->PresentModeCount);
        }
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
            PhysicalDevice,
            Surface,
            &OutSupportInfo->PresentModeCount,
            OutSupportInfo->PresentModes));
    }
}

bool VulkanDevice::DetectDepthFormat()
{
    // Формат кандидатов
    const u64 CandidateCount = 3;
    VkFormat candidates[3] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT};

        u8 sizes[3] = { 4, 4, 3 };

    u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    for (u64 i = 0; i < CandidateCount; ++i) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(PhysicalDevice, candidates[i], &properties);

        if ((properties.linearTilingFeatures & flags) == flags) {
            DepthFormat = candidates[i];
            DepthChannelCount = sizes[i];
            return true;
        } else if ((properties.optimalTilingFeatures & flags) == flags) {
            DepthFormat = candidates[i];
            DepthChannelCount = sizes[i];
            return true;
        }
    }

    return false;
}

bool VulkanDevice::SelectPhysicalDevice(VulkanAPI *VkAPI)
{
    u32 PhysicalDeviceCount = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(VkAPI->instance, &PhysicalDeviceCount, 0));
    if (PhysicalDeviceCount == 0) {
        MFATAL("Устройств, поддерживающих Vulkan, обнаружено не было.");
        return false;
    }

    // ЗАДАЧА: Эти требования, вероятно, должны определяться движком
    // конфигурация.
    VulkanPhysicalDeviceRequirements requirements{
        true, // graphics
        true, // present
        // ПРИМЕЧАНИЕ: Включите это, если потребуются вычисления.
        // true, // compute
        true, // transfer
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        true, // SamplerAnisotropy
        true  // DiscreteGPU
    };

    // Переберите физические устройства, чтобы найти то, которое подходит под ваши требования.
    VkPhysicalDevice PhysicalDevices[32];
    VK_CHECK(vkEnumeratePhysicalDevices(VkAPI->instance, &PhysicalDeviceCount, PhysicalDevices));
    for (auto &&device : PhysicalDevices) {
        VkPhysicalDeviceProperties2 properties2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        VkPhysicalDeviceDriverProperties driverProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES};
        properties2.pNext = &driverProperties;
        vkGetPhysicalDeviceProperties2(device, &properties2);
        VkPhysicalDeviceProperties properties = properties2.properties;

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(device, &features);

        VkPhysicalDeviceFeatures2 features2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        // Проверьте поддержку динамической топологии через расширение.
        VkPhysicalDeviceExtendedDynamicStateFeaturesEXT DynamicStateNext = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT};
        features2.pNext = &DynamicStateNext;
        // Проверьте поддержку растеризации сглаженных линий через расширение.
        VkPhysicalDeviceLineRasterizationFeaturesEXT SmoothLineNext = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT};
        DynamicStateNext.pNext = &SmoothLineNext;
        // Выполните запрос.
        vkGetPhysicalDeviceFeatures2(device, &features2);

        VkPhysicalDeviceMemoryProperties memory;
        vkGetPhysicalDeviceMemoryProperties(device, &memory);

        MINFO("Оценка устройства: '%s'.", properties.deviceName)

        // Проверьте, поддерживает ли устройство комбинацию «видимый локальный/хост».
        bool SupportsDeviceLocalHostVisible = false;
        for (u32 i = 0; i < memory.memoryTypeCount; ++i) {
            // Проверьте каждый тип памяти, чтобы увидеть, установлен ли его бит в 1.
            if (
                ((memory.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) &&
                ((memory.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0)) {
                SupportsDeviceLocalHostVisible = true;
                break;
            }
        }

        VulkanPhysicalDeviceQueueFamilyInfo QueueInfo = {};
        bool result = PhysicalDeviceMeetsRequirements(
            device,
            VkAPI->surface,
            &properties,
            &features,
            &requirements,
            &QueueInfo,
            &SwapchainSupport);

        if (result) {
            MINFO("Выбранное устройство: '%s'.", properties.deviceName);
            // Тип графического процессора и т.д.
            switch (properties.deviceType) {
                default:
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                    MINFO("Тип графического процессора неизвестен.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    MINFO("Тип графического процессора - интегрированный.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    MINFO("Тип графического процессора - дескретный.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    MINFO("Тип графического процессора - виртуальный.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    MINFO("Тип графического процессора - CPU.");
                    break;
            }

            MINFO("Версия драйвера графического процессора: %s", driverProperties.driverInfo);

            // Сохпаняем версию API, которая поддерживается устройством.
            ApiMaijor = VK_VERSION_MAJOR(properties.apiVersion);
            ApiMinor  = VK_VERSION_MINOR(properties.apiVersion);
            ApiPatch  = VK_VERSION_PATCH(properties.apiVersion);

            // Версия API Vulkan.
            MINFO("Версия API Vulkan: %d.%d.%d", ApiMaijor, ApiMinor, ApiPatch);

            // Информация о памяти
            for (u32 j = 0; j < memory.memoryHeapCount; ++j) {
                f32 MemorySizeGiB = (((f32)memory.memoryHeaps[j].size) / 1024.F / 1024.F / 1024.F);
                if (memory.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                    MINFO("Локальная память графического процессора: %.2f GiB", MemorySizeGiB);
                } else {
                    MINFO("Общая системная память: %.2f GiB", MemorySizeGiB);
                }
            }

            this->PhysicalDevice = device;
            this->GraphicsQueueIndex = QueueInfo.GraphicsFamilyIndex;
            this->PresentQueueIndex = QueueInfo.PresentFamilyIndex;
            this->TransferQueueIndex = QueueInfo.TransferFamilyIndex;
            // ПРИМЕЧАНИЕ: при необходимости установите здесь вычислительный индекс.

            // Сохраните копию свойств, функций и информации о памяти для последующего использования.
            this->properties = properties;
            this->features = features;
            this->memory = memory;
            this->SupportsDeviceLocalHostVisible = SupportsDeviceLocalHostVisible;

            // Устройство может поддерживать или не поддерживать эту функцию, поэтому сохраните ее здесь.
            if (DynamicStateNext.extendedDynamicState) {
                supportFlags |= DynamicTopologyBit;
                supportFlags |= DynamicFrontFaceBit;
            }
            if (ApiMaijor > 1 || ApiMinor > 2) {
                supportFlags |= NativeDynamicTopologyBit;
                supportFlags |= NativeDynamicFrontFaceBit;
            }
            if (SmoothLineNext.smoothLines) {
                supportFlags |= LineSmoothRasterisationBit;
            }
            break;
        }
    }

    // Убедитесь, что было выбрано устройство
    if (!this->PhysicalDevice) {
        MERROR("Не было найдено никаких физических устройств, отвечающих этим требованиям.");
        return false;
    }

    MINFO("Физическое устройство выбрано.");
    return true;
}

bool VulkanDevice::PhysicalDeviceMeetsRequirements(
    VkPhysicalDevice device, 
    VkSurfaceKHR surface, 
    const VkPhysicalDeviceProperties *properties, 
    const VkPhysicalDeviceFeatures *features, 
    const VulkanPhysicalDeviceRequirements *requirements, // ЗАДАЧА: передавать сюда массив DArray
    VulkanPhysicalDeviceQueueFamilyInfo *OutQueueFamilyInfo, 
    VulkanSwapchainSupportInfo *OutSwapchainSupport)
{
    // Оцените свойства устройства, чтобы определить, соответствует ли оно потребностям нашего приложения.
    OutQueueFamilyInfo->ComputeFamilyIndex = -1;
    OutQueueFamilyInfo->GraphicsFamilyIndex = -1;
    OutQueueFamilyInfo->PresentFamilyIndex = -1;
    OutQueueFamilyInfo->TransferFamilyIndex = -1;

    // Дескретная GPU?
    if (requirements->DiscreteGPU) {
        if (properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            MINFO("Устройство не является дискретным графическим процессором, а он необходим. Пропускаем.");
            return false;
        }
    }

    u32 QueueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &QueueFamilyCount, 0);
    VkQueueFamilyProperties QueueFamilies[32];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &QueueFamilyCount, QueueFamilies);

    // Посмотрите на каждую очередь и посмотрите, какие очереди она поддерживает
    MINFO("Графика | Настоящее | Вычисление | Передача | Название");
    u8 MinTransferScore = 255;
    for (u32 i = 0; i < QueueFamilyCount; ++i) {
        u8 CurrentTransferScore = 0;

        // Графическая очередь?
        if (OutQueueFamilyInfo->GraphicsFamilyIndex == -1 && QueueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            OutQueueFamilyInfo->GraphicsFamilyIndex = i;
            ++CurrentTransferScore;

            // Если также имеется очередь, приоритет отдается группировке из 2.
            VkBool32 SupportsPresent = VK_FALSE;
            VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &SupportsPresent));
            if (SupportsPresent) {
                OutQueueFamilyInfo->PresentFamilyIndex = i;
                ++CurrentTransferScore;
            }
        }

        // Очередь вычислений?
        if (QueueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            OutQueueFamilyInfo->ComputeFamilyIndex = i;
            ++CurrentTransferScore;
        }

        // Transfer queue?
        if (QueueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            // Возьмите индекс, если он является текущим самым низким. 
            // Это увеличивает вероятность того, что это выделенная очередь передачи.
            if (CurrentTransferScore <= MinTransferScore) {
                MinTransferScore = CurrentTransferScore;
                OutQueueFamilyInfo->TransferFamilyIndex = i;
            }
        }

        // Если текущая очередь не найдена, повторите итерацию и выберите первую. 
        // Это должно произойти только в том случае, если существует очередь, 
        // которая поддерживает графику, но НЕ присутствует.
        if (OutQueueFamilyInfo->PresentFamilyIndex == -1) {
            for (u32 i = 0; i < QueueFamilyCount; ++i) {
                VkBool32 SupportsPresent = VK_FALSE;
                VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &SupportsPresent));
                if (SupportsPresent) {
                    OutQueueFamilyInfo->PresentFamilyIndex = i;
                    // Если они расходятся, сообщайте об этом и идите дальше. Это здесь только для устранения неполадок.
                    if (OutQueueFamilyInfo->PresentFamilyIndex != OutQueueFamilyInfo->GraphicsFamilyIndex) {
                        MWARN("Предупреждение: для текущей и графической используется другой индекс очереди: %u.", i);
                    }
                    break;
                }
            }
        }
    }

    // Распечатайте некоторую информацию об устройстве
    MINFO("      %d |         %d |          %d |        %d | %s",
          OutQueueFamilyInfo->GraphicsFamilyIndex != -1,
          OutQueueFamilyInfo->PresentFamilyIndex != -1,
          OutQueueFamilyInfo->ComputeFamilyIndex != -1,
          OutQueueFamilyInfo->TransferFamilyIndex != -1,
          properties->deviceName);

    if (
        (!requirements->graphics || (requirements->graphics && OutQueueFamilyInfo->GraphicsFamilyIndex != -1)) &&
        (!requirements->present || (requirements->present && OutQueueFamilyInfo->PresentFamilyIndex != -1)) &&
        (!requirements->compute || (requirements->compute && OutQueueFamilyInfo->ComputeFamilyIndex != -1)) &&
        (!requirements->transfer || (requirements->transfer && OutQueueFamilyInfo->TransferFamilyIndex != -1))) {
        MINFO("Устройство соответствует требованиям к очереди.");
        MTRACE("Индекс семейства графических изображений: %i", OutQueueFamilyInfo->GraphicsFamilyIndex);
        MTRACE("Текущий семейный индекс:  %i", OutQueueFamilyInfo->PresentFamilyIndex);
        MTRACE("Transfer Family Index: %i", OutQueueFamilyInfo->TransferFamilyIndex);
        MTRACE("Compute Family Index:  %i", OutQueueFamilyInfo->ComputeFamilyIndex);

        // Запросите поддержку цепочки подкачки(swapchain).
        QuerySwapchainSupport(device, surface, OutSwapchainSupport);
        
        if (OutSwapchainSupport->FormatCount < 1 || OutSwapchainSupport->PresentModeCount < 1) {
            if (OutSwapchainSupport->formats) {
                MemorySystem::Free(OutSwapchainSupport->formats, OutSwapchainSupport->FormatCount * sizeof(VkSurfaceFormatKHR), Memory::Renderer);
            }
            if (OutSwapchainSupport->PresentModes) {
                MemorySystem::Free(OutSwapchainSupport->PresentModes, OutSwapchainSupport->PresentModeCount * sizeof(VkPresentModeKHR), Memory::Renderer);
            }
            MINFO("Требуемая поддержка swapchain отсутствует, устройство пропускается.");
            return false;
        }
        
        // Device extensions.
        if (requirements->DeviceExtensionNames.Data()) {
            u32 AvailableExtensionCount = 0;
            VkExtensionProperties* AvailableExtensions = nullptr;
            VK_CHECK(vkEnumerateDeviceExtensionProperties(
                device,
                0,
                &AvailableExtensionCount,
                0));
            if (AvailableExtensionCount != 0) {
                AvailableExtensions = MemorySystem::TAllocate<VkExtensionProperties>(Memory::Renderer, AvailableExtensionCount);
                VK_CHECK(vkEnumerateDeviceExtensionProperties(
                    device,
                    0,
                    &AvailableExtensionCount,
                    AvailableExtensions));

                u32 RequiredExtensionCount = requirements->DeviceExtensionNames.Length();
                for (u32 i = 0; i < RequiredExtensionCount; ++i) {
                    bool found = false;
                    for (u32 j = 0; j < AvailableExtensionCount; ++j) {
                        if (MString::Equal(requirements->DeviceExtensionNames[i], AvailableExtensions[j].extensionName)) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        MINFO("Требуемое расширение не найдено: '%s', устройство пропускается.", requirements->DeviceExtensionNames[i]);
                        MemorySystem::Free(AvailableExtensions, AvailableExtensionCount * sizeof(VkExtensionProperties), Memory::Renderer);
                        return false;
                    }
                }
            }
            MemorySystem::Free(AvailableExtensions, AvailableExtensionCount * sizeof(VkExtensionProperties), Memory::Renderer);
        }

        // Sampler anisotropy
        if (requirements->SamplerAnisotropy && !features->samplerAnisotropy) {
            MINFO("Device does not support samplerAnisotropy, skipping.");
            return false;
        }

        // Устройство отвечает всем требованиям.
        return true;
    }

    return false;
}
