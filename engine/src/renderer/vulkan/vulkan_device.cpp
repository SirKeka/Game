#include "vulkan_device.hpp"
#include "core/logger.hpp"
#include "core/mmemory.hpp"
#include "containers/darray.hpp"
#include "containers/mstring.hpp"

struct VulkanPhysicalDeviceRequirements {
    bool graphics;
    bool present;
    bool compute;
    bool transfer;
    DArray<const char*> DeviceExtensionNames;
    bool SamplerAnisotropy;
    bool DiscreteGPU;
};

struct VulkanPhysicalDeviceQueueFamilyInfo {
    u32 GraphicsFamilyIndex;
    u32 PresentFamilyIndex;
    u32 ComputeFamilyIndex;
    u32 TransferFamilyIndex;
};

bool SelectPhysicalDevice(VulkanAPI* VkAPI);
bool PhysicalDeviceMeetsRequirements(
    VkPhysicalDevice device,
    VkSurfaceKHR surface,
    const VkPhysicalDeviceProperties* properties,
    const VkPhysicalDeviceFeatures* features,
    const VulkanPhysicalDeviceRequirements* requirements,
    VulkanPhysicalDeviceQueueFamilyInfo* OutQueueFamilyInfo,
    VulkanSwapchainSupportInfo* OutSwapchainSupport);

bool VulkanDeviceCreate(VulkanAPI* VkAPI)
{
    if (!SelectPhysicalDevice(VkAPI)) {
        return false;
    }

    MINFO("Создание логического устройства...");
    // ПРИМЕЧАНИЕ. Не создавайте дополнительные очереди для общих индексов.
    bool PresentSharesGraphicsQueue = VkAPI->Device.GraphicsQueueIndex == VkAPI->Device.PresentQueueIndex;
    bool TransferSharesGraphicsQueue = VkAPI->Device.GraphicsQueueIndex == VkAPI->Device.TransferQueueIndex;
    u32 IndexCount = 1;
    if (!PresentSharesGraphicsQueue) {
        IndexCount++;
    }
    if (!TransferSharesGraphicsQueue) {
        IndexCount++;
    }
    u32 indices[IndexCount];
    u8 index = 0;
    indices[index++] = VkAPI->Device.GraphicsQueueIndex;
    if (!PresentSharesGraphicsQueue) {
        indices[index++] = VkAPI->Device.PresentQueueIndex;
    }
    if (!TransferSharesGraphicsQueue) {
        indices[index++] = VkAPI->Device.TransferQueueIndex;
    }

    VkDeviceQueueCreateInfo QueueCreateInfos[IndexCount];
    for (u32 i = 0; i < IndexCount; ++i) {
        QueueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        QueueCreateInfos[i].queueFamilyIndex = indices[i];
        QueueCreateInfos[i].queueCount = 1;
        // TODO: Включите это для дальнейшего улучшения.
        // if (indices[i] == VkAPI->Device.GraphicsQueueIndex) {
        //    QueueCreateInfos[i].queueCount = 2;
        // }
        QueueCreateInfos[i].flags = 0;
        QueueCreateInfos[i].pNext = 0;
        f32 queue_priority = 1.0f;
        QueueCreateInfos[i].pQueuePriorities = &queue_priority;
    }

    // Запросите характеристики устройства.
    // TODO: должно управляться конфигурацией
    VkPhysicalDeviceFeatures DeviceFeatures = {};
    DeviceFeatures.samplerAnisotropy = VK_TRUE;  // Запросить анизотропию

    VkDeviceCreateInfo DeviceCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    DeviceCreateInfo.queueCreateInfoCount = IndexCount;
    DeviceCreateInfo.pQueueCreateInfos = QueueCreateInfos;
    DeviceCreateInfo.pEnabledFeatures = &DeviceFeatures;
    DeviceCreateInfo.enabledExtensionCount = 1;
    const char* ExtensionNames = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    DeviceCreateInfo.ppEnabledExtensionNames = &ExtensionNames;

    // Устарел и игнорируется, так что ничего не передавайте.
    DeviceCreateInfo.enabledLayerCount = 0;
    DeviceCreateInfo.ppEnabledLayerNames = 0;

    // Создайте устройство.
    VK_CHECK(vkCreateDevice(
        VkAPI->Device.PhysicalDevice,
        &DeviceCreateInfo,
        VkAPI->allocator,
        &VkAPI->Device.LogicalDevice));

    MINFO("Логическое устройство создано.");

    // Получите очереди.
    vkGetDeviceQueue(
        VkAPI->Device.LogicalDevice,
        VkAPI->Device.GraphicsQueueIndex,
        0,
        &VkAPI->Device.GraphicsQueue);

    vkGetDeviceQueue(
        VkAPI->Device.LogicalDevice,
        VkAPI->Device.PresentQueueIndex,
        0,
        &VkAPI->Device.PresentQueue);

    vkGetDeviceQueue(
        VkAPI->Device.LogicalDevice,
        VkAPI->Device.TransferQueueIndex,
        0,
        &VkAPI->Device.TransferQueue);
    MINFO("Получены очереди.");

    // Создайте пул команд для графической очереди.
    VkCommandPoolCreateInfo PoolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    PoolCreateInfo.queueFamilyIndex = VkAPI->Device.GraphicsQueueIndex;
    PoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK(vkCreateCommandPool(
        VkAPI->Device.LogicalDevice,
        &PoolCreateInfo,
        VkAPI->allocator,
        &VkAPI->Device.GraphicsCommandPool));
    MINFO("Пул графических команд создан.");

    return true;
}

void VulkanDeviceDestroy(VulkanAPI* VkAPI)
{
    // Неустановленные очереди
    VkAPI->Device.GraphicsQueue = 0;
    VkAPI->Device.PresentQueue = 0;
    VkAPI->Device.TransferQueue = 0;

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

    // Физические устройства не уничтожаются.
    MINFO("Высвобождение ресурсов физического устройства...");
    VkAPI->Device.PhysicalDevice = 0;

    if (VkAPI->Device.SwapchainSupport.formats) {
        MMemory::TFree<VkSurfaceFormatKHR>(
            VkAPI->Device.SwapchainSupport.formats,
            VkAPI->Device.SwapchainSupport.FormatCount,
            MEMORY_TAG_RENDERER);
        VkAPI->Device.SwapchainSupport.formats = 0;
        VkAPI->Device.SwapchainSupport.FormatCount = 0;
    }

    if (VkAPI->Device.SwapchainSupport.PresentModes) {
        MMemory::TFree<VkPresentModeKHR>(
            VkAPI->Device.SwapchainSupport.PresentModes,
            VkAPI->Device.SwapchainSupport.PresentModeCount,
            MEMORY_TAG_RENDERER);
        VkAPI->Device.SwapchainSupport.PresentModes = 0;
        VkAPI->Device.SwapchainSupport.PresentModeCount = 0;
    }

    MMemory::ZeroMemory(
        &VkAPI->Device.SwapchainSupport.capabilities,
        sizeof(VkAPI->Device.SwapchainSupport.capabilities));

    VkAPI->Device.GraphicsQueueIndex = -1;
    VkAPI->Device.PresentQueueIndex = -1;
    VkAPI->Device.TransferQueueIndex = -1;
}

void VulkanDeviceQuerySwapchainSupport(
    VkPhysicalDevice PhysicalDevice, 
    VkSurfaceKHR Surface, 
    VulkanSwapchainSupportInfo * OutSupportInfo)
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
        0));

    if (OutSupportInfo->FormatCount != 0) {
        if (!OutSupportInfo->formats) { // раскоментировать или закоментировать знак ! если будет ошибка
            OutSupportInfo->formats = MMemory::TAllocate<VkSurfaceFormatKHR>(sizeof(VkSurfaceFormatKHR) * OutSupportInfo->FormatCount, MEMORY_TAG_RENDERER);
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
            OutSupportInfo->PresentModes = MMemory::TAllocate<VkPresentModeKHR>(sizeof(VkPresentModeKHR) * OutSupportInfo->PresentModeCount, MEMORY_TAG_RENDERER);
        }
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
            PhysicalDevice,
            Surface,
            &OutSupportInfo->PresentModeCount,
            OutSupportInfo->PresentModes));
    }
}

bool VulkanDeviceDetectDepthFormat(VulkanDevice* Device)
{
    // Формат кандидатов
    const u64 CandidateCount = 3;
    VkFormat candidates[3] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT};

    u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    for (u64 i = 0; i < CandidateCount; ++i) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(Device->PhysicalDevice, candidates[i], &properties);

        if ((properties.linearTilingFeatures & flags) == flags) {
            Device->DepthFormat = candidates[i];
            return true;
        } else if ((properties.optimalTilingFeatures & flags) == flags) {
            Device->DepthFormat = candidates[i];
            return true;
        }
    }

    return false;
}

bool SelectPhysicalDevice(VulkanAPI *VkAPI)
{
    u32 PhysicalDeviceCount = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(VkAPI->instance, &PhysicalDeviceCount, 0));
    if (PhysicalDeviceCount == 0) {
        MFATAL("Устройств, поддерживающих Vulkan, обнаружено не было.");
        return false;
    }

    VkPhysicalDevice PhysicalDevices[PhysicalDeviceCount];
    VK_CHECK(vkEnumeratePhysicalDevices(VkAPI->instance, &PhysicalDeviceCount, PhysicalDevices));
    for (auto &&device : PhysicalDevices) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(device, &features);

        VkPhysicalDeviceMemoryProperties memory;
        vkGetPhysicalDeviceMemoryProperties(device, &memory);
    
        // TODO: Эти требования, вероятно, должны определяться движком
        // конфигурация.
        VulkanPhysicalDeviceRequirements requirements = {};
        requirements.graphics = true;
        requirements.present = true;
        requirements.transfer = true;
        // ПРИМЕЧАНИЕ: Включите это, если потребуются вычисления.
        // requirements.compute = true;
        requirements.SamplerAnisotropy = true;
        requirements.DiscreteGPU = true;
        requirements.DeviceExtensionNames.PushBack(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        VulkanPhysicalDeviceQueueFamilyInfo QueueInfo = {};
        bool result = PhysicalDeviceMeetsRequirements(
            device,
            VkAPI->surface,
            &properties,
            &features,
            &requirements,
            &QueueInfo,
            &VkAPI->Device.SwapchainSupport);

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

            MINFO(
                "Версия драйвера графического процессора: %d.%d.%d",
                VK_VERSION_MAJOR(properties.driverVersion),
                VK_VERSION_MINOR(properties.driverVersion),
                VK_VERSION_PATCH(properties.driverVersion));

            // Версия API Vulkan.
            MINFO(
                "Версия API Vulkan: %d.%d.%d",
                VK_VERSION_MAJOR(properties.apiVersion),
                VK_VERSION_MINOR(properties.apiVersion),
                VK_VERSION_PATCH(properties.apiVersion));

            // Информация о памяти
            for (u32 j = 0; j < memory.memoryHeapCount; ++j) {
                f32 MemorySizeGiB = (((f32)memory.memoryHeaps[j].size) / 1024.0f / 1024.0f / 1024.0f);
                if (memory.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                    MINFO("Локальная память графического процессора: %.2f GiB", MemorySizeGiB);
                } else {
                    MINFO("Общая системная память: %.2f GiB", MemorySizeGiB);
                }
            }

            VkAPI->Device.PhysicalDevice = device;
            VkAPI->Device.GraphicsQueueIndex = QueueInfo.GraphicsFamilyIndex;
            VkAPI->Device.PresentQueueIndex = QueueInfo.PresentFamilyIndex;
            VkAPI->Device.TransferQueueIndex = QueueInfo.TransferFamilyIndex;
            // ПРИМЕЧАНИЕ: при необходимости установите здесь вычислительный индекс.

            // Сохраните копию свойств, функций и информации о памяти для последующего использования.
            VkAPI->Device.properties = properties;
            VkAPI->Device.features = features;
            VkAPI->Device.memory = memory;
            break;
        }
    }

    // Убедитесь, что было выбрано устройство
    if (!VkAPI->Device.PhysicalDevice) {
        MERROR("Не было найдено никаких физических устройств, отвечающих этим требованиям.");
        return false;
    }

    MINFO("Физическое устройство выбрано.");
    return true;
}

bool PhysicalDeviceMeetsRequirements(
    VkPhysicalDevice device, 
    VkSurfaceKHR surface, 
    const VkPhysicalDeviceProperties *properties, 
    const VkPhysicalDeviceFeatures *features, 
    const VulkanPhysicalDeviceRequirements *requirements, // TODO: передавать сюда массив DArray
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
    VkQueueFamilyProperties QueueFamilies[QueueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &QueueFamilyCount, QueueFamilies);

    // Посмотрите на каждую очередь и посмотрите, какие очереди она поддерживает
    MINFO("Графика | Настоящее | Вычисление | Передача | Название");
    u8 MinTransferScore = 255;
    for (u32 i = 0; i < QueueFamilyCount; ++i) {
        u8 CurrentTransferScore = 0;

        // Графическая очередь?
        if (QueueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            OutQueueFamilyInfo->GraphicsFamilyIndex = i;
            ++CurrentTransferScore;
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

        // Существующая очередь?
        VkBool32 SupportsPresent = VK_FALSE;
        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &SupportsPresent));
        if (SupportsPresent) {
            OutQueueFamilyInfo->PresentFamilyIndex = i;
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
        VulkanDeviceQuerySwapchainSupport(
            device,
            surface,
            OutSwapchainSupport);
        
        if (OutSwapchainSupport->FormatCount < 1 || OutSwapchainSupport->PresentModeCount < 1) {
            if (OutSwapchainSupport->formats) {
                MMemory::TFree<VkSurfaceFormatKHR>(OutSwapchainSupport->formats, OutSwapchainSupport->FormatCount, MEMORY_TAG_RENDERER);
            }
            if (OutSwapchainSupport->PresentModes) {
                MMemory::TFree<VkPresentModeKHR>(OutSwapchainSupport->PresentModes, OutSwapchainSupport->PresentModeCount, MEMORY_TAG_RENDERER);
            }
            MINFO("Требуемая поддержка swapchain отсутствует, устройство пропускается.");
            return false;
        }
        
        // Device extensions.
        if (requirements->DeviceExtensionNames.Data()) {
            u32 AvailableExtensionCount = 0;
            VkExtensionProperties* AvailableExtensions = 0;
            VK_CHECK(vkEnumerateDeviceExtensionProperties(
                device,
                0,
                &AvailableExtensionCount,
                0));
            if (AvailableExtensionCount != 0) {
                AvailableExtensions = MMemory::TAllocate<VkExtensionProperties>(sizeof(VkExtensionProperties) * AvailableExtensionCount, MEMORY_TAG_RENDERER);
                VK_CHECK(vkEnumerateDeviceExtensionProperties(
                    device,
                    0,
                    &AvailableExtensionCount,
                    AvailableExtensions));

                u32 RequiredExtensionCount = requirements->DeviceExtensionNames.Lenght();
                for (u32 i = 0; i < RequiredExtensionCount; ++i) {
                    b8 found = FALSE;
                    for (u32 j = 0; j < AvailableExtensionCount; ++j) {
                        if (StringsEqual(requirements->DeviceExtensionNames[i], AvailableExtensions[j].extensionName)) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        MINFO("Требуемое расширение не найдено: '%s', устройство пропускается.", requirements->DeviceExtensionNames[i]);
                        MMemory::TFree<VkExtensionProperties>(AvailableExtensions, AvailableExtensionCount, MEMORY_TAG_RENDERER);
                        return false;
                    }
                }
            }
            MMemory::TFree<VkExtensionProperties>(AvailableExtensions, AvailableExtensionCount, MEMORY_TAG_RENDERER);
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
