#include "vulkan_device.hpp"
#include "vulkan_api.hpp"
//#include "core/mmemory.hpp"
#include "containers/mstring.hpp"

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
    for (u32 i = 0; i < IndexCount; ++i) {
        QueueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        QueueCreateInfos[i].queueFamilyIndex = indices[i];
        QueueCreateInfos[i].queueCount = 1;
        // TODO: Включите это для дальнейшего улучшения.
        // if (indices[i] == this->GraphicsQueueIndex) {
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
    DeviceFeatures.fillModeNonSolid = VK_TRUE;

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
        this->PhysicalDevice,
        &DeviceCreateInfo,
        VkAPI->allocator,
        &this->LogicalDevice));

    MINFO("Логическое устройство создано.");

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
        MMemory::Free(SwapchainSupport.formats, sizeof(VkSurfaceFormatKHR) * SwapchainSupport.FormatCount, MemoryTag::Renderer);
        SwapchainSupport.formats = nullptr;
        SwapchainSupport.FormatCount = 0;
    }

    if (SwapchainSupport.PresentModes) {
        MMemory::Free(SwapchainSupport.PresentModes, sizeof(VkPresentModeKHR) * SwapchainSupport.PresentModeCount, MemoryTag::Renderer);
        SwapchainSupport.PresentModes = nullptr;
        SwapchainSupport.PresentModeCount = 0;
    }

    MMemory::ZeroMem(
        &SwapchainSupport.capabilities,
        sizeof(SwapchainSupport.capabilities));

    GraphicsQueueIndex = -1;
    PresentQueueIndex = -1;
    TransferQueueIndex = -1;
}

void VulkanDevice::QuerySwapchainSupport(
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
            OutSupportInfo->formats = MMemory::TAllocate<VkSurfaceFormatKHR>(OutSupportInfo->FormatCount, MemoryTag::Renderer);
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
            OutSupportInfo->PresentModes = MMemory::TAllocate<VkPresentModeKHR>(OutSupportInfo->PresentModeCount, MemoryTag::Renderer);
        }
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
            PhysicalDevice,
            Surface,
            &OutSupportInfo->PresentModeCount,
            OutSupportInfo->PresentModes));
    }
}

bool VulkanDevice::DetectDepthFormat(VulkanDevice* Device)
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

bool VulkanDevice::SelectPhysicalDevice(VulkanAPI *VkAPI)
{
    u32 PhysicalDeviceCount = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(VkAPI->instance, &PhysicalDeviceCount, 0));
    if (PhysicalDeviceCount == 0) {
        MFATAL("Устройств, поддерживающих Vulkan, обнаружено не было.");
        return false;
    }

    const u32 MaxDeviceCount = 32;
    VkPhysicalDevice PhysicalDevices[MaxDeviceCount];
    VK_CHECK(vkEnumeratePhysicalDevices(VkAPI->instance, &PhysicalDeviceCount, PhysicalDevices));
    for (auto &&device : PhysicalDevices) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(device, &features);

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
            &this->SwapchainSupport);

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
        QuerySwapchainSupport(
            device,
            surface,
            OutSwapchainSupport);
        
        if (OutSwapchainSupport->FormatCount < 1 || OutSwapchainSupport->PresentModeCount < 1) {
            if (OutSwapchainSupport->formats) {
                MMemory::Free(OutSwapchainSupport->formats, OutSwapchainSupport->FormatCount * sizeof(VkSurfaceFormatKHR), MemoryTag::Renderer);
            }
            if (OutSwapchainSupport->PresentModes) {
                MMemory::Free(OutSwapchainSupport->PresentModes, OutSwapchainSupport->PresentModeCount * sizeof(VkPresentModeKHR), MemoryTag::Renderer);
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
                AvailableExtensions = MMemory::TAllocate<VkExtensionProperties>(AvailableExtensionCount, MemoryTag::Renderer);
                VK_CHECK(vkEnumerateDeviceExtensionProperties(
                    device,
                    0,
                    &AvailableExtensionCount,
                    AvailableExtensions));

                u32 RequiredExtensionCount = requirements->DeviceExtensionNames.Lenght();
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
                        MMemory::Free(AvailableExtensions, AvailableExtensionCount * sizeof(VkExtensionProperties), MemoryTag::Renderer);
                        return false;
                    }
                }
            }
            MMemory::Free(AvailableExtensions, AvailableExtensionCount * sizeof(VkExtensionProperties), MemoryTag::Renderer);
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
