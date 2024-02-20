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

    return true;
}

void VulkanDeviceDestroy()
{

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
        if (!OutSupportInfo->formats) {
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
        if (!OutSupportInfo->PresentModes) {
            OutSupportInfo->PresentModes = MMemory::TAllocate<VkPresentModeKHR>(sizeof(VkPresentModeKHR) * OutSupportInfo->PresentModeCount, MEMORY_TAG_RENDERER);
        }
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
            PhysicalDevice,
            Surface,
            &OutSupportInfo->PresentModeCount,
            OutSupportInfo->PresentModes));
    }
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
    MINFO("       %d |       %d |       %d |        %d | %s",
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
                MMemory::TFree<VkSurfaceFormatKHR>(OutSwapchainSupport->formats, sizeof(VkSurfaceFormatKHR) * OutSwapchainSupport->FormatCount, MEMORY_TAG_RENDERER);
            }
            if (OutSwapchainSupport->PresentModes) {
                MMemory::TFree<VkPresentModeKHR>(OutSwapchainSupport->PresentModes, sizeof(VkPresentModeKHR) * OutSwapchainSupport->PresentModeCount, MEMORY_TAG_RENDERER);
            }
            MINFO("Требуемая поддержка swapchain отсутствует, устройство пропускается.");
            return false;
        }
        
        // Device extensions.
        if (requirements->DeviceExtensionNames) {
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

                u32 RequiredExtensionCount = darray_length(requirements->DeviceExtensionNames);
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
                        MMemory::TFree<VkExtensionProperties>(AvailableExtensions, sizeof(VkExtensionProperties) * AvailableExtensionCount, MEMORY_TAG_RENDERER);
                        return false;
                    }
                }
            }
            MMemory::TFree<VkExtensionProperties>(AvailableExtensions, sizeof(VkExtensionProperties) * AvailableExtensionCount, MEMORY_TAG_RENDERER);
        }
    }
}
